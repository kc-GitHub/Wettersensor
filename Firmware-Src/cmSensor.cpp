/*
* AskSin channel module for "universalsensor" sensor
*/
#include <Arduino.h>
#include <newasksin.h>															// ask sin framework
#include <as_main.h>
#include <00_debug-flag.h>

#include "cmSensor.h"

/********************************************************************************
 * mandatory functions for every module to communicate within HM protocol stack *
 ********************************************************************************/

/**
 * Constructor for channel module
 */
CM_SENSOR::CM_SENSOR(const uint8_t peer_max) : CM_MASTER(peer_max) {
	// setup the channel list with all dependencies
	lstC.lst = 1;
	lstC.reg = (uint8_t*)cm_thsensor_ChnlReg;
	lstC.def = (uint8_t*)cm_thsensor_ChnlDef;
	lstC.len = sizeof(cm_thsensor_ChnlReg);
	lstC.val = new uint8_t[lstC.len];

	// setup the peer list with all dependencies
	lstP.lst = 4;
	lstP.reg = (uint8_t*)cm_thsensor_PeerReg;
	lstP.def = (uint8_t*)cm_thsensor_PeerDef;
	lstP.len = sizeof(cm_thsensor_PeerReg);
	lstP.val = new uint8_t[lstP.len];

	cm_status.message_type = STA_INFO::NOTHING;										// send the initial status info
	cm_status.message_delay.set(0);

	this->nextAction = SENSOR_ACTION_MEASURE_INIT;
	this->tsl2561InitCount = 0;
}

/**
* @brief Virtual function is called while we received a new list0 or list1
* Herewith we can adapt changes given by the config change. Needs to be overwritten
* by the respective channel module
*/
void CM_SENSOR::info_config_change(uint8_t channel) {
	if (channel == 0) {
		infoConfigChangeChannel0();
	}
}

/**
 *
 */
void CM_SENSOR::cm_poll(void) {
	uint32_t measurementTime;

	process_send_status_poll(&cm_status, lstC.cnl);									// check if there is some status to send, function call in cmMaster.cpp

	// Check if tsl2561 generated an PinChangeInterrupt
	if (tsl2561_pciFlag) {
		if (this->nextAction == SENSOR_ACTION_MEASURE_LIGHT_WAIT) {
			this->nextAction = SENSOR_ACTION_MEASURE_LIGHT_READ;
			this->sensorTimer.set(0);													// set sensor timer to done
		}

		this->tsl2561.clearInterrupt();												// reset the interrupt line
		tsl2561_pciFlag = 0;														// clear interrupt  flag
	}

	if (!this->sensorTimer.done()) return;											// step out while timer is still running

//	DBG(SER, F("nA: "), nextAction, _TIME, F(", "), (get_millis() - this->tmpMilli), F("\n"));

	if (this->nextAction == SENSOR_ACTION_MEASURE_INIT) {							// 0: initialize sensors if needed
		this->sht10Init();															// initialize the SHT10
		this->tsl2561Init();														// initialize the TSL2561
		this->nextAction = SENSOR_ACTION_MEASURE_START_WAIT;						// initialize done we can activate measuring

	} else if (this->nextAction == SENSOR_ACTION_MEASURE_START_WAIT) {				// 1: start measuring
		this->milliMeasureStart = get_millis();

		if (this->tsl2561MeasureStart()){											// check if sensor available
			this->nextAction = SENSOR_ACTION_MEASURE_LIGHT_WAIT;
			this->sensorTimer.set(850);												// set timeout for tsl2561 measurement

		} else {																	// no tsl2561 found.
			this->luminosity = SENSOR_STATUS_LIGHT_MISSING;
			this->nextAction = SENSOR_ACTION_MEASURE_THP_READ;
		}

	} else if (this->nextAction == SENSOR_ACTION_MEASURE_LIGHT_WAIT) {				// 2: tsl2561 timeout
		this->luminosity = SENSOR_STATUS_LIGHT_ERROR;
		this->nextAction = SENSOR_ACTION_MEASURE_THP_READ;

	} else if (this->nextAction == SENSOR_ACTION_MEASURE_LIGHT_READ) {				// 3: Read tsl2561 value
		if (!this->tsl2561Read()) {
			this->nextAction = SENSOR_ACTION_MEASURE_START_WAIT;					// we measure again
		} else {
			this->nextAction = SENSOR_ACTION_MEASURE_THP_READ;
		}

	} else if (this->nextAction == SENSOR_ACTION_MEASURE_THP_READ) {				// 4: Read SHT10 and BMP180
		this->sht10Read();
		this->bmp180Read();
		this->nextAction = SENSOR_ACTION_TRANSMIT_WAIT;

	} else if (this->nextAction == SENSOR_ACTION_TRANSMIT_WAIT) {					// 5: wait time left for send slot
		measurementTime = get_millis() - this->milliMeasureStart;
		measurementTime = (measurementTime < SENSOR_MAX_MEASURE_TIME) ? measurementTime : 0;
		this->sensorTimer.set(SENSOR_MAX_MEASURE_TIME - measurementTime);		// set corrected timer to next send slot
		this->nextAction = SENSOR_ACTION_TRANSMIT;

	} else if (this->nextAction == SENSOR_ACTION_TRANSMIT) {						// 6: send data
		this->sensorTimer.set(calcSendSlot() - SENSOR_MAX_MEASURE_TIME);			// set a new measurement time
		this->nextAction = SENSOR_ACTION_MEASURE_START_WAIT;						// start state machine again
		this->transmittData();
	}
}

