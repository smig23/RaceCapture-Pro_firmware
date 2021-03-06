/*
 * luaCommands.c
 *
 *  Created on: Jul 24, 2011
 *      Author: brent
 */
#include "mod_string.h"
#include "luaCommands.h"
#include "modp_numtoa.h"
#include "modp_atonum.h"
#include "luaScript.h"
#include "lua.h"
#include "luaTask.h"
#include "memory.h"
#include "FreeRTOS.h"
#include "printk.h"

#define LINE_BUFFER_SIZE  256

void ExecLuaInterpreter(Serial *serial, unsigned int argc, char **argv){

	serial->put_s("Entering Lua Interpreter. enter 'exit' to leave");
	put_crlf(serial);

	lua_State *L = getLua();

	cmd_context *cmdContext = get_command_context();
	char * luaLine = cmdContext->lineBuffer;

	int result;
	while(1){
		serial->put_s("> ");
		interactive_read_line(serial, luaLine, cmdContext->lineBufferSize);
		if (strcmp(luaLine,"exit") == 0) break;
		lockLua();
		lua_gc(L,LUA_GCCOLLECT,0);
		result = luaL_loadstring(L,luaLine);
		if (0 != result){
			serial->put_s("error: (");
			serial->put_s(lua_tostring(L,-1));
			serial->put_s(")");
			put_crlf(serial);
			lua_pop(L,1);
		}
		else{
				lua_pushvalue(L,-1);
				result = lua_pcall(L,0,0,0);
				if (0 != result){
					serial->put_s("error: (");
					serial->put_s(lua_tostring(L,-1));
					serial->put_s(")");
					put_crlf(serial);
					lua_pop(L,1);
				}
				lua_pop(L,1);
		}
		unlockLua();
	}
}

//unescapes a string in place
void unescape(char *data){
	char *result = data;
	while (*data){
		if (*data == '\\'){
			switch(*(data + 1)){
				case '_':
					*result = ' ';
					break;
				case 'n':
					*result = '\n';
					break;
				case 'r':
					*result = '\r';
					break;
				case '\\':
					*result = '\\';
					break;
				case '"':
					*result = '\"';
					break;
				case '\0': //this should *NOT* happen
					*result = '\0';
					return;
					break;
				default: // unknown escape char?
					*result = ' ';
					break;
			}
			result++;
			data+=2;
		}
		else{
			*result++ = *data++;
		}
	}
	*result='\0';
}

void ReloadScript(Serial *serial, unsigned int argc, char **argv){
	setShouldReloadScript(1);
	put_commandOK(serial);
}


void WriteScriptPage(Serial *serial, unsigned int argc, char **argv){
	if (argc < 2){
		put_commandError(serial, ERROR_CODE_INVALID_PARAM);
		return;
	}

	unsigned int page = modp_atoi(argv[1]);
	char *scriptPage = "";
	if (argc >= 3) scriptPage = argv[2];

	if (page >=0 && page < SCRIPT_PAGES){
		if (argc >= 2) unescape(scriptPage);
		lockLua();
		vPortEnterCritical();
		pr_info_int(strlen(scriptPage));
		pr_info("=");
		pr_info(scriptPage);
		pr_info("\r\n");
		int result = flashScriptPage(page, scriptPage);
		vPortExitCritical();
		unlockLua();
		if (result == 0){
			put_commandOK(serial);
		}
		else{
			put_commandError(serial, result);
		}
	}
	else{
		put_commandError(serial, ERROR_CODE_INVALID_PARAM);
	}
}

void ReadScriptPage(Serial *serial, unsigned int argc, char **argv){

	if (argc < 2){
		put_commandError(serial, ERROR_CODE_INVALID_PARAM);
		return;
	}

	int page = modp_atoi(argv[1]);
	if (page >=0 && page < SCRIPT_PAGES){
		const char * script = getScript();
		//forward to the requested page
		script += (MEMORY_PAGE_SIZE * page);
		//check for truncated page
		size_t scriptLen = strlen(script);
		if (scriptLen > MEMORY_PAGE_SIZE) scriptLen = MEMORY_PAGE_SIZE;
		put_nameEscapedString(serial, "script",script,scriptLen);
	}
	else{
		put_commandError(serial, ERROR_CODE_INVALID_PARAM);
	}
}

