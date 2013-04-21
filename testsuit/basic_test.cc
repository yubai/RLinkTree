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

#include <iostream>
#include <fstream>

#include "../rtree.h"

int main(int argc, char *argv[])
{
    cmpt740::Rtree rtree;

    std::cout << "==========Insert Result==========" << std::endl;
    uint32_t min[DIMENSION], max[DIMENSION];
    for (int i = 0; i < 30; i++) {
	for (int j = 0; j < DIMENSION; ++j) {
	    min[j] = i + 1;
	    max[j] = i + 1;
	}
	rtree.Insert(min, max, NULL);
    }

    rtree.Dump();

    std::cout << "==========Search Result==========" << std::endl;
    for (int j = 0; j < DIMENSION; ++j) {
   	min[j] = 2;
   	max[j] = 5;
    }
    std::vector<cmpt740::Rtree::RtreeRecord> results = rtree.Search(min, max);
    std::cout << "Search Results Size:" << results.size() << "\n\n";

    std::cout << "==========Delete Result==========" << std::endl;
    for (int i = 0; i < 1; i++) {
   	for (int j = 0; j < DIMENSION; ++j) {
   	    min[j] = i + 1;
   	    max[j] = i + 1;
   	}
   	rtree.Delete(min, max, NULL);
    }

    rtree.Dump();

    // std::cout << "==========Save/Load Result==========" << std::endl;
    // // rtree.Save();

    // cmpt740::Rtree* rtree2 = new cmpt740::Rtree;
    // rtree2->Load();

    // rtree2->Dump();

    return 0;
}
