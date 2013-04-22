/***
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Author:
 *     2012 Bai Yu - zjuyubai@gmail.com
 */

#include <stdint.h>
#include <iostream>
#include <assert.h>

#include "rtree.h"
#include "mempool.h"

namespace cmpt740 {

    long Rtree::global_lsn = 0;

    Rtree::Rtree()
    {
	root = new RtreeNode;
	root->level = 0;
	root->offset = 0;
	root->lsn = global_lsn;
	mempool = new Mempool("rtree.dat");
    }

    Rtree::~Rtree()
    {
	Reset(); // Free, or reset node memory
	delete mempool;
    }

    void Rtree::Reset()
    {
	RemoveAllRec(root);
    }

    void Rtree::FreeNode(RtreeNode* node)
    {
	delete node;
    }

    void Rtree::RemoveAllRec(RtreeNode* node)
    {
	if(node->IsInternalNode()) {
	    for(uint32_t i = 0; i < node->count; ++i) {
//				LoadNode(node->records[i].offset);
		RemoveAllRec((RtreeNode*)node->records[i].child);
	    }
	}
	FreeNode(node);
    }

    long Rtree::SaveNode(RtreeNode* node)
    {
	return mempool->SaveRtreeNode(node);
    }

    Rtree::RtreeNode* Rtree::LoadNode(long offset)
    {
	return mempool->LoadRtreeNode(offset);
    }

    // Insertion
    bool Rtree::Insert(uint32_t min[DIMENSION],
		       uint32_t max[DIMENSION],
		       data_t* data)
    {
	bool ret = false;
	RtreeRecord record;

	for (int i = 0; i < DIMENSION; ++i) {
	    record.rect.max[i] = max[i];
	    record.rect.min[i] = min[i];
	}

	record.data = data;

	ret = InsertRecord(&record, &root);
	//	SaveNode(root);
	return ret;
    }

    // Insert a record
    bool Rtree::InsertRecord(RtreeRecord* record, RtreeNode** root)
    {
        uint64_t lsn;
	lsn = (*root)->lsn;

	RtreeNode* leaf = FindLeaf(*root, record, lsn);
	if (leaf == NULL)
	    return false;
	RtreeRect rect = NodeCover(leaf);
	RtreeNode* newNode;
	bool ret = AddRecord(record, leaf, &newNode);
	//	SaveNode(node);
	if (ret == true) { // leaf node was split
	    ExternParent(leaf, leaf->lsn, leaf->sibling, leaf->sibling->lsn);
	} else {
	    RtreeRect rect2 = NodeCover(leaf);
	    if (isRectCoverChanged(&rect, &rect2)) { // bounding rect changed
		UpdateParent(leaf, rect2);
	    } else {
		leaf->unlock();
	    }
	}
	return true;
    }

    Rtree::RtreeNode* Rtree::FindLeaf(RtreeNode* node, RtreeRecord* record,
				      uint64_t lsn)
    {
	if (node->level == 0) {
	    node->wrlock();
	} else {
	    node->rdlock();
	}

	while (node != NULL && lsn != node->lsn) {
	    RtreeNode* prev = node;
	    node = node->sibling;
	    prev->unlock();
	    if (node != NULL) {
		if (node->level == 0) {
		    node->wrlock();
		} else {
		    node->rdlock();
		}
	    } else {
		return NULL;
	    }
	}

	assert(node != NULL);

	if (node->level == 0) {
	    return node;
	} else {
	    int index = PickRecord(&record->rect, node);
	    lsn = node->records[index].child->lsn;
	    node->unlock();
	    return FindLeaf(node->records[index].child, record, lsn);
	}
	return node;
    }

