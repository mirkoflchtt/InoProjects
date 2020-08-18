#ifndef BLIND_DEFINES_H
#define BLIND_DEFINES_H
#include <Streaming.h>
#include <InoPlatform.h>
#include <stdarg.h>

#include "BlindConfig.h"

#define BLIND_ANALOG_BUTTON_THR_OPEN      (900)
#define BLIND_ANALOG_BUTTON_THR_CLOSE     (700)
#define BLIND_ANALOG_BUTTON_THR_LIGHT1    (500)
#define BLIND_ANALOG_BUTTON_THR_LIGHT2    (400)

#define BLIND_MIN_POSITION \
  (0)
#define BLIND_MAX_POSITION \
  (100)
#define BLIND_MID_POSITION \
  ((BLIND_MIN_POSITION+BLIND_MAX_POSITION)/2)

#define BLIND_GET_UNIT_POSITION(pos_) \
  (((float)(pos_)-BLIND_MIN_POSITION)/(BLIND_MAX_POSITION-BLIND_MIN_POSITION))
#define BLIND_GET_POSITION(unit_) \
  (((float)(unit_))*(BLIND_MAX_POSITION-BLIND_MIN_POSITION))

#define BLIND_OPEN_POSITION \
  (BLIND_MIN_POSITION)
#define BLIND_CLOSE_POSITION \
  (BLIND_MAX_POSITION)
#define BLIND_INVERT_POSITION(pos_) \
  ((BLIND_OPEN_POSITION+BLIND_CLOSE_POSITION)-(pos_))
#define BLIND_VALID_POSITION(pos_) \
  (/*((pos_)>=BLIND_MIN_POSITION) && */((pos_)<=BLIND_MAX_POSITION))

#define BLIND_CMD_IDLE                    (BLIND_MAX_POSITION+1)
#define BLIND_CMD_DISABLE                 (BLIND_MAX_POSITION+2)

#define BLIND_STATE_COMMAND_DEBOUNCE      (600)


//#define LIGHT01_RELAY                   (3) // GPIO3
//#define LIGHT02_RELAY                   (1) // GPIO1

#define NTP_SERVER_HOST                   "pool.ntp.org"
//#define NTP_SERVER_HOST                 "time.nist.gov"
#define NTP_SERVER_PORT                   (2390)

#define MQTT_IN_TOPIC_DOMOTICZ          "domoticz/out"
#define MQTT_OUT_TOPIC_DOMOTICZ         "domoticz/in"

#define MQTT_IN_TOPIC_INO               "ino/messages/out/" MQTT_CLIENT_NAME
#define MQTT_OUT_TOPIC_INO              "ino/messages/in/" MQTT_CLIENT_NAME

#define MQTT_LOG_TOPIC_INO              "ino/log/" MQTT_CLIENT_NAME

typedef ino_u8 BlindPos;

typedef enum {
  BLIND_IDLE    = 0x0,
  BLIND_OPEN,
  BLIND_CLOSE,
} BlindDirection;

#endif    /*BLIND_DEFINES_H*/
