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
#ifndef __LUA_SENDBOX_H__
#define __LUA_SENDBOX_H__

#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#define LUA_ERRBUF_SIZE 1024

typedef struct {
	size_t maxMemSize;
} LUA_SENDBOX_OPTS;

typedef struct {
	lua_State *L;
	LUA_SENDBOX_OPTS opts;
	int memRemain;
	const char *errmsg;
	char errBuf[LUA_ERRBUF_SIZE];
} LUA_SENDBOX;

extern LUA_SENDBOX_OPTS LUA_SENDBOX_DEFAULT;

void initLuaSendbox(const char **luaBody, const char **luaFuncs, luaL_Reg *cfuncs);

LUA_SENDBOX *createLuaSendbox(LUA_SENDBOX_OPTS opts, char *errbuf, int errbuflen);

int callLUA(LUA_SENDBOX *sb, const char *funcName, const char *arg_sig, char *res_sig, ...);

void clearLuaSendBox(LUA_SENDBOX *sb);

void destroyLuaSendbox(LUA_SENDBOX *sb);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __LUA_SENDBOX_H__ */
