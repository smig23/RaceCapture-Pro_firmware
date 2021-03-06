#include "bluetooth.h"
#include "printk.h"
#include "mod_string.h"
#include "FreeRTOS.h"
#include "task.h"
#include "loggerConfig.h"

#define COMMAND_WAIT 	600

static int readBtWait(DeviceConfig *config, size_t delay) {
	int c = config->serial->get_line_wait(config->buffer, config->length, delay);
	return c;
}

static void flushBt(DeviceConfig *config) {
	config->buffer[0] = '\0';
	config->serial->flush();
}

void putsBt(DeviceConfig *config, const char *data) {
	config->serial->put_s(data);
}

static int sendBtCommandWaitResponse(DeviceConfig *config, const char *cmd, const char *rsp, size_t wait) {
	flushBt(config);
	vTaskDelay(COMMAND_WAIT);
	putsBt(config, cmd);
	readBtWait(config, wait);
	pr_debug("btrsp: ");
	pr_debug(config->buffer);
	pr_debug("\n");
	int res = strncmp(config->buffer, rsp, strlen(rsp));
	pr_debug(res == 0 ? "btMatch\r\n" : "btnomatch\r\n");
	return  res == 0;
}

static int sendBtCommandWait(DeviceConfig *config, const char *cmd, size_t wait) {
	return sendBtCommandWaitResponse(config, cmd, "OK", COMMAND_WAIT);
}

static int sendCommand(DeviceConfig *config, const char * cmd) {
	pr_debug("btcmd: ");
	pr_debug(cmd);
	pr_debug("\r\n");
	return sendBtCommandWait(config, cmd, COMMAND_WAIT);
}

static char * baudConfigCmdForRate(unsigned int baudRate){
	switch (baudRate){
			case 9600:
				return "AT+BAUD4";
				break;
			case 115200:
				return "AT+BAUD8";
				break;
			case 230400:
				return "AT+BAUD9";
				break;
			default:
				pr_error("invalid BT baud");
				pr_error_int(baudRate);
				pr_error("\r\n");
				return "";
				break;
	}
}


static int configureBt(DeviceConfig *config, unsigned int targetBaud, const char * deviceName) {
	if (DEBUG_LEVEL){
		pr_debug("Configuring BT baud Rate");
		pr_debug_int(targetBaud);
		pr_debug("\r\n");
	}
	//set baud rate
	if (!sendCommand(config, baudConfigCmdForRate(targetBaud)))	return -1;
	config->serial->init(8, 0, 1, targetBaud);

	//set Device Name
	char btName[30];
	strcpy(btName, "AT+NAME");
	strcat(btName, deviceName);
	if (DEBUG_LEVEL){
		pr_debug("Configuring BT device name");
		pr_debug(btName);
		pr_debug("\r\n");
	}
	if (!sendBtCommandWaitResponse(config, btName, "OK", COMMAND_WAIT))	return -2;
	return 0;
}

static int bt_probe_config(unsigned int probeBaud, unsigned int targetBaud, const char * deviceName, DeviceConfig *config){
	if (DEBUG_LEVEL){
		pr_debug("Probing BT baud ");
		pr_debug_int(probeBaud);
		pr_debug(": ");
	}
	config->serial->init(8, 0, 1, probeBaud);
	if (sendCommand(config, "AT") && (targetBaud == probeBaud || configureBt(config, targetBaud, deviceName) == 0)){
		pr_debug("provisioning @");
		pr_debug_int(targetBaud);
		pr_debug("success\r\n");
		return DEVICE_INIT_SUCCESS;
	}
	else{
		pr_debug(" fail\r\n");
		return DEVICE_INIT_FAIL;
	}
}

int bt_init_connection(DeviceConfig *config){
	BluetoothConfig *btConfig = &(getWorkingLoggerConfig()->ConnectivityConfigs.bluetoothConfig);
	unsigned int targetBaud = btConfig->baudRate;
	const char *deviceName = btConfig->deviceName;

	if (bt_probe_config(115200, targetBaud, deviceName, config) != 0){
		if (bt_probe_config(9600, targetBaud, deviceName, config) != 0){
			if (bt_probe_config(230400, targetBaud, deviceName, config) !=0){
				pr_debug("failed to provision BT module. assuming already connected.\r\n");
			}
		}
	}
	config->serial->init(8, 0, 1, targetBaud);
	pr_debug("BT device initialized\r\n");
	return DEVICE_INIT_SUCCESS;
}

int bt_check_connection_status(DeviceConfig *config){
	return DEVICE_STATUS_NO_ERROR;
}
