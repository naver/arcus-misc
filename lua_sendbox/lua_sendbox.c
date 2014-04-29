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
#include <string.h>

LUA_SENDBOX_OPTS LUA_SENDBOX_DEFAULT = { 1024 * 1024 * 5 };

typedef struct {
	const char **luaBody;
	const char **luaFuncs;

	luaL_Reg *cfuncs;
} LUA_SENDBOX_GLOBAL;

LUA_SENDBOX_GLOBAL __lsg;

static void *sendboxAlloc(void *ud, void *ptr, size_t osize, size_t nsize)
{
	int *remain = (int *)ud;
	int required = nsize - osize;

	if (remain - required < 0) {
		return NULL;
	}

	remain -= required;

	if (nsize == 0) {
		free(ptr);
		return NULL;
	}
	else {
		return realloc(ptr, nsize);
	}
}

void initLuaSendbox(const char **luaBody, const char **luaFuncs, luaL_Reg *cfuncs)
{
	int i, cnt = 0;

	if (cfuncs != NULL) {
		while (cfuncs[cnt].name != NULL) ++cnt;
	}

	__lsg.cfuncs = (luaL_Reg *)malloc(sizeof(luaL_Reg) * (cnt + 1));

	for (i=0;i<cnt;++i) {
		__lsg.cfuncs[i].name = strdup(cfuncs[i].name);
		__lsg.cfuncs[i].func = cfuncs[i].func;
	}
	__lsg.cfuncs[i].name = NULL;
	__lsg.cfuncs[i].func = NULL;

	cnt = 0;
	if (luaBody != NULL) {
		while (luaBody[cnt] != NULL) ++cnt;
	}

	__lsg.luaBody = (const char **)malloc(sizeof(const char *) * (cnt + 1));
	
	for (i=0;i<cnt;++i) {
		__lsg.luaBody[i] = strdup(luaBody[i]);
	}
	__lsg.luaBody[i] = NULL;

	cnt = 0;
	if (luaFuncs != NULL) {
		while (luaFuncs[cnt] != NULL) ++cnt;
	}

	__lsg.luaFuncs = (const char **)malloc(sizeof(const char *) * (cnt + 1));
	
	for (i=0;i<cnt;++i) {
		__lsg.luaFuncs[i] = strdup(luaFuncs[i]);
	}
	__lsg.luaFuncs[i] = NULL;
}

