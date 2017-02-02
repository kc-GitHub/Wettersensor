#ifndef _REGISTER_h
	#define _REGISTER_h

	/**
	 * @brief Libraries needed to run AskSin library
	 */
	#include <newasksin.h>

	#include "asBattery.h"
	#include "cmSensor.h"

	#define FIRMWARE_VERSION       0x10											// Version 1.00

	// set to 1 if we should read the address data and device type from bootloader section
	#define USE_ADRESS_SECTION           1
	#define ADDRESS_SECTION_START        0x7FF0									// Start address of data in adress section at and of bootloader

	#define BATTERY_MIN_VOLTAGE          10										// minimal configuable low battery voltage level
	#define BATTERY_MAX_VOLTAGE          50										// maximal configuable low battery voltage level
	#define BATTERY_CHECK_INTERVAL       900000
	#define BATTERY_MEASURE_ENABLE_PIN   &pin_D7
	#define BATTERY_MEASURE_PIN          &pin_C1
	#define BATTERY_MEASURE_R1           47
	#define BATTERY_MEASURE_R2           10

	#define ADC_MEASURE_COUNT            64
	#define ADC_MEASURE_DUMMY_COUNT      10

	uint8_t  tsl2561_pciFlag;													// global var for storing the pin change interrupt for tsl2561
	uint16_t batteryVoltage;													// global var for storing the measured battery voltage in mV
	int16_t  regChnl0Altitude;													// global var altitude set in register

	 // HMID, Serial number, HM-Default-Key, Key-Index
	uint8_t HMSerialData[] = {
		/* HMID */         0x11, 0x12, 0x13,									// dummy HM-ID, can override later, if USE_ADRESS_SECTION is set to 1. @See above
		/* Serial */      'U','W','S','0','0','0','0','0','0','0',				// dummy serial, can override later, if USE_ADRESS_SECTION is set to 1. @See above
		/* Default-Key */ 0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,				// dummy default AES-Key. We don't use AES in this device
		                  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		/* Key-Index */   0xFF													// dummy AES key index
	};

	/**
	 * HM device Settings
	 * firmwareVersion: The firmware version reported by the device
	 *					Sometimes this value is important for select the related device-XML-File
	 *
	 * modelID:			Important for identification of the device.
	 *					@See Device-XML-File /device/supported_types/type/parameter/const_value
	 *
	 * subType:			Identifier if device is a switch or a blind or a remote or other
	 * DevInfo:			Sometimes HM-Config-Files are referring on byte 23 for the amount of channels.
	 *					Other bytes not known.
	 *					23:0 0.4, means first four bit of byte 23 reflecting the amount of channels.
	 */
	uint8_t dev_static[] = {
		/* firmwareVersion 1 byte */  FIRMWARE_VERSION,							// firmware version. @See above.
		/* modelID         2 byte */  0xF1,0x01,								// dummy model-id, can override later, if USE_ADRESS_SECTION is set to 1. @See above
		/* subTypeID       1 byte */  0x70,
		/* deviceInfo      3 byte */  0x01,0x01,0x00,
	};

	// Register to name mapping. See below.
	#define	REG_CHN0_BURST_RX			0x01	// Register 1.0 / 1.0
	#define	REG_CHN0_LED_MODE			0x05	// Register 5.6 / 0.1: bit 6 LED mode (on/off)
	#define	REG_CHN0_AES_ACTIVE			0x08	// Register 8.0 / 0.1: bit 1 AES (on/off)
	#define	REG_CHN0_LOW_BAT_LIMIT_TH	0x12	// Register 18.0 / 1.0: in 1/10 volts
	#define	REG_CHN0_TRANS_DEV_TRY_MAX	0x14	// Register 20.0 / 1.0: 1-10, maxRetryCount
	#define	REG_CHN0_ALTITUDE0			0x24	// Register 36.0 / 2.0: altitude to calculate the air pressure based at see level
	#define	REG_CHN0_ALTITUDE1			0x25

	// Register definition for Channel 0 (maintenance)
	const uint8_t cm_maintenance_ChnlReg[] PROGMEM = {
		REG_CHN0_BURST_RX,			//uint8_t burstRx;         // 0x01,             startBit:0, bits:8
									//uint8_t             :6;  // 0x05              startBit:0, bits:6
		REG_CHN0_LED_MODE,			//uint8_t ledMode     :2;  // 0x05,             startBit:6, bits:2
		REG_CHN0_AES_ACTIVE,		//uint8_t aes active  :1;  // 0x08,             startBit:0, bits:1
		0x0A,						//uint8_t pairCentral[3];  // 0x0A, 0x0B, 0x0C, startBit:0, bits:8 (3 times)
		0x0B,
		0x0C,
		REG_CHN0_LOW_BAT_LIMIT_TH,	//uint8_t lowBatLimit;     // 0x12,             startBit:0, bits:8
		REG_CHN0_TRANS_DEV_TRY_MAX,	//uint8_t transmDevTryMax; // 0x14,             startBit:0, bits:8
		REG_CHN0_ALTITUDE0,			//uint8_t altitude[2];     // 0x24,             startBit:0, bits:8 (2 times)
		REG_CHN0_ALTITUDE1,
	};

	// Default values for Register @See above
	const uint8_t cm_maintenance_ChnlDef[] PROGMEM = {
		0x00,	// burstRx
		0x40,	// ledMode
		0x00,	// aes active
		0x00,	// pairCentral[0]
		0x00,	// pairCentral[1]
		0x00,	// pairCentral[2]
		0x15,	// lowBatLimit
		0x03,	// transmDevTryMax
		0x00,	// altitude[0]
		0x00,	// altitude[1]
	};
	const uint8_t cm_maintenance_ChnlLen = sizeof(cm_maintenance_ChnlReg);

	CM_MASTER *cmm[2] = {
		new CM_MAINTENANCE(0),
		new CM_SENSOR(10),
	};

	// we do not use AES (NO_AES or HAS_AES)
	AES *aes = new NO_AES;

	// Pin configuration for the CC1101 module
	COM *com = new CC1101(&pin_B4, &pin_B3, &pin_B5, &pin_B2, &pin_D2);

	/**
	 * Configure the config button.
	 * - Scenario (0 = no config button, 1 = config at press short, 2 = config at press long)
	 * - config button pin
	 */
	CBN *cbn = new CBN(1, &pin_B0);
