/**
*  AskSin driver implementation
*  2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
* - -----------------------------------------------------------------------------------------------------------------------
* - AskSin battery measurement class --------------------------------------------------------------------------------------
* - -----------------------------------------------------------------------------------------------------------------------
*/

#include "asBattery.h"
#include <00_debug-flag.h>

#define ADC_REF_VOLTAGE    1100UL
#define ADC_MAX_VAL        1023UL													// max ADC values

//public:  //--------------------------------------------------------------------------------------------------------------
BATTERY::BATTERY(uint32_t check_interval, uint8_t tenth_volt, const s_pin_def *ptr_pin_enable, const s_pin_def *ptr_pin_measure, uint8_t z1, uint8_t z2) {
	interval = check_interval;
	default_value = tenth_volt;

	ptr_enable = ptr_pin_enable;
	ptr_measure = ptr_pin_measure;
	res1 = z1;
	res2 = z2;
	measure_value = 0;
}

//protected:  //-----------------------------------------------------------------------------------------------------------
void BATTERY::do_measure() {
	set_pin_input(ptr_measure);													// set the ADC pin as input

	// enable voltage divider
	set_pin_output(ptr_enable);													// set the enable pin as output
	set_pin_low(ptr_enable);													// and to gnd, while measurement goes from VCC over the resistor network to GND

	uint32_t result = getAdcValue(admux_external | ptr_measure->PINBIT);		// get ADC value

	set_pin_input(ptr_enable);													// disable voltage divider

	result = result * ((uint32_t)ref_v_external * 1000 / ADC_MAX_VAL) / 1000;	// calculate voltage at ADC

	uint16_t fact = (res2 * 1000) / (res1 + res2);								// voltage divider factor
	result = (result * 1000) / fact;											// calculate the real battery voltage in mV

	measure_value = (uint8_t)(result / 100);									// set internal measure_value (in tenth V)
	batteryVoltage = (uint16_t)result;											// set extended battery voltage in mV
}