    void Rtree::ExternParent(RtreeNode* p, uint64_t p_lsn,
			     RtreeNode* q, uint64_t q_lsn)
    {
	if (q->parent == NULL) {
	    RtreeRecord newRecord;
	    RtreeNode* newRoot = new RtreeNode;
	    newRoot->wrlock();
	    newRoot->level = q->level + 1;
	    newRoot->lsn = ++global_lsn;
	    //	    newRoot->offset = 0;
	    newRecord.rect = NodeCover(p);
	    newRecord.child = p;
            newRecord.lsn = p_lsn;
	    //	    p->offset = -1;
	    //	    newRecord.offset = SaveNode(*root);
	    AddRecord(&newRecord, newRoot, NULL);

	    newRecord.rect = NodeCover(q);
	    newRecord.child = q;
            newRecord.lsn = q_lsn;
	    //	    newRecord.offset = SaveNode(newNode);
	    AddRecord(&newRecord, newRoot, NULL);

	    root = newRoot;

	    p->parent = root;
	    q->parent = root;
	    q->unlock();
	    p->unlock();
	    root->unlock();

	} else {
	    RtreeNode* parent = q->parent;
	    RtreeRecord* record = NULL;
	    while (parent != NULL) {
		parent->wrlock();
		for (uint32_t i = 0; i < parent->count; ++i) {
		    if (parent->records[i].child == p) {
			record = &parent->records[i];
			goto found;
		    }
		}
		RtreeNode* prev = parent;
		parent = parent->sibling;
		prev->unlock();
	    }
	found:
	    p->parent = parent;

	    assert(record != NULL);
	    record->lsn = p_lsn;
	    record->rect = NodeCover(p);
	    RtreeRect rect = NodeCover(parent);
	    RtreeRecord newRecord;
	    newRecord.lsn = q_lsn;
	    newRecord.rect = NodeCover(q);
	    newRecord.child = q;

	    RtreeNode* newNode;
	    bool ret = AddRecord(&newRecord, parent, &newNode);

	    if (ret == true) {
		q->unlock();
		p->unlock();

		ExternParent(parent, parent->lsn,
			     parent->sibling, parent->sibling->lsn);

	    } else {
		q->unlock();
		p->unlock();
		RtreeRect rect2 = NodeCover(parent);
		if (isRectCoverChanged(&rect, &rect2)) {// bounding rect changed
		    UpdateParent(parent, rect2);
		} else {
		    parent->unlock();
		}
	    }
	}
    }

    void Rtree::UpdateParent(RtreeNode* node, RtreeRect rect)
    {
	if (node->parent == NULL) {
	    node->unlock();
	} else {
	    RtreeNode* parent = node->parent;
	    assert(parent != NULL);
	    node->unlock();
            parent->wrlock();
	    RtreeRecord* record = NULL;
	    while (parent != NULL) {
		for (uint32_t i = 0; i < parent->count; ++i) {
		    if (parent->records[i].child == node) {
			record = &parent->records[i];
			goto found;
		    }
		}
                RtreeNode* prev = parent;
		parent = parent->sibling;
		assert(parent != NULL);
		prev->unlock();
                parent->wrlock();
	    }
	found:
	    record->lsn = node->lsn;
	    record->rect = NodeCover(node);
	    RtreeRect rect2 = NodeCover(parent);
	    if (isRectCoverChanged(&rect, &rect2)) { // bounding rect changed
		UpdateParent(parent, rect2);
	    } else {
		 parent->unlock();
	    }
	}
    }

    void Rtree::InitNode(RtreeNode* node)
    {
	node->count = 0;
	node->level = -1;
    }

    void Rtree::InitRect(RtreeRect* rect)
    {
	for (int index = 0; index < DIMENSION; ++index) {
	    rect->min[index] = 0;
	    rect->max[index] = 0;
	}
    }

    // Find the smallest rectangle that includes all rectangles in a node.
    Rtree::RtreeRect Rtree::NodeCover(RtreeNode* node)
    {
	int firstTime = true;
	RtreeRect rect;

	for(uint32_t index = 0; index < node->count; ++index) {
	    if ( firstTime ) {
		rect = node->records[index].rect;
		firstTime = false;
	    }
	    else {
		rect = CombineRect(&rect, &(node->records[index].rect));
	    }
	}
	return rect;
    }

