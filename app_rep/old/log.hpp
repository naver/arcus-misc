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
/**
* @file log.hpp
* @brief 
* @author bindung <bindung@nhn.com>
* @version $Id$
* @date 2012-07-05
*/

#include <stdio.h>

#ifndef __LOG_HPP_D7FE238D76DB6ABA__
#define __LOG_HPP_D7FE238D76DB6ABA__

#define ERROR(fmt, args...) fprintf(stderr, "(%s:%d) " fmt "\n", __FILE__, __LINE__, ## args)
#define DEBUG(fmt, args...) fprintf(stdout, "(%s:%d) " fmt "\n", __FILE__, __LINE__, ## args)

#endif /* __LOG_HPP_D7FE238D76DB6ABA__ */

