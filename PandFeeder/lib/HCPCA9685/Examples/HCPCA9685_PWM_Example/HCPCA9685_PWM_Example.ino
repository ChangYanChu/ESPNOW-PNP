/* FILE:    HCPCA9685_PWM_Example
   DATE:    10/06/16
   VERSION: 0.1
   AUTHOR:  Andrew Davies

   Sketch created by Hobby Components Ltd (HOBBYCOMPONENTS.COM)
   
10/06/16 version 0.1: Original version

This example demonstrates how to use the HCPCA9685 library together with the PCA9685
to set the frequency and ON/OFF times of one of the 16 PWM outputs on the PCA9685.
The example has been written particularly for the 16 Channel 12-bit PWM Servo Motor 
Driver Module (HCMODU0097) available from hobbycomponents.com 

To use the module connect it to your Arduino as follows:

PCA9685...........Uno/Nano
GND...............GND
OE................N/A
SCL...............A5
SDA...............A4
VCC...............5V


External 5V Power for the PWM output pins can be supplied by the V+ and GND input of 
the screw terminal block or via the V+ and GND pins on the HCMODU0097 module header.


PLEASE NOTE: At 5V, each PWM pin has a maximum 25 mA current sink capability and a 
10 mA source capability. You should therefore not use these pins to directly drive 
high current devices such as a DC motor. For LEDs a sufficient current limiting 
resistor should be provided.

You may copy, alter and reuse this code in any way you like, but please leave
reference to HobbyComponents.com in your comments if you redistribute this code.
This software may not be used directly for the purpose of selling products that
directly compete with Hobby Components Ltd's own range of products.

THIS SOFTWARE IS PROVIDED "AS IS". HOBBY COMPONENTS MAKES NO WARRANTIES, WHETHER
EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE, ACCURACY OR LACK OF NEGLIGENCE.
HOBBY COMPONENTS SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR ANY DAMAGES,
INCLUDING, BUT NOT LIMITED TO, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES FOR ANY
REASON WHATSOEVER.
*/



/* Include the HCPCA9685 library */
#include "HCPCA9685.h"

/* I2C slave address for the device/module. For the HCMODU0097 the default I2C address
   is 0x40 */
#define  I2CAdd 0x40


/* Create an instance of the library */
HCPCA9685 HCPCA9685(I2CAdd);


void setup() 
{
  /* Initialise the library */ 
  HCPCA9685.Init();

  /* Wake the device up */
  HCPCA9685.Sleep(false);
}


void loop()
{
  /* Set the period frequency to 100Hz. The period frequency is the frequency of one 
     full high/low cycle of the PWM output. The frequency can be set anywhere between
   24 (24Hz) to 1526 (1.526KHz) */
  HCPCA9685.SetPeriodFreq(100);

  /* Set the ON and OFF times for PWM output 0. This function takes 3 parameters,
     the first is the PWM output pin to control (0 to 15), the second sets the point 
   at which to pull the output high (0 to 4095), the third sets the point at which to
     pull the pin low again (0 to 4095). With the values below the output will 
   immediately go high at the start of a cycle and then go low half way though the 
   cycle giving a 50% duty cycle. */
  HCPCA9685.Output(0, 0, 2047);
}