    bool Rtree::isRectCoverChanged(RtreeRect* rectA, RtreeRect* rectB)
    {
	for (int i = 0; i < DIMENSION; ++i) {
	    if (rectA->min[i] != rectB->min[i] ||
		rectA->max[i] != rectB->max[i]) {
		return true;
	    }
	}
	return false;
    }

    // Add a branch to a node.  Split the node if necessary.
    // Returns 0 if node not split.  Old node updated.
    // Returns 1 if node split, sets *new_node to address of new node.
    // Old node updated, becomes one of two.
    bool Rtree::AddRecord(RtreeRecord* record, RtreeNode* node,
			  RtreeNode** newNode )
    {
	if (node->count < MAX_REC_NUM_PER_NODE) { // Split won't be necessary
	    node->records[node->count] = *record;
	    node->count++;
	    return false;
	} else {
	    SplitNode(node, record, newNode);
	    return true;
	}
    }

    // Pick a record
    int Rtree::PickRecord(RtreeRect* rect, RtreeNode* node)
    {
	bool firstTime = true;
	uint64_t increase;
	uint64_t bestIncr = -1;
	uint64_t area;
	uint64_t bestArea =  0;
	int best = 0;
	RtreeRect tempRect;

	for (uint32_t index = 0; index < node->count; ++index) {
	    RtreeRect* curRect = &node->records[index].rect;
	    area = CalcRectVolume(curRect);
	    tempRect = CombineRect(rect, curRect);
	    increase = CalcRectVolume(&tempRect) - area;
	    if ((increase < bestIncr) || firstTime) {
		best = index;
		bestArea = area;
		bestIncr = increase;
		firstTime = false;
	    } else if ((increase == bestIncr) && (area < bestArea)) {
		best = index;
		bestArea = area;
		bestIncr = increase;
	    }
	}
	return best;
    }

    // Combine two rectangles into larger one containing both
    Rtree::RtreeRect Rtree::CombineRect(RtreeRect* rectA, RtreeRect* rectB)
    {
	RtreeRect newRect = { {0,}, {0,} };

	for (int index = 0; index < DIMENSION; ++index) {
	    newRect.min[index] = std::min(rectA->min[index], rectB->min[index]);
	    newRect.max[index] = std::max(rectA->max[index], rectB->max[index]);
	}
	return newRect;
    }

    // Split a node.
    // Divides the nodes branches and the extra one between two nodes.
    // Old node is one of the new ones, and one really new one is created.
    // Tries more than one method for choosing a partition, uses best result.
    void Rtree::SplitNode(RtreeNode* node, RtreeRecord* record,
			  RtreeNode** newNode)
    {
	PartitionVars localVars;
	PartitionVars* parVars = &localVars;
	int32_t level;

	// Load all the branches into a buffer, initialize old node
	level = node->level;
	GetRecords(node, record, parVars);

	// Find partition
	ChoosePartition(parVars, MIN_REC_NUM_PER_NODE);

	// Put branches from buffer into 2 nodes according to chosen partition
	*newNode = new RtreeNode;
	(*newNode)->level = node->level = level;
	(*newNode)->lsn = node->lsn;
	node->lsn = ++global_lsn;
	(*newNode)->wrlock();
	LoadNodes(node, *newNode, parVars);
	(*newNode)->sibling = node->sibling;
	(*newNode)->parent = node->parent;
	node->sibling = *newNode;
	node->parent = NULL;
	//	(*newNode)->unlock();
	//	SaveNode(*newNode);
    }

    // Calculate the n-dimensional volume of a rectangle
    uint64_t Rtree::CalcRectVolume(RtreeRect* rect)
    {
	uint64_t volume = 1;

	for ( int index = 0; index < DIMENSION; ++index ) {
	    volume *= rect->max[index] - rect->min[index];
	}

	return volume;
    }

