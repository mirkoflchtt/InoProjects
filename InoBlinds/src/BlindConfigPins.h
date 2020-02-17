#ifndef BLIND_CONFIG_PINS_H
#define BLIND_CONFIG_PINS_H

#define ESP12_STANDALONE            (0)
#define WEMOSD1_5V_RELE             (1)
#define WEMOSD1_SOLID_STATE_RELE    (2)

//#define PIN_CONFIGURATION           (WEMOSD1_5V_RELE)
#define PIN_CONFIGURATION           (WEMOSD1_SOLID_STATE_RELE)

/*
  * Wemos D1 Mini Pinout
  * 
  * tx = GPIO01 (pwm)                           rst
  * rx = GPIO03 (pwm)                           A0 = A0
  * D1 = GPIO05 (pwm-SCL)                       D0 = GPIO16 (wake)
  * D2 = GPIO04 (pwm-SDA)                       D5 = GPIO14 (pwm-CLK-SCK)
  * D3 = GPIO00 (pwm-10k pullup)                D6 = GPIO12 (pwm-MISO)
  * D4 = GPIO02 (pwm-builtin_led-tx1-pullup)    D7 = GPIO13 (pwm-MOSI-rx2)
  * GND                                         D8 = GPIO15 (pwm-CS-tx2-10k pulldown)
  * 5V (USb)                                    3v3
 */

#if (PIN_CONFIGURATION==ESP12_STANDALONE)

// This is the pin-out configuration for the ESP12-E standalone module
#define PIN_ANALOG_BUTTON  (A0)

#define PIN_BLIND_LED      (0)
#define PIN_BLIND_LED_SWAP (false)

#define PIN_CLOSE_RELAY    (2)
#define PIN_OPEN_RELAY     (16)
#define PIN_SWAP_RELAY     (true)

//#define PIN_TEMP2          (4)
#define PIN_TEMP1          (5)

//#define PIN_REMOTE_OPEN    (14)   /*D5*/
//#define PIN_REMOTE_CLOSE   (12)   /*D6*/

#elif (PIN_CONFIGURATION==WEMOSD1_5V_RELE)

// This is the pin-out configuration for the Wemos-D1-Mini Module
#define PIN_ANALOG_BUTTON  (A0) 

#define PIN_BLIND_LED      (16)     /*D0*/
#define PIN_BLIND_LED_SWAP (false)

#define PIN_TEMP1          (2)      /*D4*/
//#define PIN_TEMP2          (0)    /*D3*/
#define PIN_CLOSE_RELAY    (5)      /*D1 - SCL*/  
#define PIN_OPEN_RELAY     (4)      /*D2 - SDA*/
#define PIN_SWAP_RELAY     (true)

#define PIN_REMOTE_OPEN    (14)     /*D5*/
#define PIN_REMOTE_CLOSE   (12)     /*D6*/

#elif (PIN_CONFIGURATION==WEMOSD1_SOLID_STATE_RELE)

// This is the pin-out configuration for the Wemos-D1-Mini Module
#define PIN_ANALOG_BUTTON  (A0) 

#define PIN_BLIND_LED      (16)     /*D0*/
#define PIN_BLIND_LED_SWAP (true)

#define PIN_TEMP1          (2)      /*D4*/
//#define PIN_TEMP2          (0)    /*D3*/

#define PIN_CLOSE_RELAY    (15)     /*D8*/  
#define PIN_OPEN_RELAY     (13)     /*D7*/
#define PIN_SWAP_RELAY     (false)

#define PIN_REMOTE_OPEN    (14)     /*D5*/
#define PIN_REMOTE_CLOSE   (12)     /*D6*/

#endif    /*PIN_CONFIGURATION*/

#endif    /*BLIND_CONFIG_PINS_H*/
