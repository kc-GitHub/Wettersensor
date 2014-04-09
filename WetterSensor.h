#if defined(ARDUINO) && ARDUINO >= 100
	#include "Arduino.h"
#else
	#include "WProgram.h"
#endif

#include <Wire.h>																// i2c library, needed for bmp085 or bmp180

#define BATTERY_MEASSUREMENT_FACTOR 0.164										// Faktor Spannungsteiler externe Batteriemessung
#define BATTERY_MIN_VOLTAGE         22											// Faktor Spannungsteiler externe Batteriemessung

void HM_Reset_Cmd(uint8_t *data, uint8_t len);
void sendPairing();
void resetDevice();
void sendCmdStr();
void buttonSend();
void printConfig();
