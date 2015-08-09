Firmware for Univarsal Wettersensor
===================================

* http://www.fhemwiki.de/wiki/Universalsensor
* http://forum.fhem.de/index.php/topic,20620.0.html

#Changelog#

##Version Beta 0.15 (Develpopment)##
Improve / Fix debugging
Fixed #16: Pairing with thermostat is stable now

##Version Beta 0.14##
 * Fix millis() overflow bug. Now, the sensor stays in sleep mode after 49 days, so battery should not drained anymore
 * Battery voltage now measured with 1 digit only.

##Version Beta 0.13 (Beta)##
 * Add new register:
   - transmitTryMax: How often should the sensor try to send data without ack.
   - lowBatLimit: Defined the battery voltage used for low battery detection.
   - ledMode: Switch the device LED on or off while sent out data.
   - altitude: Define the sensor-altitude for calculating the air pressure at sea level.
