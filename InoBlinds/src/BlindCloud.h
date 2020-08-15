#ifndef BLIND_CLOUD_H
#define BLIND_CLOUD_H
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <PubSubClient.h>

#include "BlindDefines.h"

class BlindCloud {
public:
  BlindCloud(const char* ssid, const char* password, MQTT_CALLBACK_SIGNATURE);

  bool          reset(void);

  void          setTimeZone(const uint8_t time_zone);
  void          setDayLightSaving(const uint8_t daylight_saving);
  
  bool          connect(const bool wait=false);
  bool          disconnect(void);
  bool          connected(void);
  
  void          setCallback(MQTT_CALLBACK_SIGNATURE);

  /* if epoch==0 it means to negotiate new epoch with NTP server */
  void          updateDateTime(const ino::datetime_ts epoch_ms);
  
  bool          loop(void);

  bool          updateTemperature(const uint8_t idx, const float temperature, const float humidity);
  bool          updateBlindPosition(const uint8_t pos, const bool force=false);

  ino::DateTime& dateTime(void) { return m_datetime; }
  
private:
  bool          try_to_connect(const bool wait);

  bool          checkWiFiConnection(void);
  
  bool          mqttReconnect(const bool wait);
  bool          mqttDisconnect(void);
  bool          mqttSendBlindPosition(const uint8_t pos);
  bool          mqttPublishMessage(const char* msg);
  
  bool          sendNtpPacket(void);
  bool          receiveNtpPacket(void); 

  void          logStatus(const bool ok);
  
  const char*   m_ssid;
  const char*   m_password;
  MQTT_CALLBACK_SIGNATURE;
  
  uint8_t       m_state;
  uint8_t       m_time_zone;
  uint8_t       m_daylight_saving;
  WiFiUDP       m_udp;
  // WiFiClientSecure m_client;
  WiFiClient    m_client;
  String        m_client_name;
  
  PubSubClient  m_mqtt;
  ino_timestamp m_mqtt_last_reconnect;
  ino_timestamp m_mqtt_interval;

  bool          m_epoch_update_flag;

  uint8_t       m_last_pos;

  ino::DateTime m_datetime;
};

#endif    /*BLIND_CLOUD_H*/
