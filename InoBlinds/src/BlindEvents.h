#ifndef BLIND_EVENTS_H
#define BLIND_EVENTS_H
#pragma once

#include "InoTypes.h"
#include "InoEventHandler.h"

#include "BlindDefines.h"

class BlindEvents : public ino::Events
{
public:
  enum {
    BLIND_MOVE_TO       = (EVENT_LAST),
    BLIND_ON_START      = (EVENT_LAST<<1),
    BLIND_ON_STOP       = (EVENT_LAST<<2),
    WIFI_ON_CONNECT     = (EVENT_LAST<<3),
    WIFI_ON_DISCONNECT  = (EVENT_LAST<<4),
    MQTT_ON_CONNECT     = (EVENT_LAST<<5),
    MQTT_ON_DISCONNECT  = (EVENT_LAST<<6),
  };

private:

};

class BlindEventHandler : public ino::EventHandler
{
public:

  BlindEventHandler(
    const ino_size event_queue_size);

  ino_bool pushEventMoveTo(
    const BlindPos pos, const ino_bool wait=false);
  
  ino_bool parseEventMoveTo(
    const Event& event, BlindPos& pos, ino_bool& wait);

  ino_bool pushEventOnStart(
    const BlindPos pos, const BlindDirection direction);
  
  ino_bool pushEventOnStop(
    const BlindPos pos, const BlindDirection direction);

private:
  

};

#endif      /*BLIND_EVENTS_H*/
