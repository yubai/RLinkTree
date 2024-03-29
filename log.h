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

#ifndef _LOG_H_
#define _LOG_H_

#include <stdio.h>
#define PPNFS_OUTPUT_FD stdout

#define __Log(__prefix, __fmt, ...)                                     \
    do {                                                                \
        fprintf(PPNFS_OUTPUT_FD, __prefix "|" __FILE__ ":%d:%s|" __fmt, \
                __LINE__,  __FUNCTION__,                                \
                ##__VA_ARGS__ );                                        \
        fflush(PPNFS_OUTPUT_FD);                                        \
       } while(0)

#ifdef DEBUG
#define Log(...) __Log("LOG", __VA_ARGS__)
#else
#define Log(...)
#endif
#define Info(...) __Log("INF", __VA_ARGS__)
#define Warn(...) __Log("WRN", __VA_ARGS__)
#define Err(...)  __Log("ERR", __VA_ARGS__)
#define Bug(...) do { __Log("BUG",__VA_ARGS__); ((char*)NULL)[0] = 0; } while(0)

#endif
