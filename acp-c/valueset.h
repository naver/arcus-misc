/*
 * acp-c : Arcus C Client Performance benchmark program
 * Copyright 2013-2014 NAVER Corp.
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
#ifndef _VALUESET_H_
#define _VALUESET_H_

struct valueset;

struct valueset *valueset_init(int min, int max);
void valueset_reset(struct valueset *vs);
int valueset_get_value(struct valueset *vs, uint8_t **ptr);
void valueset_return_value(struct valueset *vs, uint8_t *ptr);

#endif /* !defined(_VALUESET_H_) */
