#ifndef BLIND_CLOUD_H
#define BLIND_CLOUD_H
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <PubSubClient.h>

#include "BlindDefines.h"
#include "BlindEvents.h"

class BlindCloud {
public:
  BlindCloud(
    BlindEventHandler& event_handler,
    const char* ssid,
    const char* password,
    MQTT_CALLBACK_SIGNATURE);

  bool          init(void);
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

  void          parse_event(
                  const BlindEventHandler::Event& event);

  void          updateTemperature(
                  const ino_u8 pos, const ino_u8 idx, const float temperature);
  void          updateHumidity(
                  const ino_u8 pos, const ino_u8 idx, const float humidity);

  bool          updateBlindPosition(const ino_u8 pos, const bool force=false);

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

  void          logStatus(void);
  
  bool          pushEvent(const BlindEventHandler::Event& event);

  bool          loopDirtySensors(void);

  typedef struct {
    ino_u8        dirty;
    ino_u8        idx;
    ino_float     temperature;
    ino_float     humidity;
  } SensorState;

  BlindEventHandler&            m_event_handler;
  BlindEventHandler::Listener   m_event_listener;
  const char*   m_ssid;
  const char*   m_password;
  MQTT_CALLBACK_SIGNATURE;
  
  ino_u8        m_dirty;
  ino_u8        m_state;
  ino_bool      m_state_dirty;
  ino_u8        m_time_zone;
  ino_u8        m_daylight_saving;
  WiFiUDP       m_ntp;
  ino_timestamp m_ntp_last_reconnect;
  ino_timestamp m_ntp_interval;

  // WiFiClientSecure m_client;
  WiFiClient    m_client;
  String        m_client_name;
  
  PubSubClient  m_mqtt;
  ino_timestamp m_mqtt_last_reconnect;
  ino_timestamp m_mqtt_interval;

  bool          m_epoch_update_flag;

  ino_u8        m_last_pos;

  ino::DateTime m_datetime;

  std::vector<SensorState>     m_sensors;
};

#endif    /*BLIND_CLOUD_H*/
