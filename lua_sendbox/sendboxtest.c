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
#include "lua_sendbox.h"
#include <stdlib.h>

int add(lua_State *L)
{
	double a = lua_tonumber(L, 1);
	double b = lua_tonumber(L, 2);
	lua_pushnumber(L, a+b);
	return 1;
}

int debugwrite(lua_State *L)
{
	const char *a = lua_tostring(L, 1);
	printf("%s\n", a);
	return 0;
}

const char *loadFile(const char *file)
{
	FILE *fp = fopen(file, "rt");

	if (fp == NULL) {
		perror("fopen");
		exit(-1);
	}

	int bufsiz = BUFSIZ;
	int remain = bufsiz;
	char *buf = (char *)malloc(bufsiz);
	char *ptr = buf;

	while(1) {
		if (remain < 1024) {
			remain += bufsiz;
			bufsiz += bufsiz;
			buf = realloc(buf, bufsiz);
		}

		int len = fread(ptr, 1, remain, fp);
		if (len < remain) {
			ptr[len] = 0x00;
			return ptr;
		}
		ptr += len;
		remain -= len;
	}
}

int main(void)
{
	luaL_Reg cfuncs[] = {
		{ "add", add },
		{ "debugwrite", debugwrite },
		{ NULL, NULL }
	};

	const char *luaBody[] = {
		loadFile("scripts/lua_func.lua"),
		NULL
	};

	const char *luaFuncs[] = {
		"lua_add",
		NULL
	};

	initLuaSendbox(luaBody, luaFuncs, cfuncs);

	char errbuf[BUFSIZ];
	LUA_SENDBOX *sb = createLuaSendbox(LUA_SENDBOX_DEFAULT, errbuf, BUFSIZ);

	if (sb == NULL) {
		printf("%s\n", errbuf);
		exit(-1);
	}

	// #1. call lua native function
	call(sb, "print", "is", NULL, 1, " melong");

	// #2. call c func
	int result;
	if (!call(sb, "add", "ii", "i", 1, 2, &result)) {
		printf("fail add : %s\n", sb->errmsg);
	}
	else {
		printf("success : add returns %d\n", result);
	}

	// #3. call lua func
	if (!call(sb, "lua_add", "ii", "i", 1, 3, &result)) {
		printf("fail lua_add : %s\n", sb->errmsg);
	}
	else {
		printf("success : lua_add returns %d\n", result);
	}

	// #4. violate sendbox
	if (!call(sb, "sendbox_violate", NULL, NULL)) {
		printf("fail sendbox_violate : %s\n", sb->errmsg);
	}
	else {
		printf("success : sendbox_violate\n");
	}

	// #5. save sendbox
	if (!call(sb, "sendbox_safe", NULL, NULL)) {
		printf("fail sendbox_safe : %s\n", sb->errmsg);
	}
	else {
		printf("success : sendbox_safe\n");
	}

	//#do string
	if (!dostring(sb, "debugwrite( \"it is lua script. 1 + 3 = \"..add(1,3))")) {
		printf("fail dostring : %s\n", sb->errmsg);
	}

	//#do string violate sendbox
	if (!dostring(sb, "debugwrite( \"it is lua script. 1 - 3 = \"..lua_del(1,3))")) {
		printf("fail dostring : %s\n", sb->errmsg);
	}



	clearLuaSendBox(sb);
}
