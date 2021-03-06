#include "api.h"
#include "constants.h"
#include "printk.h"
#include "mod_string.h"

#define JSON_TOKENS 100

static jsmn_parser g_jsonParser;
static jsmntok_t g_json_tok[JSON_TOKENS];

const api_t apis[] = SYSTEM_APIS;

void initApi(){
	jsmn_init(&g_jsonParser);
}

void json_int(Serial *serial, const char *name, int value, int more){
	serial->put_c('"');
	serial->put_s(name);
	serial->put_c('"');
	serial->put_c(':');
	put_int(serial, value);
	if (more) serial->put_c(',');
}

void json_uint(Serial *serial, const char *name, unsigned int value, int more){
	serial->put_c('"');
	serial->put_s(name);
	serial->put_c('"');
	serial->put_c(':');
	put_uint(serial, value);
	if (more) serial->put_c(',');
}

void json_string(Serial *serial, const char *name, const char *value, int more){
	serial->put_c('"');
	serial->put_s(name);
	serial->put_c('"');
	serial->put_c(':');
	serial->put_c('"');
	serial->put_s(value);
	serial->put_c('"');
	if (more) serial->put_c(',');
}

void json_float(Serial *serial, const char *name, float value, int precision, int more){
	serial->put_c('"');
	serial->put_s(name);
	serial->put_c('"');
	serial->put_c(':');
	put_float(serial, value, precision);
	if (more) serial->put_c(',');
}

void json_blockStart(Serial *serial, const char * label){
	serial->put_c('"');
	serial->put_s(label);
	serial->put_s("\":{");
}

void json_blockStartInt(Serial *serial, int label){
	serial->put_c('"');
	put_int(serial, label);
	serial->put_s("\":{");
}

void json_messageStart(Serial *serial){
	serial->put_c('{');
}

void json_messageEnd(Serial *serial){
	json_blockEnd(serial, 0);
}

void json_blockEnd(Serial *serial, int more){
	serial->put_s("}");
	if (more) serial->put_c(',');
}

void json_arrayStart(Serial *serial, const char * name){
	serial->put_c('"');
	serial->put_s(name);
	serial->put_s("\":[");
}

void json_arrayEnd(Serial *serial, int more){
	serial->put_s("]");
	if (more) serial->put_c(',');
}

void json_sendResult(Serial *serial, const char *messageName, int resultCode){
	json_messageStart(serial);
	json_blockStart(serial,messageName);
	json_int(serial, "rc", resultCode, 0);
	json_blockEnd(serial,0);
	json_blockEnd(serial,0);
}

static int dispatch_api(Serial *serial, const char * apiMsgName, const jsmntok_t *apiPayload){

	const api_t * api = apis;
	int res = API_ERROR_UNSPECIFIED;
	while (api->cmd != NULL){
		if (strcmp(api->cmd, apiMsgName) == 0){
			res = api->func(serial, apiPayload);
			if (res != API_SUCCESS_NO_RETURN) json_sendResult(serial, apiMsgName, res);
			break;
		}
		api++;
	}
	if (NULL == api->cmd){
		res = API_ERROR_UNKNOWN_MSG;
		json_sendResult(serial,apiMsgName, res);
	}
	serial->put_s("\r\n");
	return res;
}

static int execute_api(Serial * serial, const jsmntok_t *json){
	const jsmntok_t *root = &json[0];
	if (root->type == JSMN_OBJECT && root->size == 2){
		const jsmntok_t *apiMsgName = &json[1];
		const jsmntok_t *payload = &json[2];
		if (apiMsgName->type == JSMN_STRING){
			jsmn_trimData(apiMsgName);
			return dispatch_api(serial, apiMsgName->data, payload);
		}
		else{
			return API_ERROR_MALFORMED;
		}
	}
	else{
		return API_ERROR_MALFORMED;
	}
}

int process_api(Serial *serial, char *buffer, size_t bufferSize){
	jsmn_init(&g_jsonParser);
	memset(g_json_tok,0,sizeof(g_json_tok));

	int r = jsmn_parse(&g_jsonParser, buffer, g_json_tok, JSON_TOKENS);
	if (r == JSMN_SUCCESS){
		return execute_api(serial, g_json_tok);
	}
	else{
		pr_warning("API Error \r\n");
		return API_ERROR_MALFORMED;
	}
}