/* http://lua-users.org/wiki/SandBoxes */
const char *make_default_sendbox = 
	"sendbox_mod_string = {}\n"
	"sendbox_mod_string[\"byte\"] = string.byte\n"
	"sendbox_mod_string[\"char\"] = string.char\n"
	"sendbox_mod_string[\"find\"] = string.find\n"
	"sendbox_mod_string[\"format\"] = string.format\n"
	"sendbox_mod_string[\"gmatch\"] = string.gmatch\n"
	"sendbox_mod_string[\"gsub\"] = string.gsub\n"
	"sendbox_mod_string[\"len\"] = string.len\n"
	"sendbox_mod_string[\"lower\"] = string.lower\n"
	"sendbox_mod_string[\"match\"] = string.match\n"
	"sendbox_mod_string[\"rep\"] = string.rep\n"
	"sendbox_mod_string[\"reverse\"] = string.reverse\n"
	"sendbox_mod_string[\"sub\"] = string.sub\n"
	"sendbox_mod_string[\"upper\"] = string.upper\n"
	"\n"
	"sendbox_mod_table = {}\n"
	"sendbox_mod_table[\"insert\"] = table.insert\n"
	"sendbox_mod_table[\"maxn\"] = table.maxn\n"
	"sendbox_mod_table[\"remove\"] = table.remove\n"
	"sendbox_mod_table[\"sort\"] = table.sort\n"
	"\n"
	"sendbox_mod_math = {}\n"
	"sendbox_mod_math[\"abs\"] = math.abs\n"
	"sendbox_mod_math[\"acos\"] = math.acos\n"
	"sendbox_mod_math[\"asin\"] = math.asin\n"
	"sendbox_mod_math[\"atan\"] = math.atan\n"
	"sendbox_mod_math[\"atan2\"] = math.atan2\n"
	"sendbox_mod_math[\"ceil\"] = math.ceil\n"
	"sendbox_mod_math[\"cos\"] = math.cos\n"
	"sendbox_mod_math[\"cosh\"] = math.cosh\n"
	"sendbox_mod_math[\"deg\"] = math.deg\n"
	"sendbox_mod_math[\"exp\"] = math.exp\n"
	"sendbox_mod_math[\"floor\"] = math.floor\n"
	"sendbox_mod_math[\"fmod\"] = math.fmod\n"
	"sendbox_mod_math[\"frexp\"] = math.frexp\n"
	"sendbox_mod_math[\"huge\"] = math.huge\n"
	"sendbox_mod_math[\"idexp\"] = math.idexp\n"
	"sendbox_mod_math[\"log\"] = math.log\n"
	"sendbox_mod_math[\"log10\"] = math.log10\n"
	"sendbox_mod_math[\"max\"] = math.max\n"
	"sendbox_mod_math[\"min\"] = math.min\n"
	"sendbox_mod_math[\"modf\"] = math.modf\n"
	"sendbox_mod_math[\"pi\"] = math.pi\n"
	"sendbox_mod_math[\"pow\"] = math.pow\n"
	"sendbox_mod_math[\"rad\"] = math.rad\n"
	"sendbox_mod_math[\"random\"] = math.random\n"
	"sendbox_mod_math[\"sin\"] = math.sin\n"
	"sendbox_mod_math[\"sinh\"] = math.sinh\n"
	"sendbox_mod_math[\"sqrt\"] = math.sqrt\n"
	"sendbox_mod_math[\"tan\"] = math.tan\n"
	"sendbox_mod_math[\"tanh\"] = math.tanh\n"
	"\n"
	"sendbox_mod_os = {}\n"
	"sendbox_mod_os[\"clock\"] = os.clock\n"
	"sendbox_mod_os[\"date\"] = os.date\n"
	"sendbox_mod_os[\"difftime\"] = os.difftime\n"
	"sendbox_mod_os[\"time\"] = os.time\n"
	"\n"
	"sendbox_env = {}\n"
	"sendbox_env[\"assert\"] = assert\n"
	"sendbox_env[\"error\"] = error\n"
	"sendbox_env[\"ipairs\"] = ipairs\n"
	"sendbox_env[\"pairs\"] = pairs\n"
	"sendbox_env[\"pcall\"] = pcall\n"
	"sendbox_env[\"select\"] = select\n"
	"sendbox_env[\"tonumber\"] = tonumber\n"
	"sendbox_env[\"tostring\"] = tostring\n"
	"sendbox_env[\"type\"] = type\n"
	"sendbox_env[\"unpack\"] = unpack\n"
	"sendbox_env[\"_VERSION\"] = _VERSION\n"
	"sendbox_env[\"xpcall\"] = xpcall\n"
	"sendbox_env[\"string\"] = sendbox_mod_string\n"
	"sendbox_env[\"math\"] = sendbox_mod_math\n"
	"sendbox_env[\"table\"] = sendbox_mod_table\n"
	"sendbox_env[\"os\"] = sendbox_mod_os\n";

LUA_SENDBOX *createLuaSendbox(LUA_SENDBOX_OPTS opts, char *errbuf, int errbuflen)
{
	LUA_SENDBOX *sb = (LUA_SENDBOX *)malloc(sizeof(LUA_SENDBOX));
	sb->L = NULL;
	sb->opts = opts;

	sb->memRemain = sb->opts.maxMemSize;
	sb->L = lua_newstate(sendboxAlloc, &(sb->memRemain));
	if (sb->L == NULL) {
		free(sb);
		return NULL;
	}

	luaL_openlibs(sb->L);

	// make sendbox env
	if (luaL_dostring(sb->L, make_default_sendbox)) {
		if (errbuf != NULL) {
				snprintf(errbuf, errbuflen, "%s", lua_tostring(sb->L, -1));
				errbuf[errbuflen-1] = 0x00;
		}
		lua_close(sb->L);
		return NULL;
	}

	// load 
	int i = 0;
	while(__lsg.luaBody[i] != NULL) {
		if (luaL_loadstring(sb->L, __lsg.luaBody[i]) || lua_pcall(sb->L, 0, 0, 0)) {
			if (errbuf != NULL) {
				snprintf(errbuf, errbuflen, "%s", lua_tostring(sb->L, -1));
				errbuf[errbuflen-1] = 0x00;
			}
			lua_close(sb->L);
			return NULL;
		}
		++i;
	}

	lua_settop(sb->L, 0); // clear previous stack
	lua_getglobal(sb->L, "sendbox_env"); /* index 1 */

	// register cfuncs
	i=0;
	while(__lsg.cfuncs[i].name != NULL) {
		lua_pushcfunction(sb->L, __lsg.cfuncs[i].func);
		lua_setglobal(sb->L, __lsg.cfuncs[i].name);
		lua_getglobal(sb->L, __lsg.cfuncs[i].name);
		lua_setfield(sb->L, 1, __lsg.cfuncs[i].name);
		++i;
	}

	// register lfuncs
	i=0;
	while(__lsg.luaFuncs[i] != NULL) {
		lua_getglobal(sb->L, __lsg.luaFuncs[i]);
		lua_setfield(sb->L, 1, __lsg.luaFuncs[i]);
		++i;
	}
	lua_pop(sb->L, 1);

	return sb;
}