/**
 * @brief Received message handling forwarded by AS::processMessage
 */

void CM_SENSOR::CONFIG_STATUS_REQUEST(s_m01xx0e *buf) {
//	cm_status.message_type = STA_INFO::SND_ACTUATOR_STATUS;							// send next time a info status message
//	cm_status.message_delay.set(50);												// wait a short time to set status
//	DBG(SER, F("TH:CONFIG_STATUS_REQUEST\n"));
}

/**
 *
 */
void CM_SENSOR::set_toggle(void) {
	if (this->nextAction == SENSOR_ACTION_MEASURE_START_WAIT && this->sensorTimer.remain() > 2000) {
		this->sensorTimer.set(0);													// arm sensor timer, so we start measuring immediately
	}
}




/****************************
 * sensor related functions *
 ****************************/

/**
 * @brief Calculate the next send slot
 */
inline uint32_t CM_SENSOR::calcSendSlot(void) {
	uint8_t hmId[4];
	hmId[0] = dev_ident.HMID[2];
	hmId[1] = dev_ident.HMID[1];
	hmId[2] = dev_ident.HMID[0];
	hmId[3] = 0;


	uint32_t result = ((( *(uint32_t*)&hmId << 8) | (snd_msg.mBody.MSG_CNT)) * 1103515245 + 12345) >> 16;
	result = ((result & 0xFF) + 480) * 250;
	result+= 1000;

	return result;
}

inline void CM_SENSOR::transmittData() {
	// Temperature
	((uint8_t *)&(this->sensorValues.temperature))[0] = ((this->temperature >> 8) & 0x7F);	// battery status is added later
	((uint8_t *)&(this->sensorValues.temperature))[1] = this->temperature & 0xFF;

	// Air pressure transmitted in little-endian format and must converted
	((uint8_t *)&(this->sensorValues.pressure))[0] = this->pressure >> 8;
	((uint8_t *)&(this->sensorValues.pressure))[1] = this->pressure & 0xFF;

	// Battery voltage transmitted in little-endian format and must converted
	((uint8_t *)&(this->sensorValues.batteryVoltage))[0] = batteryVoltage >> 8;
	((uint8_t *)&(this->sensorValues.batteryVoltage))[1] = batteryVoltage & 0xFF;

	// Luminosity
	((uint8_t *)&(this->sensorValues.luminosity))[0] = (this->luminosity >> 24) & 0xFFFFFF;
	this->luminosity = this->luminosity & 0xFFFFFF;
	((uint8_t *)&(this->sensorValues.luminosity))[1] = (this->luminosity >> 16) & 0xFFFF;
	this->luminosity = this->luminosity & 0xFFFF;
	((uint8_t *)&(this->sensorValues.luminosity))[2] = (this->luminosity >> 8) & 0xFF;
	((uint8_t *)&(this->sensorValues.luminosity))[3] = this->luminosity & 0xFF;

	/*
	DBG(SER, F("Temp: "), this->temperature, F("\n"));
	DBG(SER, F("Hum: "), this->sensorValues.humidity, F("\n"));
	DBG(SER, F("Press: "), this->pressure, F("\n"));
	DBG(SER, F("Bat: "), batteryVoltage, F("\n"));
	DBG(SER, F("Lumi: "), this->luminosity, F("\n"));
	*/

	hm->send_WEATHER_EVENT(
		this, (uint8_t *)&this->sensorValues, sizeof(this->sensorValues)			// prepare the message and send, burst if burstRx register is set
	);
}

