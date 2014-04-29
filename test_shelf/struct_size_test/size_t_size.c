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
#include <stdint.h>

main()
{
    size_t a, b, c;

    a = 2097152000;
    b = 2101975680;
    c = a / 100;

    fprintf(stderr, "sizeof(size_t) = %d\n", sizeof(size_t));
    fprintf(stderr, "a = %d\n", a);
    fprintf(stderr, "b = %d\n", b);
    fprintf(stderr, "c = %d\n", c);
    fprintf(stderr, "a - b = %u\n", a - b);  
    fprintf(stderr, "a - b = %d\n", a - b);  
    if ((a - b) < c) 
        fprintf(stderr, "(a - b) < c: true\n");
    else
        fprintf(stderr, "(a - b) < c: false\n");
}