    // Load branch buffer with branches from full node plus the extra branch.
    void Rtree::GetRecords(RtreeNode* node, RtreeRecord* record,
			   PartitionVars* parVars)
    {
	int index;
	// Load the branch buffer
	for (index = 0; index < MAX_REC_NUM_PER_NODE; ++index) {
	    parVars->recordBuf[index] = node->records[index];
	}
	parVars->recordBuf[MAX_REC_NUM_PER_NODE] = *record;
	parVars->recordCount = MAX_REC_NUM_PER_NODE + 1;

	// Calculate rect containing all in the set
	parVars->coverSplit = parVars->recordBuf[0].rect;
	for (index = 1; index < MAX_REC_NUM_PER_NODE + 1; ++index) {
	    parVars->coverSplit = CombineRect(&parVars->coverSplit,
					      &parVars->recordBuf[index].rect);
	}
	parVars->coverSplitArea = CalcRectVolume(&parVars->coverSplit);

	InitNode(node);
    }

    // Method #0 for choosing a partition:
    // As the seeds for the two groups, pick the two rects that would waste the
    // most area if covered by a single rectangle, i.e. evidently the worst pair
    // to have in the same group.
    // Of the remaining, one at a time is chosen to be put in one of the 2 groups.
    // The one chosen is the one with the greatest difference in area expansion
    // depending on which group - the rect most strongly attracted to one group
    // and repelled from the other.
    // If one group gets too full (more would force other group to violate min
    // fill requirement) then other group gets the rest.
    // These last are the ones that can go in either group most easily.
    void Rtree::ChoosePartition(PartitionVars* parVars, int minFill)
    {
	int64_t biggestDiff;
	int group, chosen = 0, betterGroup = 0;

	InitParVars(parVars, parVars->recordCount, minFill);
	PickSeeds(parVars);

	while (((parVars->count[0] + parVars->count[1] ) < parVars->total)
	       && (parVars->count[0] < (parVars->total - parVars->minFill))
	       && (parVars->count[1] < (parVars->total - parVars->minFill))) {
	    biggestDiff = - 1;
	    for (int index = 0; index < parVars->total; ++index) {
		if (!parVars->taken[index]) {
		    RtreeRect* curRect = &parVars->recordBuf[index].rect;
		    RtreeRect rect0 = CombineRect(curRect, &parVars->cover[0]);
		    RtreeRect rect1 = CombineRect(curRect, &parVars->cover[1]);
		    int64_t growth0 = CalcRectVolume(&rect0) - parVars->area[0];
		    int64_t growth1 = CalcRectVolume(&rect1) - parVars->area[1];
		    int64_t diff = growth1 - growth0;
		    if (diff >= 0) {
			group = 0;
		    } else {
			group = 1;
			diff = -diff;
		    }

		    if (diff > biggestDiff) {
			biggestDiff = diff;
			chosen = index;
			betterGroup = group;
		    } else if (( diff == biggestDiff ) &&
			       (parVars->count[group] < parVars->count[betterGroup])) {
			chosen = index;
			betterGroup = group;
		    }
		}
	    }
	    Classify(chosen, betterGroup, parVars);
	}

	// If one group too full, put remaining rects in the other
	if ((parVars->count[0] + parVars->count[1] ) < parVars->total) {
	    if (parVars->count[0] >= parVars->total - parVars->minFill) {
		group = 1;
	    } else {
		group = 0;
	    }
	    for (int index = 0; index < parVars->total; ++index) {
		if (!parVars->taken[index]) {
		    Classify(index, group, parVars);
		}
	    }
	}
    }

    // Copy branches from the buffer into two nodes according to the partition.
    void Rtree::LoadNodes(RtreeNode* nodeA, RtreeNode* nodeB,
			  PartitionVars* parVars)
    {
	for (int index = 0; index < parVars->total; ++index) {
	    if (parVars->partition[index] == 0) {
		AddRecord(&parVars->recordBuf[index], nodeA, NULL);
	    }
	    else if (parVars->partition[index] == 1) {
		AddRecord(&parVars->recordBuf[index], nodeB, NULL);
	    }
	}
    }

