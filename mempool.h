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

#ifndef _MEMPOOL_H_
#define _MEMPOOL_H_

#include <fstream>
#include <map>
#include "rtree.h"

namespace cmpt740 {

    enum RtreeNodeStatus{AVAILABLE = 1, UNAVAILABLE};

    class Mempool {
    public:
	Mempool(const char* filename);
	virtual ~Mempool();
	Rtree::RtreeNode* LoadRtreeNode(long offset);
	long SaveRtreeNode(Rtree::RtreeNode* node);
    protected:

	struct RtreeNodeEnt {
	    Rtree::RtreeNode* node; // Rtree node address
	    RtreeNodeStatus status; // Rtree node status
	};
    private:
	std::fstream hd;
	std::map<int, RtreeNodeEnt> nodes_map;
    };

}

#endif
