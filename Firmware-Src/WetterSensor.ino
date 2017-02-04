// load AskSin related library's
#include <Arduino.h>
#include <newasksin.h>																// ask sin framework
#include <avr/wdt.h>
#include <00_debug-flag.h>

#include "cmSensor.h"
#include "register.h"																// configuration sheet

//- arduino functions -----------------------------------------------------------------------------------------------------
void setup() {
	wdt_disable();																	// disable Watchdog to avoid endless resets after WDT reset

	// Say hello over UART
	DBG_START(SER, F("Starting sketch for HB-UW-Sen-THPL (" __DATE__ " " __TIME__ ")\n"));
	DBG(SER, F(LIB_VERSION_STRING), F("\n"));

	// - AskSin related ---------------------------------------
	init_millis_timer2();															// init timer2
	hm->init();																		// init the asksin framework

	#if USE_ADRESS_SECTION == 1
		getDataFromAddressSection(HMSerialData, 0, ADDRESS_SECTION_START + 12, 3);	// get hmid from bootloader section
		getDataFromAddressSection(HMSerialData, 3, ADDRESS_SECTION_START + 2, 10);	// get serial number from bootloader section
		getDataFromAddressSection(dev_static,   1, ADDRESS_SECTION_START + 0, 2);	// get device type from bootloader section

		// we must copy HMSerialData to dev_ident every time at start
		memcpy(((uint8_t*)&dev_ident) + 2, HMSerialData, sizeof(dev_ident) - 2);

		#ifdef SER_DBG
			DBG(SER, F("\nGet HMID and HMSR from Bootloader-Area\n"));
			DBG(SER, F("SerialData: "), _HEX(HMSerialData,13), F("\n"));
			DBG(SER, F("Master-ID: "), _HEX(dev_operate.MAID,3), F("\n\n"));
		#endif
	#endif

	// Debug function
	if ( !(PINB & (1<<PINB0)) ) {													// check if config btn pressed at boot
		cbn->button_check.scenario = 2;												// set config button in action mode
		PORTD |= (1<<PD4);															// Status-LED on
		_delay_ms(3000);
		PORTD &= ~(1<<PD4);															// Status-LED off
	}

	pci_ptr = &pci_callback;
	sei();																			// enable interrupts

	DBG(SER, F("Ready.\n\n"));

	#ifndef SER_DBG
		Serial.flush();
		power_usart0_disable();
	#endif

	// - user related -----------------------------------------

	// load register data
	infoConfigChangeChannel0();
}

void loop() {
	// AskSin related
	hm->poll();																		// poll the homematic main loop

//	#ifdef SER_DBG
//		serialEvent();
//	#endif
}

//- user functions --------------------------------------------------------------------------------------------------------

void infoConfigChangeChannel0() {
	// configure low battery level
	uint8_t lowBatVoltage = *cmm[0]->list[0]->ptr_to_val(REG_CHN0_LOW_BAT_LIMIT_TH);
	lowBatVoltage = (lowBatVoltage < BATTERY_MIN_VOLTAGE) ? BATTERY_MIN_VOLTAGE : lowBatVoltage;
	lowBatVoltage = (lowBatVoltage > BATTERY_MAX_VOLTAGE) ? BATTERY_MAX_VOLTAGE : lowBatVoltage;
	// set lowBat threshold
	bat->set(BATTERY_CHECK_INTERVAL, lowBatVoltage);

	// enable / disable LED depend on register
	uint8_t ledMode = (*cmm[0]->list[0]->ptr_to_val(REG_CHN0_LED_MODE) & 0x40);
	led->setIgnoreSendPattern( !ledMode );

	// if burstRx is set ...
	if (*cmm[0]->list[0]->ptr_to_val(REG_CHN0_BURST_RX)) {
		pom->setMode(POWER_MODE_WAKEUP_ONRADIO);									// set mode to wakeup on burst
	} else {	// no burstRx wanted
		pom->setMode(POWER_MODE_WAKEUP_250MS);										// set mode to awake every 250 msecs
	}

	// set max transmit retry
	uint8_t transmDevTryMax = *cmm[0]->list[0]->ptr_to_val(REG_CHN0_TRANS_DEV_TRY_MAX);
	transmDevTryMax = (transmDevTryMax < 1) ? 1 : transmDevTryMax;
	transmDevTryMax = (transmDevTryMax > 10) ? 10 : transmDevTryMax;
	snd_msg.max_retr = transmDevTryMax;

	// set altitude
	int16_t regChnl0Altitude = *cmm[0]->list[0]->ptr_to_val(REG_CHN0_ALTITUDE1) | (*cmm[0]->list[0]->ptr_to_val(REG_CHN0_ALTITUDE0) << 8);
	regChnl0Altitude = (regChnl0Altitude < -500) ? -500 : regChnl0Altitude;
	regChnl0Altitude = (regChnl0Altitude > 10000) ? 10000 : regChnl0Altitude;

	#ifdef SER_DBG
		DBG(SER, F("lowBat: "), lowBatVoltage, F("\n"));
		DBG(SER, F("led: "), ledMode, F("\n"));
		DBG(SER, F("bRx: "), *cmm[0]->list[0]->ptr_to_val(REG_CHN0_BURST_RX), F("\n"));
		DBG(SER, F("transTryMax: "), transmDevTryMax, F("\n"));
		DBG(SER, F("alt: "), regChnl0Altitude, F("\n"));
	#endif
}

#if USE_ADRESS_SECTION == 1
	void getDataFromAddressSection(uint8_t *buffer, uint8_t bufferStartAddress, uint16_t sectionAddress, uint8_t dataLen) {
		for (unsigned char i = 0; i < dataLen; i++) {
			buffer[(i + bufferStartAddress)] = pgm_read_byte(sectionAddress + i);
		}
	}
#endif

uint16_t getAdcValue(uint8_t admux) {
	uint32_t adcValue = 0;

	ADMUX = (admux);
	ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1);							// Enable ADC and set ADC prescaler

	for(int i = 0; i < ADC_MEASURE_COUNT + ADC_MEASURE_DUMMY_COUNT; i++) {
		ADCSRA |= (1 << ADSC);													// start conversion
		while (ADCSRA & (1 << ADSC)) {}											// wait for conversion complete

		if (i >= ADC_MEASURE_DUMMY_COUNT) {										// we discard the first dummy measurements
			adcValue += ADCW;
		}
	}

	ADCSRA &= ~(1 << ADEN);														// ADC disable

	adcValue = adcValue / ADC_MEASURE_COUNT;

	return (uint16_t)adcValue;
}

/**
 * pcInt callback funktion
 *
 * @brief this is the registered callback function for the pin change interrupt.
 * there are 3 parameters as a hand over to identify the pin which has raised the interrupt and if it was a falling or raising edge.
 * <vec> returns the port which had raised the interrupt.
 * <pin> is the byte value of the pin which has raised the interrupt. 1 = pin0, 2 = pin1, 4 = pin2, 8 = pin3, 16 = pin4, etc...
 * <flag> to indentify a falling or raising edge. 0 = falling, value above 0 = raising
 */
void pci_callback(uint8_t vec, uint8_t pin, uint8_t flag) {
	if (vec == 1 && flag == 0) {												// Check PC-INT for PortC, Pin0 Falling Edge (TSL2561 conversion complete)
		tsl2561_pciFlag = 1;
	}

//	DBG(SER, F("v:"), vec, F(", p:"), pin, F(", f:"), flag, F("\n"));
}