    // Initialize a PartitionVars structure.
    void Rtree::InitParVars(PartitionVars* parVars, int maxRects, int minFill)
    {
	parVars->count[0] = parVars->count[1] = 0;
	parVars->area[0] = parVars->area[1] = 0;
	parVars->total = maxRects;
	parVars->minFill = minFill;
	for ( int index = 0; index < maxRects; ++index )
	{
	    parVars->taken[index] = false;
	    parVars->partition[index] = -1;
	}
    }

    void Rtree::PickSeeds(PartitionVars* parVars)
    {
	int seed0 = 0, seed1 = 0;
	int64_t worst, waste;
	int64_t area[MAX_REC_NUM_PER_NODE+1];

	for (int index = 0; index < parVars->total; ++index) {
	    area[index] = CalcRectVolume(&parVars->recordBuf[index].rect);
	}

	worst = -parVars->coverSplitArea - 1;
	for (int indexA = 0; indexA < parVars->total - 1; ++indexA) {
	    for (int indexB = indexA + 1; indexB < parVars->total; ++indexB) {
		RtreeRect oneRect = CombineRect(&parVars->recordBuf[indexA].rect,
						&parVars->recordBuf[indexB].rect);
		waste = CalcRectVolume(&oneRect) - area[indexA] - area[indexB];
		if ( waste > worst ) {
		    worst = waste;
		    seed0 = indexA;
		    seed1 = indexB;
		}
	    }
	}
	Classify(seed0, 0, parVars);
	Classify(seed1, 1, parVars);
    }

    // Put a record in one of the groups.
    void Rtree::Classify(int index, int group, PartitionVars* parVars)
    {
	parVars->partition[index] = group;
	parVars->taken[index] = true;

	if (parVars->count[group] == 0)	{
	    parVars->cover[group] = parVars->recordBuf[index].rect;
	} else {
	    parVars->cover[group] = CombineRect(&parVars->recordBuf[index].rect,
						&parVars->cover[group]);
	}
	parVars->area[group] = CalcRectVolume(&parVars->cover[group]);
	++parVars->count[group];
    }

    bool Rtree::Delete(uint32_t min[DIMENSION], uint32_t max[DIMENSION],
		       data_t* data)
    {
	RtreeRecord record;

	for (int i = 0; i < DIMENSION; ++i) {
	    record.rect.max[i] = max[i];
	    record.rect.min[i] = min[i];
	}

	record.data = data;

	DeleteRecord(&record, &root);
	return false;
    }

    bool Rtree::DeleteRecord(RtreeRecord* record, RtreeNode** node)
    {
	bool ret = false;

	RtreeNode* leaf = FindLeaf(*node, record, (*node)->lsn);
	RtreeRect rect = NodeCover(leaf);

	for (uint32_t index = 0; index < leaf->count; ++index) {
	    if (Overlap(&record->rect, &(leaf->records[index].rect))) {
		DisconnectRecord(leaf, index);
		ret = true;
	    }
	}

	RtreeRect rect2 = NodeCover(leaf);

	if (isRectCoverChanged(&rect, &rect2)) { // bounding rect changed
	    UpdateParent(leaf, rect2);
	} else {
	    leaf->unlock();
	}

	return ret;
    }

    // Disconnect a dependent node.
    // Caller must return after this as count has changed
    void Rtree::DisconnectRecord(RtreeNode* node, int index)
    {
	// Remove element by swapping with the last element to prevent gaps
	node->records[index] = node->records[node->count - 1];
	--node->count;
    }

    // Decide whether two rectangles overlap.
    bool Rtree::Overlap(RtreeRect* rectA, RtreeRect* rectB)
    {
	for (int index = 0; index < DIMENSION; ++index) {
	    if (rectA->min[index] > rectB->max[index] ||
		rectB->min[index] > rectA->max[index]) {
		return false;
	    }
	}
	return true;
    }

