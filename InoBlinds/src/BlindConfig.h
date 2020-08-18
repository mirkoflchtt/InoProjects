#ifndef BLIND_CONFIG_H
#define BLIND_CONFIG_H
#include "BlindConfigPins.h"


#define BLIND_FIRMWARE_VERSION \
  INO_FIRMWARE_SIGNATURE(2020,8,15,0,0,0x0)

#define BLIND_EVENT_QUEUE_SIZE          (32)
#define BLIND_EVENT_POLLING_INTERVAL    (500)

/*
 * Option to enable/disable saving on Esp flash memory current rolling settings 
 */
#define BLIND_CONFIG_FILE \
  "/config/ino/blind.save.config"

/*
 * Option to wipe current rolling settings stored on flash memory 
 */
//#define BLIND_CONFIG_FILE_WIPE

#define BLIND_BAUD_RATE \
  (115200)

/*
 * Option to enable/disable open/close rolling-code wireless remote 
 */
#undef BLIND_REMOTE_BUTTON
#if (defined PIN_REMOTE_OPEN && defined PIN_REMOTE_CLOSE)
#define BLIND_REMOTE_BUTTON
#define BLIND_REMOTE_OPEN                    (PIN_REMOTE_OPEN)
#define BLIND_REMOTE_CLOSE                   (PIN_REMOTE_CLOSE)
#endif

/*
 * Option to enable/disable open/close analog button 
 */
#undef BLIND_ANALOG_BUTTON
#if (defined PIN_ANALOG_BUTTON)
#define BLIND_ANALOG_BUTTON            (PIN_ANALOG_BUTTON)
#endif

/*
 * Option to enable/disable TEMPERATURE SENSOR #1
 */
#undef BLIND_SENSOR1_TEMPERATURE_PIN
#undef BLIND_SENSOR1_TEMPERATURE_INTERVAL
#if (defined PIN_TEMP1)
#define BLIND_SENSOR1_TEMPERATURE_PIN         (PIN_TEMP1)
#define BLIND_SENSOR1_TEMPERATURE_INTERVAL    (5*60)      // in seconds
#endif

/*
 * Option to enable/disable TEMPERATURE SENSOR #2
 */
#undef BLIND_SENSOR2_TEMPERATURE_PIN
#undef BLIND_SENSOR2_TEMPERATURE_INTERVAL
#if (defined PIN_TEMP2)
#define BLIND_SENSOR2_TEMPERATURE_PIN         (PIN_TEMP2)
#define BLIND_SENSOR2_TEMPERATURE_INTERVAL    (5*60)      // in seconds
#endif

/*
 * Option to enable/disable status LED
 */
#undef BLIND_LED
#if (defined PIN_BLIND_LED)
#define BLIND_LED                   (PIN_BLIND_LED)
#define BLIND_LED_SWAP              (PIN_BLIND_LED_SWAP)
#else
#define BLIND_LED                   (-1)   // -1 = disabled
#define BLIND_LED_SWAP              (false)
#endif

//#define BLIND_DISCONNECT_CLOUD

#define BLIND_VERBOSE
//#define BLIND_DEBUGGING

#define BLIND_BUTTON_LONG_PRESS         (500)
#define BLIND_BUTTON_TIMEOUT            (1500)
#define BLIND_BUTTON_DEBOUNCE           (50)

#define DEEP_SLEEP_TIME_SEC             (10)


#define BLIND_WIFI_SSID                 "<My_SSID>"
#define BLIND_WIFI_PWD                  "<My_Password>"

#define OTA_PASSWORD                    (NULL)
#define OTA_PORT                        (8266)
#define OTA_ENABLE_ON_OFF_COUNT         (4)

//#define MQTT_SERVER_HOST              "broker.mqtt-dashboard.com"
#define MQTT_SERVER_HOST                "test.mosquitto.org"
//#define MQTT_SERVER_HOST              IPAddress(192,168,33,1)
#define MQTT_SERVER_PORT                (1883)
#define MQTT_RECONNECT_INTERVAL         (60)

#define BLIND_CLOSE_RELAY               (PIN_CLOSE_RELAY)
#define BLIND_OPEN_RELAY                (PIN_OPEN_RELAY)

#define BLIND_IDLE_TIME                 (10)
#define BLIND_IDLE_TIME_MAX             (10)

#define MQTT_CLIENT_NAME                "TestInoBlindsMQTT"
#define MQTT_SENSOR_IDX_TEMP1           (5)       // Set Domoticz IDX here
#define MQTT_SENSOR_IDX_BLIND           (11)      // Set Domoticz IDX here
#define BLIND_OPEN_TIME_MSEC            (12000)   // in milliseconds
#define BLIND_CLOSE_TIME_MSEC           (10000)   // in milliseconds


#endif    /*BLIND_CONFIG_H*/

