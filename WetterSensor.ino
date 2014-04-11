//#define SER_DBG

//- load library's --------------------------------------------------------------------------------------------------------
#include "WetterSensor.h"
#include "Register.h"															// configuration sheet
#include <Buttons.h>															// remote buttons library
#include <Sensor_SHT10_BMP085_TSL2561.h>

#ifdef SER_DBG

//- serial communication --------------------------------------------------------------------------------------------------
const uint8_t helptext1[] PROGMEM = {													// help text for serial console
	"\n"
	"Available commands:" "\n"
	"  p                - start pairing with master" "\n"
	"  r                - reset device" "\n"
	"  b[0]  b[n]  s    - send a string, b[0] is length (50 bytes max)" "\n"
	"\n"
	"  c                - print configuration" "\n"
	"\n"
	"  $nn for HEX input (e.g. $AB,$AC ); b[] = byte, i[]. = integer " "\n"
};

const InputParser::Commands cmdTab[] PROGMEM = {
	{ 'p', 0, sendPairing },
	{ 'r', 0, resetDevice },
	{ 's', 1, sendCmdStr },
	{ 'b', 1, buttonSend },
	{ 'c', 0, printConfig },
	{ 0 }
};
InputParser parser (50, cmdTab);

#endif

// homematic communication
HM::s_jumptable jTbl[] = {														// jump table for HM communication
	// byte3, byte10, byte11, function to call									// 0xff means - any byte
	{  0x11,  0x04,  0x00,    HM_Reset_Cmd },
	{ 0x00 }
};

Buttons button[1];																// declare remote button object

Sensirion sht10;
BMP085 bmp085;
TSL2561 tsl2561;

Sensors_SHT10_BMP085_TSL2561 sensTHPL;

uint16_t ledCount;

// main functions
void setup() {

	// We disable the Watchdog first

	#ifdef SER_DBG
	Serial.begin(57600);																// serial setup
	Serial << F("Starting sketch...\n");												// ...and some information
	Serial << pCharPGM(helptext1) << '\n';
	Serial << F("freeMem: ") << freeMem() << F(" byte") <<'\n';
#endif	

	hm.cc.config(10,11,12,13,2,0);												// CS, MOSI, MISO, SCK, GDO0, Interrupt

	hm.statusLed.config(4, 4);													// configure the status led pin
	hm.statusLed.set(STATUSLED_BOTH, STATUSLED_MODE_BLINKFAST, 3);

	// setup battery measurement
	hm.battery.config(
		BATTERY_MODE_EXTERNAL_MESSUREMENT, 7, 1, BATTERY_MEASSUREMENT_FACTOR, 10000
	);
	hm.battery.setMinVoltage(BATTERY_MIN_VOLTAGE);

	hm.setPowerMode(3);															// power mode for HM device
	hm.init();																	// initialize the hm module

	button[0].regInHM(0, &hm);													// register buttons in HM per channel, handover HM class pointer
	button[0].config(8, NULL);													// configure button on specific pin and handover a function pointer to the main sketch

	sensTHPL.regInHM(1, &hm);													// register sensor class in hm
	sensTHPL.config(A4, A5, 0, &sht10, &bmp085, &tsl2561);						// data pin, clock pin and timing - 0 means HM calculated timing, every number above will taken in milliseconds

	byte rr = MCUSR;
	MCUSR =0;

	sensTHPL.setResetReason(rr);
}

void loop() {
#ifdef SER_DBG
	parser.poll();																		// handle serial input from console
#endif
	hm.poll();																	// poll the HM communication

/*
	// debug led blinking, disabled led.poll before
	ledCount++;
	if (ledCount == 5000) {
		digitalWrite(4, 1);
		hm.stayAwake(50);

//		hm.statusLed.on(STATUSLED_BOTH);
	} else if (ledCount == 5100) {
		digitalWrite(4, 0);
		ledCount = 0;
//		hm.statusLed.off(STATUSLED_BOTH);
	}
*/
}

//- HM functions ----------------------------------------------------------------------------------------------------------
void HM_Status_Request(uint8_t *data, uint8_t len) {
	//	Serial << F("status request, data: ") << pHex(data,len) << '\n';
}

void HM_Reset_Cmd(uint8_t *data, uint8_t len) {
	//	Serial << F("reset, data: ") << pHex(data,len) << '\n';
	hm.send_ACK();																// send an ACK
	if (data[0] == 0) {
		hm.reset();																// do a reset only if channel is 0
	}
}

void HM_Config_Changed(uint8_t *data, uint8_t len) {
	//	Serial << F("config changed, data: ") << pHex(data,len) << '\n';
}

void HM_Remote_Event(uint8_t *data, uint8_t len) {
	//	Serial << F("remote event, data: ") << pHex(data,len) << '\n';
}

// config functions
#ifdef SER_DBG
void sendPairing() {																	// send the first pairing request
	hm.startPairing();
}
void resetDevice() {
	Serial << F("reset device, clear eeprom...\n");
	hm.reset();
	Serial << F("reset done\n");
}
void sendCmdStr() {																		// reads a sndStr from console and put it in the send queue
	memcpy(hm.send.data,parser.buffer,parser.count());									// take over the parsed byte data
	Serial << F("s: ") << pHexL(hm.send.data, hm.send.data[0]+1) << '\n';				// some debug string
	hm.send_out();																		// fire to send routine
}
void buttonSend() {
	uint8_t cnl, lpr;
	parser >> cnl >> lpr;
	
	Serial << "button press, cnl: " << cnl << ", long press: " << lpr << '\n';			// some debug message
	hm.sendPeerREMOTE(cnl,lpr,0);														// parameter: button/channel, long press, battery
}
void printConfig() {
	hm.printConfig();
}
#endif