    // Search
    std::vector<Rtree::RtreeRecord> Rtree::Search(uint32_t min[DIMENSION],
						  uint32_t max[DIMENSION])
    {
	RtreeRecord record;

	for (int i = 0; i < DIMENSION; ++i) {
	    record.rect.max[i] = max[i];
	    record.rect.min[i] = min[i];
	}

	record.data = NULL;

	return SearchRecord(&record);
    }

    std::vector<Rtree::RtreeRecord> Rtree::SearchRecord(RtreeRecord* record)
    {
	std::vector<Rtree::RtreeRecord> results;
	std::stack<Rtree::RtreeNodeLSN*> stk;
	Rtree::RtreeNodeLSN* nodelsn = new Rtree::RtreeNodeLSN;
	nodelsn->node = root;
	nodelsn->lsn = root->lsn;
	stk.push(nodelsn);
	SearchRecordInNode(record, stk, results);
	return results;
    }

    // // Range query version
    // void Rtree::SearchRecordInNode(RtreeRecord* record,
    // 				   std::stack<Rtree::RtreeNodeLSN*>& stk,
    // 				   std::vector<Rtree::RtreeRecord>& results)
    // {
    // 	while (!stk.empty()) {
    // 	    Rtree::RtreeNodeLSN* nodelsn = stk.top();
    // 	    stk.pop();
    // 	    Rtree::RtreeNode* node = nodelsn->node;
    // 	    node->rdlock();
    // 	    if (node->level == 0) { // leaf node
    // 		for(uint32_t index=0; index < node->count; ++index) {
    // 		    if(Overlap(&record->rect, &(node->records[index].rect))) {
    // 			results.push_back(node->records[index]);
    // 		    }
    // 		}
    // 		node->unlock();
    // 		continue;
    // 	    }
    // 	    node->rdlock();
    // 	    uint64_t lsn = nodelsn->lsn;
    // 	    while (node != NULL && lsn != node->lsn) {
    // 	    	RtreeNode* prev = node;
    // 	    	node = node->sibling;
    // 	    	assert(node != NULL);
    // 	    	prev->unlock();
    // 	    	node->rdlock();
    // 	    	Rtree::RtreeNodeLSN* nl = new Rtree::RtreeNodeLSN;
    // 	    	nl->node = node;
    // 	    	nl->lsn = node->lsn;
    // 	    	stk.push(nl);
    // 	    }
    // 	    for(uint32_t index=0; index < node->count; ++index) {
    // 		if(Overlap(&record->rect, &(node->records[index].rect))) {
    // 		    //		    LoadNode(node->records[index].offset);
    // 		    RtreeNodeLSN* nodelsn = new RtreeNodeLSN;
    // 		    nodelsn->node = node->records[index].child;
    // 		    nodelsn->lsn = node->records[index].child->lsn;
    // 		    stk.push(nodelsn);
    // 		}
    // 	    }
    // 	    node->unlock();
    // 	}
    // }

