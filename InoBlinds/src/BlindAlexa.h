#ifndef BLIND_ALEXA_H
#define BLIND_ALEXA_H
#pragma once

#include <Espalexa.h>

#include "BlindDefines.h"
#include "BlindConfig.h"
#include "BlindEvents.h"

#include <Arduino.h>

#ifdef ARDUINO_ARCH_ESP32
#include <WiFi.h>
#include <AsyncTCP.h>
#else
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#endif
#include <ESPAsyncWebServer.h>

//Espalexa espalexa;
//AsyncWebServer server(80);

class BlindEspAlexa {
public:
  BlindEspAlexa(
    BlindEventHandler& event_handler,
    const char* name, const ino_i32 server_port=80);

  ino_bool init(void);
  ino_bool loop(void);

  void parse_event(
    const BlindEventHandler::Event& event);

  void moveTo(
    const uint8_t value);

private:
  ino_bool bootstrap(void);

  BlindEventHandler&            m_event_handler;
  BlindEventHandler::Listener   m_event_listener;

  Espalexa        m_espalexa;
  AsyncWebServer  m_server;
  EspalexaDevice  m_device;
  ino_bool        m_initialized;
};

#endif    /*BLIND_ALEXA_H*/
