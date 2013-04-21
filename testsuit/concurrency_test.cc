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
#include <pthread.h>

#include "../rtree.h"

#define NUM_THREADS  10
#define GAP          4

struct rtree_args {
    int index;
    cmpt740::Rtree* rtree;
};

void* insert_routine(void* arg)
{
    struct rtree_args* p = (struct rtree_args*)arg;
    uint32_t min[DIMENSION], max[DIMENSION];
    for (int i = p->index*GAP; i < p->index*GAP + GAP; i++) {
	for (int j = 0; j < DIMENSION; ++j) {
	    min[j] = i+1;
	    max[j] = i+1;
	}
	p->rtree->Insert(min, max, NULL);
    }
    pthread_exit(NULL);
}

void* search_routine(void* arg)
{
    struct rtree_args* p = (struct rtree_args*)arg;
    uint32_t min[DIMENSION], max[DIMENSION];
    for (int i = p->index*GAP; i < p->index*GAP + GAP; i++) {
    	for (int j = 0; j < DIMENSION; ++j) {
    	    min[j] = i+1;
    	    max[j] = i+1;
    	}
    	p->rtree->Search(min, max);
    }
    pthread_exit(NULL);
}

void* delete_routine(void* arg)
{
    struct rtree_args* p = (struct rtree_args*)arg;
    uint32_t min[DIMENSION], max[DIMENSION];
    for (int i = p->index*GAP; i < p->index*GAP + GAP; i++) {
    	for (int j = 0; j < DIMENSION; ++j) {
    	    min[j] = i+1;
    	    max[j] = i+1;
    	}
    	p->rtree->Delete(min, max, NULL);
    }
    pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
    cmpt740::Rtree rtree;

    int rc;
    pthread_attr_t attr;
    void *status;

    /* Initialize and set thread detached attribute */
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    pthread_t insert_threads[NUM_THREADS];
    std::cout << "==========Insert Result==========" << std::endl;
    for (int i = 0; i < NUM_THREADS; ++i) {
	//	printf("In main: creating thread %d\n", i);
	struct rtree_args* p1 = new struct rtree_args;
	p1->index = i;
	p1->rtree = &rtree;
	rc = pthread_create(&insert_threads[i], NULL, insert_routine, (void*)p1);
	if (rc){
	    printf("ERROR; return code from pthread_create() is %d\n", rc);
	    return -1;
	}
    }

    pthread_t search_threads[NUM_THREADS];
    std::cout << "==========Search Result==========" << std::endl;
    for (int i = 0; i < NUM_THREADS; ++i) {
	//	printf("In main: creating thread %d\n", i);
	struct rtree_args* p2 = new struct rtree_args;
	p2->index = i;
	p2->rtree = &rtree;
	rc = pthread_create(&search_threads[i], NULL, search_routine, (void*)p2);
	if (rc){
	    printf("ERROR; return code from pthread_create() is %d\n", rc);
	    return -1;
	}
    }

    // pthread_t delete_threads[NUM_THREADS];
    // std::cout << "==========Delete Result==========" << std::endl;
    // for (int i = 0; i < NUM_THREADS; ++i) {
    // 	//	printf("In main: creating thread %d\n", i);
    // 	struct rtree_args* p3 = new struct rtree_args;
    // 	p3->index = i;
    // 	p3->rtree = &rtree;
    // 	rc = pthread_create(&delete_threads[i], NULL, delete_routine, (void*)p3);
    // 	if (rc){
    // 	    printf("ERROR; return code from pthread_create() is %d\n", rc);
    // 	    return -1;
    // 	}
    // }

    for (int i = 0; i < NUM_THREADS; ++i) {
    	pthread_join(insert_threads[i], &status);
	pthread_join(search_threads[i], &status);
	// pthread_join(delete_threads[i], &status);
    }

    rtree.Dump();

    return 0;
}
