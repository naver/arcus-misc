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
#include <string.h>
#include <stdlib.h>

main()
{
	char *str[6] = {"2..9", "0..-1", "24", "-2", "-23..-7", "-23..32"};	
	char *save_ptr, *token;
	int i;
	char *temp = (char *)malloc(20);

	
	for (i = 0; i < 6; i++) {
		fprintf(stderr, "str[%d] = %s\n", i, str[i]);
		sprintf(temp, "%s", str[i]);

		token = strtok_r(temp, "..", &save_ptr);
		while (token != NULL) {
			fprintf(stderr, "\ttoken = %s\n", token);
			token = strtok_r(NULL, "..", &save_ptr);
		}
	}
	free(temp);
}