//	CBN *cbn = new CBN(2, &pin_B0);

	// Setup status led (red, green)
	LED *led = new LED(&pin_D4, &pin_D4);

	/**
	 * Setup Battery measurement
	 * 9000:   interval in ms (15 * 60 * 1000)
	 * 21:     default battery empty voltage in tenth volt (21 = 2,1V)
	 * pin_D7: enable measure pin
	 * pin_C1: measure pin
	 *         calculation factor of used voltage divider @see above
	 */
	BAT *bat = new BATTERY(
		BATTERY_CHECK_INTERVAL,
		*cmm[0]->list[0]->ptr_to_val(REG_CHN0_LOW_BAT_LIMIT_TH),
		BATTERY_MEASURE_ENABLE_PIN,
		BATTERY_MEASURE_PIN,
		BATTERY_MEASURE_R1,
		BATTERY_MEASURE_R2
	);

	// set default power mode
	POM *pom = new POM(POWER_MODE_NO_SLEEP);

    /**
     * Regular start function
     * This function is called by the main function every time when the device starts,
     * here we can setup everything which is needed for a proper device operation
     */
	void everyTimeStart(void) {
	}

	/**
	 * First time start function
	 * This function is called by the main function on the first boot of a device.
	 * First boot is indicated by a magic byte in the eeprom.
	 * Here we can setup everything which is needed for a proper device operation, like cleaning
	 * of eeprom variables, or setting a default link in the peer table for 2 channels
	 */
	void firstTimeStart(void) {
	}

#endif
