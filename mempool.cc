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

#include "mempool.h"
#include "log.h"

namespace cmpt740 {
    Mempool::Mempool(const char* filename)
    {
	hd.open(filename, std::fstream::in | std::fstream::out);
	if (!hd.is_open()) {
	    hd.open("rtree.dat", std::fstream::in |
		    std::fstream::out | std::fstream::trunc);
	}
    }

    Mempool::~Mempool()
    {
	hd.close();
    }

    Rtree::RtreeNode* Mempool::LoadRtreeNode(long offset)
    {
	std::map<int, RtreeNodeEnt>::iterator it;
	int id = offset / sizeof(Rtree::RtreeNode);
	if ((it = nodes_map.find(id)) != nodes_map.end()) { // Already loaded
	    if ((it->second).status == AVAILABLE)
		return (it->second).node;
	}
	long pos = hd.tellg();
	hd.seekg(offset);
	Rtree::RtreeNode* node = new Rtree::RtreeNode;
	hd.read((char*)node, sizeof(Rtree::RtreeNode));
	hd.seekg(pos);

	RtreeNodeEnt ent;
	ent.node = node;
	ent.status = AVAILABLE;

	nodes_map.insert(std::pair<int, RtreeNodeEnt>(id, ent));
	return node;
    }

    long Mempool::SaveRtreeNode(Rtree::RtreeNode* node)
    {
	long offset = hd.tellp();
	if (node->offset == 0) { // root node
	    hd.seekp(hd.beg);
	    hd.write((char*)node, sizeof(Rtree::RtreeNode));
	    if (offset == 0) offset = sizeof(Rtree::RtreeNode);
	    hd.seekp(offset);
	    hd.flush();
	    return 0;
	} else if (node->offset == -1) { // newly created node
	    node->offset = offset;
	    hd.write((char*)node, sizeof(Rtree::RtreeNode));
	    hd.flush();
	    return offset;
	} else { // node already exists in disk
	    hd.seekp(node->offset);
	    hd.write((char*)node, sizeof(Rtree::RtreeNode));
	    hd.seekp(offset);
	    hd.flush();
	    return node->offset;
	}
    }
}