#define ERROR(sb, fmt, ...) \
do { \
	snprintf(sb->errBuf, LUA_ERRBUF_SIZE, fmt, __VA_ARGS__); \
	sb->errBuf[LUA_ERRBUF_SIZE - 1] = 0x00; \
	sb->errmsg = sb->errBuf; \
} while(0)

int dostring(LUA_SENDBOX *sb, const char *string)
{
	lua_settop(sb->L, 0); // clear previous stack
	luaL_checkstack(sb->L, 2, "too many arguments"); // env + func

	if (luaL_loadstring(sb->L, string)) { /* index = 1 */
		sb->errmsg = lua_tostring(sb->L, -1);
		return 0;
	}

	lua_getglobal(sb->L, "sendbox_env"); /* index = 2 */

	if (!lua_setfenv(sb->L, 1)) {
		sb->errmsg = "bug??";
		return 0;
	}

	if (lua_pcall(sb->L, 0, 0, 0) != 0)  { /* do the call */
		sb->errmsg = lua_tostring(sb->L, -1);
		return 0;
	}

	return 1;
}

/* http://www.lua.org/pil/25.3.html */
int call(LUA_SENDBOX *sb, const char *funcName, const char *arg_sig, char *res_sig, ...)
{
	va_list va;
	va_start(va, res_sig);

	int narg = (arg_sig == NULL)?0:strlen(arg_sig);
	int nres = (res_sig == NULL)?0:strlen(res_sig);
	int i;

	lua_settop(sb->L, 0); // clear previous stack

	luaL_checkstack(sb->L, narg + 2, "too many arguments"); // env + func + narg

	lua_getglobal(sb->L, funcName);  /* index = 1 */
	lua_getglobal(sb->L, "sendbox_env"); /* index = 2 */

	if (!lua_setfenv(sb->L, 1)) {
		sb->errmsg = "bug??";
		goto ERROR;
	}

	// push args
	for(i=0;i<narg;++i) {
		switch(arg_sig[i]) {
			case 'd':  /* double argument */
				lua_pushnumber(sb->L, va_arg(va, double));
				break;

			case 'i':  /* int argument */
				lua_pushnumber(sb->L, va_arg(va, int));
				break;

			case 's':  /* string argument */
				lua_pushstring(sb->L, va_arg(va, const char *));
				break;

			case 'b': /* boolean argument */
				lua_pushboolean(sb->L, va_arg(va, int));
				break;

			case 'n': /* nil */
				lua_pushnil(sb->L);
				break;
			
			default:
				ERROR(sb, "invalid option (%c)", arg_sig[i]);
				goto ERROR;
		}
	}

	if (lua_pcall(sb->L, narg, nres, 0) != 0)  { /* do the call */
		sb->errmsg = lua_tostring(sb->L, -1);
		goto ERROR;
	}

	for (i=0;i<nres;++i) {
		int stackIdx = i - nres;

		if (lua_isnil(sb->L, stackIdx)) {
			res_sig[i] = 'n';
			continue;
		}

		switch (res_sig[i]) {
			case 'n':  /* nil */
				break;

			case 'd':  /* double result */
				if (!lua_isnumber(sb->L, stackIdx)) {
					sb->errmsg = "wrong result type";
					goto ERROR;
				}
				*va_arg(va, double *) = lua_tonumber(sb->L, stackIdx);
				break;

			case 'i':  /* int result */
				if (!lua_isnumber(sb->L, stackIdx)) {
					sb->errmsg = "wrong result type";
					goto ERROR;
				}
				*va_arg(va, int *) = (int)lua_tonumber(sb->L, stackIdx);
				break;

			case 'b': /* boolean result */
				if (!lua_isboolean(sb->L, stackIdx)) {
					sb->errmsg = "wrong result type";
					goto ERROR;
				}
				*va_arg(va, int *) = lua_toboolean(sb->L, stackIdx);
				break;

			case 's':  /* string result */
				if (!lua_isstring(sb->L, stackIdx)) {
					sb->errmsg = "wrong result type";
					goto ERROR;
				}
				*va_arg(va, const char **) = lua_tostring(sb->L, stackIdx);
				break;

			default:
				ERROR(sb, "invalid option (%c)", res_sig[i]);
				goto ERROR;
		}
	}

	va_end(va); // clear va
	return 1;

ERROR:
	va_end(va); // clear va
	return 0;
}

void clearLuaSendBox(LUA_SENDBOX *sb)
{
	lua_settop(sb->L, 0);
}

void destroyLuaSendbox(LUA_SENDBOX *sb)
{
	lua_close(sb->L);	
	free(sb);
}