    // Point query version
    void Rtree::SearchRecordInNode(RtreeRecord* record,
    				   std::stack<Rtree::RtreeNodeLSN*>& stk,
    				   std::vector<Rtree::RtreeRecord>& results)
    {
	while (!stk.empty()) {
    	    Rtree::RtreeNodeLSN* nodelsn = stk.top();
    	    stk.pop();
    	    Rtree::RtreeNode* node = nodelsn->node;
    	    if (node->level == 0) { // leaf node
    	    	for(uint32_t index=0; index < node->count; ++index) {
    	    	    if(Overlap(&record->rect, &(node->records[index].rect))) {
    	    		results.push_back(node->records[index]);
	    		return;
    	    	    }
    	    	}
    	    	continue;
    	    }
    	    node->rdlock();
    	    uint64_t lsn = nodelsn->lsn;
    	    while (node != NULL && lsn != node->lsn) {
    	    	RtreeNode* prev = node;
    	    	node = node->sibling;
    	    	assert(node != NULL);
    	    	prev->unlock();
    	    	node->rdlock();
    	    	Rtree::RtreeNodeLSN* nl = new Rtree::RtreeNodeLSN;
    	    	nl->node = node;
    	    	nl->lsn = node->lsn;
    	    	stk.push(nl);
    	    }
    	    for(uint32_t index=0; index < node->count; ++index) {
    	    	if(Overlap(&record->rect, &(node->records[index].rect))) {
    	    	    // LoadNode(node->records[index].offset);
    	    	    RtreeNodeLSN* nodelsn = new RtreeNodeLSN;
    	    	    nodelsn->node = node->records[index].child;
    	    	    nodelsn->lsn = node->records[index].child->lsn;
    	    	    stk.push(nodelsn);
    	    	}
    	    }
    	    node->unlock();
    	}
    }

    // bool Rtree::Save(std::ofstream& out)
    //     void Rtree::Save()
    //     {
    // 	std::queue<RtreeNode*> nodeque;
    // //	hd.write((char*)root, sizeof(RtreeNode));
    // 	SaveNode(root);
    // 	nodeque.push(root);
    // 	SaveRec(nodeque);
    // //	hd.close();
    //     }

    // bool Rtree::SaveRec(std::ofstream& out, std::queue<RtreeNode*> nodeque)
    // void Rtree::SaveRec(std::queue<RtreeNode*> nodeque)
    // {
    // 	int size = nodeque.size();
    // 	for (int i = 0; i < size; ++i) {
    // 	    RtreeNode* node = nodeque.front();
    // 	    if(node->IsInternalNode()) {
    // 		for(int index=0; index < node->count; ++index) {
    // 		    node->records[index].offset = hd.tellp();
    // 		    // hd.write((char*)node->records[index].child,
    // 		    // 	      sizeof(RtreeNode));
    // 		    SaveNode(node->records[index].child);
    // 		    nodeque.push(node->records[index].child);
    // 		}
    // 	    }
    // 	    nodeque.pop();
    // 	}

    // 	if (!nodeque.empty())
    // 	    SaveRec(nodeque);
    // }

    void Rtree::Load()
    {
	root = LoadNode(root->offset);
    }

    void Rtree::LoadRec(std::queue<RtreeNode*> nodeque)
    {
	int size = (int)nodeque.size();
	for (int i = 0; i < size; ++i) {
	    RtreeNode* node = nodeque.front();
	    node = LoadNode(node->offset);
	    if(node->IsInternalNode()) {
		for(uint32_t index=0; index < node->count; ++index) {
		    RtreeNode* child = new RtreeNode;
		    nodeque.push(child);
		    node->records[index].child = child;
		}
	    }
	    nodeque.pop();
	}

	if (!nodeque.empty())
	    LoadRec(nodeque);
    }

    void Rtree::Dump()
    {
	std::queue<RtreeNode*> nodeque;
	if (root == NULL)
	    Load();
	nodeque.push(root);
	NodeDump(nodeque);
    }

    void Rtree::NodeDump(std::queue<RtreeNode*> nodeque)
    {
	int size = (int)nodeque.size();

	for (int i = 0; i < size; ++i) {
	    RtreeNode* node = nodeque.front(); /// Get the leftmost node
	    std::cout << "---- level: " << node->level <<
		", lsn: " << node->lsn << ", parent: " << node->parent << std::endl;
	    for(uint32_t index = 0; index < node->count; ++index) {
		node->records[index].rect.disp();
		if (index < node->count - 1)
		    std::cout << " -> ";
		if(node->IsInternalNode()) {
		    //		    LoadNode(node->records[index].offset);
		    nodeque.push(node->records[index].child);
		}
	    }

	    std::cout << std::endl << std::endl;
	    nodeque.pop();
	}

	if (!nodeque.empty())
	    NodeDump(nodeque);
    }
}
