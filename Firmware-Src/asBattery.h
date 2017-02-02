/**
*  AskSin driver implementation
*  2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
* - -----------------------------------------------------------------------------------------------------------------------
* - AskSin battery measurement class --------------------------------------------------------------------------------------
* - -----------------------------------------------------------------------------------------------------------------------
*/

#ifndef _BATTERY_H
#define _BATTERY_H

	#include <as_battery.h>

	class BATTERY : public BAT {
		public:  //----------------------------------------------------------------------------------------------------------------
			BATTERY(uint32_t check_interval, uint8_t tenth_volt, const s_pin_def *ptr_pin_enable, const s_pin_def *ptr_pin_measure, uint8_t z1, uint8_t z2);

		protected:  //-------------------------------------------------------------------------------------------------------------
			const s_pin_def *ptr_enable;
			const s_pin_def *ptr_measure;
			uint8_t res1, res2;
			uint8_t measure_value;													// measured tenth volt battery value

			virtual void do_measure(void);
	};

	extern uint16_t  batteryVoltage;

	extern uint16_t getAdcValue(uint8_t admux);

#endif
