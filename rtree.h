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

#ifndef _RTREE_H_
#define _RTREE_H_

#include <iostream>
#include <vector>
#include <stack>
#include <queue>
#include <unistd.h>
#include <stdint.h>
#include <pthread.h>

//#include "mempool.h"

namespace cmpt740 {

    class Mempool;

    namespace internal {
        #define LEAF_LEVEL 0
        //#define PAGE_SIZE (1 << 12)
        #define DIMENSION 3
	// Page size for each RtreeNode
	// But seems too slow when the number of records goes too high
        // #define MAX_REC_NUM_PER_NODE ((PAGE_SIZE - sizeof(int32_t) -
	// - sizeof(long) - sizeof(uint32_t)) / sizeof(struct RtreeRecord))

        #define MAX_REC_NUM_PER_NODE 6
        #define MIN_REC_NUM_PER_NODE (MAX_REC_NUM_PER_NODE / 2)
	typedef void* data_t;
    }

    using namespace internal;

    class Rtree {
    public:
	static long global_lsn;
	struct RtreeNode; // forward declaration
    protected:
	// Rtree rectangle
        struct RtreeRect {
	    uint32_t max[DIMENSION];
	    uint32_t min[DIMENSION];
	    void disp() {
	        std::cout << "[";
		for (int index = 0; index < DIMENSION; ++index) {
	            std::cout << "(" << min[index] << ", " << max[index] << ")";
                }
		std::cout << "]";
            }
        };

    public:
	// Rtree record
        struct RtreeRecord {
	    struct RtreeRect rect;
	    union{
	        RtreeNode* child;
		data_t* data;
            };
	    long offset;
	    uint64_t lsn;
	    RtreeRecord(){this->child = NULL; this->data = NULL; offset = -1;}
        };

	// Rtree node
        struct RtreeNode {
	    int32_t level;
	    uint32_t count;
	    long offset;
	    uint64_t lsn;
	    RtreeNode* parent;
	    RtreeNode* sibling;
	    pthread_rwlock_t lock;
            struct RtreeRecord records[MAX_REC_NUM_PER_NODE];
            bool IsInternalNode() { return (level > 0); }
            bool IsLeaf() { return (level == 0); }
	    RtreeNode() {
	        level = -1;
		count = 0;
		offset = -1;
		lsn = -1;
		parent = NULL;
		sibling = NULL;
		pthread_rwlock_init(&lock, NULL);
            }
	    void rdlock() {
	        pthread_rwlock_rdlock(&lock);
            }
	    void wrlock() {
	        pthread_rwlock_wrlock(&lock);
            }
	    void unlock() {
	        pthread_rwlock_unlock(&lock);
            }
	    ~RtreeNode() {
		pthread_rwlock_destroy(&lock);
            }
        };

    protected:

	struct RtreeNodeLSN {
	    RtreeNode* node;
	    uint64_t lsn;
        };

	// Variables for finding a split partition
	struct PartitionVars
	{
	    int partition[MAX_REC_NUM_PER_NODE+1];
	    int total;
	    int minFill;
	    int taken[MAX_REC_NUM_PER_NODE+1];
	    int count[2];
	    RtreeRect cover[2];
	    uint64_t area[2];
	    RtreeRecord recordBuf[MAX_REC_NUM_PER_NODE+1];
	    int recordCount;
	    RtreeRect coverSplit;
	    uint64_t coverSplitArea;
        };


    public:
	Rtree();
        virtual ~Rtree();
        bool Insert(uint32_t min[DIMENSION], uint32_t max[DIMENSION], data_t* data);
	bool Delete(uint32_t min[DIMENSION], uint32_t max[DIMENSION], data_t* data);
	std::vector<Rtree::RtreeRecord> Search(uint32_t min[DIMENSION],
	                                       uint32_t max[DIMENSION]);

	void Save();
	void Load();
        void Dump();

    protected:
	void Reset();
        void FreeNode(RtreeNode* node);
        void RemoveAllRec(RtreeNode* node);
	void InitNode(RtreeNode* node);
	void InitRect(RtreeRect* rect);
	bool InsertRecord(RtreeRecord* record, RtreeNode** node);
	RtreeNode* FindLeaf(RtreeNode* node, RtreeRecord* record, uint64_t lsn);
	void ExternParent(RtreeNode* p, uint64_t p_lsn,
	                  RtreeNode* q, uint64_t q_lsn);
	void UpdateParent(RtreeNode* node, RtreeRect rect);
	RtreeRect NodeCover(RtreeNode* node);
	bool isRectCoverChanged(RtreeRect* rectA, RtreeRect* rectB);
	bool AddRecord(RtreeRecord* record, RtreeNode* node, RtreeNode** newNode);
	int PickRecord(RtreeRect* rect, RtreeNode* node);
	RtreeRect CombineRect(RtreeRect* rectA, RtreeRect* rectB);
	void SplitNode(RtreeNode* node, RtreeRecord* record, RtreeNode** newNode);
	uint64_t CalcRectVolume(RtreeRect* rect);
	void GetRecords(RtreeNode* node, RtreeRecord* record,
			PartitionVars* parVars);
	void ChoosePartition(PartitionVars* parVars, int minFill);
	void LoadNodes(RtreeNode* nodeA, RtreeNode* nodeB, PartitionVars* parVars);
	void InitParVars(PartitionVars* parVars, int maxRects, int minFill);
	void PickSeeds(PartitionVars* parVars);
	void Classify(int index, int group, PartitionVars* parVars);

	bool Overlap(RtreeRect* rectA, RtreeRect* rectB);
	bool Inside(RtreeRect* rectA, RtreeRect* rectB);
	std::vector<Rtree::RtreeRecord> SearchRecord(RtreeRecord* record);
	void SearchRecordInNode(RtreeRecord* record,
	                        std::stack<Rtree::RtreeNodeLSN*>& stk,
	                        std::vector<Rtree::RtreeRecord>& results);
	void NodeDump(std::queue<RtreeNode*> nodeque);

	void SaveRoot(RtreeNode* node);
	long SaveNode(RtreeNode* node);
	RtreeNode* LoadNode(long offset);
	void SaveRec(std::queue<RtreeNode*> nodeque);
	void LoadRec(std::queue<RtreeNode*> nodeque);

	bool DeleteRecord(RtreeRecord* record, RtreeNode** node);
	void DisconnectRecord(RtreeNode* node, int index);

    private:
        RtreeNode* root;
	Mempool* mempool;
    };
}

#endif