/**
 * Initialize the SHT10
 */
inline void CM_SENSOR::sht10Init(void) {
	this->sht10.config(A4, A5);														// configure the sensor
	this->sht10.writeSR(LOW_RES);													// low resolution is enough
}

/**
 * get the temperature from SHT10
 */
inline void CM_SENSOR::sht10Read(void) {
	uint16_t rawData;

	TWCR = 0;																		// Disable I2C

	this->sht10Error = this->sht10.measTemp(&rawData);
	// Measure temperature and humidity from Sensor only if no error
	if (!this->sht10Error) {
		float temp = this->sht10.calcTemp(rawData);
		this->temperature = temp * 10;

		this->sht10.measHumi(&rawData);
		this->sensorValues.humidity = this->sht10.calcHumi(rawData, temp);
	}
}

/**
 * get the air pressure and temperature from BMP180
 */
inline void CM_SENSOR::bmp180Read(void) {
	this->bmp180.begin();

	// simple barometric formula
	this->pressure = (uint16_t)((this->bmp180.readPressure() / 10) + (regChnl0Altitude / 0.85));

	if (this->pressure > 300) {
		// get temperature from bmp180 if sht10 no present
		if (this->sht10Error) {
			this->temperature = this->bmp180.readTemperature() * 10;				// read temperature from BMP180
		}
	} else {
		this->pressure = 0;
	}
}

/*
 * Initialize the TSL2561 Sensor
 */
inline void CM_SENSOR::tsl2561Init() {
	this->tsl2561.begin(TSL2561_ADDR_0);

	// Check if tsl2561 available
	if (this->tsl2561.setPowerUp()) {
		this->tsl2561.clearInterrupt();
		register_PCINT(&pin_C0);
	}
}

inline uint8_t CM_SENSOR::tsl2561MeasureStart() {
//	DBG(SER, F("Ttsl2561MeasureStart\n"));

	// check if TSL2561 available
	uint8_t initOk = this->tsl2561.setPowerUp();
	if (initOk) {
		this->tsl2561.setInterruptControl(
			TSL2561_INTERRUPT_CONTROL_LEVEL, TSL2561_INTERRUPT_PSELECT_EVERY_ADC
		);

		uint8_t gain = 0;
		uint8_t integrationTime = INTEGATION_TIME_14;

		if (this->tsl2561InitCount == 0) {
			// first set to low sensitive
			this->tsl2561Data0 = 0;
			this->tsl2561Data1 = 0;

		} else {
			if ((this->tsl2561Data0 < 1000) && (this->tsl2561Data1 < 1000)) {
				gain = ((this->tsl2561Data0 < 100) && (this->tsl2561Data1 < 100)) ? true : false;
				integrationTime = INTEGATION_TIME_402;
			} else {
				integrationTime = INTEGATION_TIME_101;
			}
		}

		this->tsl2561.setTiming(gain, integrationTime);
	}

	return initOk;
}

inline uint8_t CM_SENSOR::tsl2561Read() {
	uint8_t result;
	// read light data
	this->tsl2561.getData(this->tsl2561Data0, this->tsl2561Data1);

	if (this->tsl2561InitCount == 0 && this->tsl2561Data0 < 2000 && this->tsl2561Data1 < 2000) {
		this->tsl2561InitCount++;
		result = 0;

	} else {
		double lux = 0;
		boolean luxValid = this->tsl2561.getLux(this->tsl2561Data0, this->tsl2561Data1, lux);
		this->luminosity = ((luxValid) ? lux : 65535) * 100;
		result = 1;
	}

	this->tsl2561.setPowerDown();

	return result;
}
