/*
 * Copyright 2012-2014 NAVER Corp.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <execinfo.h>

main()
{
    bar();
    exit(0);
}

bar()
{
    int a;
    a = foo(fileno(stdout));
    return (a);
}

foo(int file)
{
    assert(file == 10);
}
#if 0
foo(int file)
{
    printstack(file);
}
#endif

#if 0
foo(int file)
{
    //printstack(file);

    void* tracePtrs[100];
    char** funcNames;
    int ii, count;

    count     = backtrace( tracePtrs, 100 );
    funcNames = backtrace_symbols( tracePtrs, count );
    // Print the stack trace
    for(ii = 0; ii < count; ii++ )
        printf( "%s\n", funcNames[ii] );

    // Free the string pointers
    free( funcNames );
}
#endif

