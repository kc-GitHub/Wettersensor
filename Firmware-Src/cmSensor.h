#ifndef _CM_SENSOR_h
	#define _CM_SENSOR_h

	#include <cm_master.h>

	#include <Wire.h>															// i2c library, needed for bmp085 or bmp180
	#include <TSL2561.h>
	#include <Sensirion.h>
	#include <BMP085.h>


	// default settings are defined in CM_SENSOR.cpp - updatePeerDefaults
	// List 1
	const uint8_t cm_thsensor_ChnlReg[] PROGMEM = { 0x08, };
	const uint8_t cm_thsensor_ChnlDef[] PROGMEM = { 0x00, };

	// List 4
	const uint8_t cm_thsensor_PeerReg[] PROGMEM = { 0x01,0x02, };
	const uint8_t cm_thsensor_PeerDef[] PROGMEM = { 0x00,0x00, };

	#define SENSOR_ACTION_MEASURE_INIT          0
	#define SENSOR_ACTION_MEASURE_START_WAIT    1
	#define SENSOR_ACTION_MEASURE_LIGHT_TIMEOUT 2
	#define SENSOR_ACTION_MEASURE_LIGHT_READ    3
	#define SENSOR_ACTION_MEASURE_THP_READ      4
	#define SENSOR_ACTION_TRANSMIT_WAIT         5
	#define SENSOR_ACTION_TRANSMIT              6

	#define SENSOR_MAX_MEASURE_TIME           750								// maximum expected time which sensors needed for collecting data
	#define SENSOR_STATUS_LIGHT_MISSING       6553800
	#define SENSOR_STATUS_LIGHT_ERROR         6553700

	class CM_SENSOR : public CM_MASTER {
		private:
			struct s_l1 {
//				uint8_t AES_ACTIVE		: 1;		// 0x08.0, s:1   d: false
//				uint8_t					: 7;		// 0x08.1, s:7   d:
			} *l1;

			struct s_lstPeer {
				uint8_t peerNeedsBurst	:1;			// 0x01, s:0, e:1
				uint8_t					:7;			//
			} *l4;

			uint8_t      sht10Error;
			uint16_t     temperature;
			uint16_t     pressure;
			uint32_t     luminosity;
			uint8_t      nextAction;
			uint8_t      tsl2561InitCount;
			unsigned int tsl2561Data0;
			unsigned int tsl2561Data1;
			waitTimer    sensorTimer;											// delay timer for sensor
			uint32_t     milliMeasureStart;										// save millis at start sensor reading

			Sensirion    sht10;
			BMP085       bmp180;
			TSL2561      tsl2561;

			uint32_t calcSendSlot(void);

		public:  //----------------------------------------------------------------------------------------------------------------
			struct s_sensorValues
			{
				uint16_t	temperature = 0;										//(2 bytes, temperatur + battery state
				uint8_t		humidity = 0;											// 1 byte,  humidity
				uint16_t	pressure = 0;											// 2 bytes  air pressure
				uint32_t	luminosity = 0;											// 4 bytes  luminosity
				uint16_t	batteryVoltage = 0;										// 2 bytes  battery voltage
				uint16_t	reserved1 = 0;											// 2 bytes  reserved
				uint16_t	reserved2 = 0;											// 2 bytes  reserved
			} sensorValues;

			s_cm_status cm_status;												// defined in cmMaster.h, holds current status and set_satatus

			CM_SENSOR(const uint8_t peer_max);									// The constructor

			virtual void   info_config_change(uint8_t channel);
			virtual void   cm_poll(void);										// poll function, driven by HM loop
			virtual void   CONFIG_STATUS_REQUEST(s_m01xx0e *buf);
			virtual void   set_toggle(void);

			inline void    sht10Init(void);
			inline void    sht10Read(void);
			inline void    bmp180Read(void);

			inline void    tsl2561Init();
			inline uint8_t tsl2561MeasureStart();
			inline void    tsl2561Read();

			inline void    transmittData();
	};

	extern uint8_t  tsl2561_pciFlag;
	extern int16_t  regChnl0Altitude;
	extern uint16_t batteryVoltage;

	extern void 	infoConfigChangeChannel0();

#endif
