#include <time.h>
#include "BlindCloud.h"

#define DAY_LIGHT_SAVING  (0)
#define TIME_ZONE         (0)

/*
domoticz/out {
   "Battery" : 255,
   "RSSI" : 12,
   "dtype" : "Light/Switch",
   "id" : "00000000",
   "idx" : 2,
   "name" : "Studio-Tapparella",
   "nvalue" : 2,
   "stype" : "Selector Switch",
   "svalue1" : "66",
   "switchType" : "Blinds Percentage Inverted",
   "unit" : 1
}

domoticz/out {
   "Battery" : 255,
   "RSSI" : 12,
   "dtype" : "Light/Switch",
   "id" : "00000001",
   "idx" : 3,
   "name" : "Studio-Tapparella-02",
   "nvalue" : 2,
   "stype" : "Selector Switch",
   "svalue1" : "47",
   "switchType" : "Blinds Percentage",
   "unit" : 1
}

set to 81%
domoticz/out {
   "Battery" : 255,
   "RSSI" : 12,
   "dtype" : "Light/Switch",
   "id" : "00000001",
   "idx" : 3,
   "name" : "Studio-Tapparella-02",
   "nvalue" : 2,
   "stype" : "Selector Switch",
   "svalue1" : "81",
   "switchType" : "Blinds Percentage",
   "unit" : 1
}

Switching OFF
domoticz/out {
   "Battery" : 255,
   "RSSI" : 12,
   "dtype" : "Light/Switch",
   "id" : "00000001",
   "idx" : 3,
   "name" : "Studio-Tapparella-02",
   "nvalue" : 1,
   "stype" : "Selector Switch",
   "svalue1" : "0",
   "switchType" : "Blinds Percentage",
   "unit" : 1
}

Switching ON

*/

#define CLOUD_IS(flag_) \
  ( (flag_)==((m_state)&(flag_)) )

enum {
  STATE_NONE            = (0x0U),
  STATE_ON              = (0x1U<<0),
  STATE_INIT            = (0x1U<<1),
  STATE_WIFI_CONNECTED  = (0x1U<<2),
  STATE_MQTT_CONNECTED  = (0x1U<<3),
  STATE_NTP_CONNECTED   = (0x1U<<4),
};

#define DIRTY_SENSORS       (0x1U)

INO_STATIC
void listenerDefault(
  const BlindEventHandler::Event& event,
  ino_handle cookie)
{
  INO_ASSERT(cookie)
  ((BlindCloud*)cookie)->parse_event(event);
}

/* Create MQTT message payload for temperature sensor */
bool createMQTTPayloadTemperature(
  char* msg, const ino_u8 idx,
  const float temp, const float hum=0) 
{
  String dataMsg = "{";

  dataMsg.concat(F("\"idx\":"));
  dataMsg.concat(idx);
  dataMsg.concat(F(",\"nvalue\":0"));

  dataMsg.concat(F(",\"svalue\":\""));
  dataMsg.concat(temp);
  if ( hum!=0 ) {
    dataMsg.concat(F(";"));
    dataMsg.concat(hum);
    dataMsg.concat(F(";0"));
  }
  dataMsg.concat("\"}");

  dataMsg.toCharArray(msg, dataMsg.length()+1);
  
  return (dataMsg.length()>0);
}

/* Create MQTT message payload for temperature sensor */
bool createMQTTPayloadBlindPosition(
  char* msg, const ino_u8 idx,
  const ino_u8 pos) 
{
  String dataMsg = "{";

  dataMsg.concat(F("\"idx\":"));
  dataMsg.concat(idx);
  dataMsg.concat(F(",\"nvalue\":2"));
  dataMsg.concat(F(",\"svalue\":\""));
  dataMsg.concat(BLIND_INVERT_POSITION(pos));
  dataMsg.concat("\"}");
  
  dataMsg.toCharArray(msg, dataMsg.length()+1);
  
  return (dataMsg.length()>0);
  
}

/* Create MQTT message payload for temperature sensor */
bool createMQTTPayloadEpochTime(
  char* msg,
  const ino::datetime_ts epoch_ms) 
{
  String dataMsg = "{";
  dataMsg.concat(F("\"epoch\":"));
  dataMsg.concat((unsigned long)(epoch_ms / 1000));
  dataMsg.concat(F("."));
  dataMsg.concat((unsigned long)(epoch_ms % 1000));
  dataMsg.concat("\"}");
  
  dataMsg.toCharArray(msg, dataMsg.length()+1);
  
  return (dataMsg.length()>0);
}

bool BlindCloud::checkWiFiConnection(void)
{
  /*
    WL_NO_SHIELD        = 255,   // for compatibility with WiFi Shield library
    WL_IDLE_STATUS      = 0,
    WL_NO_SSID_AVAIL    = 1,
    WL_SCAN_COMPLETED   = 2,
    WL_CONNECTED        = 3,
    WL_CONNECT_FAILED   = 4,
    WL_CONNECTION_LOST  = 5,
    WL_DISCONNECTED     = 6
  */
  const bool ok = (WiFi.status()==WL_CONNECTED);
  // const bool ok = ( WiFi.waitForConnectResult()==WL_CONNECTED );

  /* Mark flag as connected or not.. */
  if (ok) {
    if (!CLOUD_IS(STATE_WIFI_CONNECTED)) {
      pushEvent(BlindEventHandler::Event(BlindEvents::WIFI_ON_CONNECT));
      m_state |= STATE_WIFI_CONNECTED;
      m_state_dirty = true;
    }
  } else {
    if (CLOUD_IS(STATE_WIFI_CONNECTED)) {
      pushEvent(BlindEventHandler::Event(BlindEvents::WIFI_ON_DISCONNECT));
      m_state &= ~STATE_WIFI_CONNECTED;
      m_state_dirty = true;
    }
  }
  return ok;
}


BlindCloud::BlindCloud(
  BlindEventHandler& event_handler,
  const char* ssid,
  const char* password,
  MQTT_CALLBACK_SIGNATURE) :
m_event_handler(event_handler),
m_event_listener(
    BlindEvents::BLIND_ON_START|BlindEvents::BLIND_ON_STOP|
    BlindEvents::GOT_TEMPERATURE|BlindEvents::GOT_HUMIDITY,
    listenerDefault, this),
m_ssid(ssid),
m_password(password),
m_dirty(0x0),
m_state(STATE_NONE),
m_state_dirty(false),
m_time_zone(TIME_ZONE),
m_daylight_saving(DAY_LIGHT_SAVING),
m_ntp(),
m_ntp_last_reconnect(0),
m_ntp_interval(NTP_RECONNECT_INTERVAL*1000),
m_client(),
m_client_name(MQTT_CLIENT_NAME),
m_mqtt(MQTT_SERVER_HOST, MQTT_SERVER_PORT, m_client),
m_mqtt_last_reconnect(0),
m_mqtt_interval(MQTT_RECONNECT_INTERVAL*1000),
m_epoch_update_flag(false),
m_datetime(),
m_sensors(8)
{
  m_client_name += String("-") + String(random(0xffffff), HEX);
  
  m_mqtt.setProtocol(MQTT_AUTO_NEGOTIATE);
  //m_mqtt.setProtocol(MQTT_VERSION_3_1);
  //m_mqtt.setProtocol(MQTT_VERSION_3_1_1);
  setCallback(callback);
   
  reset();
}

bool BlindCloud::init(void)
{
  m_event_handler.pushListener(m_event_listener);
  return true;
}

bool BlindCloud::reset(void)
{
  m_epoch_update_flag   = true;
  m_state               = STATE_NONE;
  m_state_dirty         = false;
  m_last_pos            = ~0;
  m_mqtt_last_reconnect = 0;
  
  return true;
}

void BlindCloud::setTimeZone(const ino_u8 time_zone)
{
  m_time_zone = time_zone;
  m_epoch_update_flag = true;
}

void BlindCloud::setDayLightSaving(const ino_u8 daylight_saving)
{
  m_daylight_saving = daylight_saving;
  m_epoch_update_flag = true;
}

void BlindCloud::updateDateTime(const ino::datetime_ts epoch_ms)
{
  if (0==epoch_ms) {
    m_epoch_update_flag = true;
    return;
  }
  
  m_datetime.set_base_ts(epoch_ms, m_time_zone+m_daylight_saving);
}

bool BlindCloud::loop(void)
{
  if (try_to_connect(false))
  {
    if (!connected()) {
      return false;
    }

    if (m_dirty & DIRTY_SENSORS) {
      loopDirtySensors();
    }

    if (m_epoch_update_flag) {
      sendNtpPacket( ); /* send an NTP packet to a time server */
      delay(150);
        
      if (connected() && receiveNtpPacket()) {
        char msg[128];
        
        if (createMQTTPayloadEpochTime(msg, m_datetime.now_ms())) {
          mqttPublishMessage(msg);
        }
      }
    }

    logStatus();
    return true;
  }

  logStatus();
  return false;
}

bool BlindCloud::try_to_connect(const bool wait)
{
  /* if network client has been disabled return.. */
  if (!CLOUD_IS(STATE_ON)) {
    return false;
  }
  
  /* if network client not initilized do it now! */
  if (!CLOUD_IS(STATE_INIT)) {
    WiFi.mode(WIFI_STA);    // WIFI_OFF = 0, WIFI_STA = 1, WIFI_AP = 2, WIFI_AP_STA = 3
    
    WiFi.begin(m_ssid, m_password);
    Serial.printf("WiFi Mode is : %d\n", WiFi.getMode() );
    m_state |= STATE_INIT;
  }

  while (!checkWiFiConnection() && wait) {
    delay(200);
    Serial.print(".");
  }
  
  if (CLOUD_IS(STATE_WIFI_CONNECTED)) {
    if (!CLOUD_IS(STATE_NTP_CONNECTED)) {
      m_state |= STATE_NTP_CONNECTED;
      /* connect to NTP time server */
      m_ntp.begin(NTP_SERVER_PORT);
    }

    mqttReconnect(false);
    
    return true;
  }

  m_state &= ~STATE_NTP_CONNECTED;
  return false;
}

bool BlindCloud::connect(const bool wait)
{
  if (connected()) {
    return true;
  }

  m_state |= STATE_ON;

  return try_to_connect(wait);
}

bool BlindCloud::disconnect( void )
{
  if (checkWiFiConnection())
  {
    mqttDisconnect();
    
    WiFi.disconnect();
    reset();
    delay(100);
    
    pushEvent(BlindEventHandler::Event(BlindEvents::WIFI_ON_DISCONNECT));

    INO_LOG_INFO("BlindCloud::disconnect : 0x%08x", m_state)
  }
  
  return true;  
}

bool BlindCloud::connected(void)
{
  return (checkWiFiConnection() && mqttReconnect(false));
}

void BlindCloud::setCallback(MQTT_CALLBACK_SIGNATURE)
{ 
  this->callback  = callback;
  m_mqtt.setCallback(callback);
}

bool BlindCloud::mqttReconnect(const bool wait)
{ 
  bool ok = m_mqtt.connected();
  
  if ( !ok ) {
    if (!(wait || ino::trigger_event(ino::clock_ms(), m_mqtt_last_reconnect, m_mqtt_interval))) {
      return false;
    }

    do {
      ok = m_mqtt.connect(INO_TO_CSTRING(m_client_name));
    } while (wait && (!ok));
    
    if (ok) {
      m_mqtt.setCallback(this->callback); 
      ok &= m_mqtt.subscribe(MQTT_IN_TOPIC_DOMOTICZ);
      m_mqtt.loop();
      ok &= m_mqtt.subscribe(MQTT_IN_TOPIC_INO);
      m_mqtt.loop();
    }
  
    ok &= m_mqtt.connected();

    if (ok) {
      if (!CLOUD_IS(STATE_MQTT_CONNECTED)) {
        m_state |= STATE_MQTT_CONNECTED;
        m_state_dirty = true;
        pushEvent(BlindEventHandler::Event(BlindEvents::MQTT_ON_CONNECT));
      }
      ino::logSetMqtt(&m_mqtt, MQTT_LOG_TOPIC_INO);
      mqttSendBlindPosition(m_last_pos);
    } else {
      if (CLOUD_IS(STATE_MQTT_CONNECTED)) {
        m_state &= ~STATE_MQTT_CONNECTED;
        m_state_dirty = true;
        pushEvent(BlindEventHandler::Event(BlindEvents::MQTT_ON_DISCONNECT));
      }
      ino::logSetMqtt(NULL, NULL);
    }
  
    m_mqtt_last_reconnect = ino::clock_ms();
    
    return ok;
  }
  
  m_mqtt_last_reconnect = ino::clock_ms();
  m_mqtt.loop();
  
  return true;
}

bool BlindCloud::mqttDisconnect(void)
{
  ino::logSetMqtt(NULL, NULL);
  
  if (m_mqtt.connected()) {  
    m_mqtt.unsubscribe(MQTT_IN_TOPIC_DOMOTICZ);
    m_mqtt.unsubscribe(MQTT_IN_TOPIC_INO);
    m_mqtt.setCallback(NULL);
    m_mqtt.disconnect();
    m_state_dirty = true;

    delay(100);
    
    return (!m_mqtt.connected());
  }
  return true;   
}

bool BlindCloud::mqttSendBlindPosition(const ino_u8 pos)
{
  char msg[MQTT_MAX_PACKET_SIZE];

  if (BLIND_VALID_POSITION(pos) && 
      createMQTTPayloadBlindPosition(msg, MQTT_SENSOR_IDX_BLIND, pos))
  {
    if (mqttPublishMessage(msg)) { 
      m_last_pos = pos;
      return true; 
    }
  }
  return false; 
}

bool BlindCloud::mqttPublishMessage(const char* msg)
{
  if (!m_mqtt.connected()) {
    return false;
  }
  
  if (m_mqtt.publish(MQTT_OUT_TOPIC_DOMOTICZ, msg)) {
    m_mqtt.publish(MQTT_OUT_TOPIC_INO, msg);
    
    INO_LOG_INFO("[%s] BlindCloud::mqttPublishMessage : %s",
      INO_TO_CSTRING(ino::printDateTime(m_datetime.now_ms())), msg)
    return true;
  }

  INO_LOG_ERROR("[%s] BlindCloud::mqttPublishMessage : publish to MQTT broker failed...",
                INO_TO_CSTRING(ino::printDateTime(m_datetime.now_ms())))
            
  return false;
}

// Handlers for NTP Server interfacing..
bool BlindCloud::sendNtpPacket(void)
{
  if (!CLOUD_IS(STATE_NTP_CONNECTED)) {
    return false;
  }
    
  IPAddress ntpServerIP;
  ino_u8 packetBuffer[INO_NTP_PACKET_SIZE];

  WiFi.hostByName(NTP_SERVER_HOST, ntpServerIP); 
  
  /* set all bytes in the buffer to 0 */
  memset(packetBuffer, 0, sizeof(packetBuffer));
  
  /* 
   * Initialize values needed to form NTP request 
   * (see URL above for details on the packets)
   */
  packetBuffer[0]   = 0b11100011;   // LI, Version, Mode
  packetBuffer[1]   = 0;     // Stratum, or type of clock
  packetBuffer[2]   = 6;     // Polling Interval
  packetBuffer[3]   = 0xEC;  // Peer Clock Precision
  /* 8 bytes of zero for Root Delay & Root Dispersion */
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  /* 
   * all NTP fields have been given values, now
   * you can send a packet requesting a timestamp:
   */
  m_ntp.beginPacket(ntpServerIP, 123); // NTP requests are to port 123
  m_ntp.write(packetBuffer, sizeof(packetBuffer));
  m_ntp.endPacket();

  INO_LOG_DEBUG("BlindCloud::sendNtpPacket NTP packet sent...")

  return true;
}

bool BlindCloud::receiveNtpPacket(void)
{
  ino_bool ok = false;
  ino_u8 packetBuffer[INO_NTP_PACKET_SIZE] = {0};
    
  if (m_ntp.parsePacket() >= (int)sizeof(packetBuffer))
  {
    m_ntp.read(packetBuffer, sizeof(packetBuffer));
    m_datetime.set_base_ts(m_datetime.ntp_to_datetime(packetBuffer),
                           m_time_zone+m_daylight_saving);
        
    INO_LOG_DEBUG("BlindCloud : NTP Packet received (%lu)",
                  (ino_u32)(m_datetime.now_ms() / 1000))

    ok                  = true;
    m_epoch_update_flag = false;
    m_state_dirty       = true;
    m_ntp_last_reconnect = ino::clock_ms();
  }
      
  while (m_ntp.parsePacket()>0) {   /* clean-up buffer */
    m_ntp.read(packetBuffer, m_ntp.parsePacket());
  }

  return ok;
}

void BlindCloud::logStatus(void)
{
  if (!m_state_dirty) return;

  const ino_bool ok = CLOUD_IS(STATE_WIFI_CONNECTED|STATE_MQTT_CONNECTED);

  INO_LOG_INFO("##################################################################")
  INO_LOG_INFO("###### Firmware Revision         : %s",
    INO_TO_CSTRING(ino::getFirmwareVersion(BLIND_FIRMWARE_VERSION)))
  INO_LOG_INFO("###### Client Name               : %s",
    INO_TO_CSTRING(m_client_name))
  INO_LOG_INFO("###### Client State              : %s", (ok) ? "OK" : "Error!")
  INO_LOG_INFO("##################################################################")
  if (CLOUD_IS(STATE_WIFI_CONNECTED)) {
    INO_LOG_INFO("###### WiFi : Connected to SSID \"%s\" on channel %u",
      INO_TO_CSTRING(WiFi.SSID()), WiFi.channel())
    INO_LOG_INFO("###### IP address  : %s",
      INO_TO_CSTRING(WiFi.localIP().toString()))
    INO_LOG_INFO("###### MAC address : %s",
      INO_TO_CSTRING(WiFi.macAddress()))
    INO_LOG_INFO("###### Subnet mask : %s",
      INO_TO_CSTRING(WiFi.subnetMask().toString()))
    INO_LOG_INFO("###### Gateway     : %s",
      INO_TO_CSTRING(WiFi.gatewayIP().toString()))
    INO_LOG_INFO("###### DNS Server  : %s",
      INO_TO_CSTRING(WiFi.dnsIP().toString()))
  }
  if (CLOUD_IS(STATE_MQTT_CONNECTED)) {
    INO_LOG_INFO("###### MQTT        : %s", "Connected")
  }
  if (CLOUD_IS(STATE_NTP_CONNECTED)) {
    INO_LOG_INFO("###### NTP Server  : %s", "Connected")
  }

  m_state_dirty = false;
}

bool BlindCloud::pushEvent(
  const BlindEventHandler::Event& event)
{
  // INO_LOG_INFO("[BlindCloud::pushEvent] pushed event(0x%04x) payload_u32(0x%x)", event.get_code(), event.get_u32())
  return m_event_handler.pushEvent(BlindEventHandler::HIGH_PRIORITY, event);
}

bool BlindCloud::loopDirtySensors(void)
{
  if (connected() && (m_dirty & DIRTY_SENSORS))
  {
    char msg[MQTT_MAX_PACKET_SIZE];
    ino_u32 pos = 0;
    for (std::vector<SensorState>::iterator it=m_sensors.begin(); it!=m_sensors.end(); it++, pos++)
    {
      if (!it->dirty) continue;
      INO_LOG_DEBUG("[%s] BlindCloud::loopDirtySensors pos(%d) idx(%u) temperature(%f) humidity(%f)",
        INO_TO_CSTRING(ino::printDateTime(m_datetime.now_ms())), pos, it->idx, it->temperature, it->humidity)
          
      if (createMQTTPayloadTemperature(msg, it->idx, it->temperature, it->humidity))
      {
        /* Publish payload to MQTT broker */
        if (mqttPublishMessage(msg)) {
          it->dirty = false;
          // printf(" loopDirtySensors: published message: %s\n", msg);
          return true;
        }
      }
    }
    INO_LOG_DEBUG("[%s] BlindCloud::loopDirtySensors flag dirty cleared. pos(%d)",
      INO_TO_CSTRING(ino::printDateTime(m_datetime.now_ms())), pos)
    m_dirty &= ~DIRTY_SENSORS;
    // printf(" loopDirtySensors: reset dirty flag.\n");
  }
  return false; 
}

void BlindCloud::updateTemperature(
  const ino_u8 pos, const ino_u8 idx, const float temperature)
{
  m_dirty |= DIRTY_SENSORS;
  m_sensors[pos].dirty = true;
  m_sensors[pos].idx = idx;
  m_sensors[pos].temperature = temperature;

  INO_LOG_DEBUG("[%s] BlindCloud::updateTemperature pos(%u) idx(%u) temperature(%f)",
    INO_TO_CSTRING(ino::printDateTime(m_datetime.now_ms())), pos, idx, temperature)
}

void BlindCloud::updateHumidity(
  const ino_u8 pos, const ino_u8 idx, const float humidity)
{
  m_dirty |= DIRTY_SENSORS;
  m_sensors[pos].dirty = true;
  m_sensors[pos].idx = idx;
  m_sensors[pos].humidity = humidity;

  INO_LOG_DEBUG("[%s] BlindCloud::updateHumidity pos(%u) idx(%u) humidity(%f)",
    INO_TO_CSTRING(ino::printDateTime(m_datetime.now_ms())), pos, idx, humidity)
}

bool BlindCloud::updateBlindPosition(
  const ino_u8 pos, const bool force)
{
  if ((!force) && (pos==m_last_pos) ) {
    return true;
  }

  if (connected()) {
    INO_LOG_DEBUG("[%s] BlindCloud::updateBlindPosition to %u",
      INO_TO_CSTRING(ino::printDateTime(m_datetime.now_ms())), pos)

    return mqttSendBlindPosition(pos);
  }
  return false;  
}

void BlindCloud::parse_event(
  const BlindEventHandler::Event& event)
{
  switch (event.get_code())
  {
    case BlindEvents::BLIND_ON_START:
    case BlindEvents::BLIND_ON_STOP:
      break;

    case BlindEvents::GOT_TEMPERATURE:
    {
      ino_u8 sensor_id;
      ino_float temperature;

      if (m_event_handler.parseEventTemperature(
        event, sensor_id, temperature))
      {
        if (sensor_id==0x1) {
#ifdef MQTT_SENSOR_IDX_TEMP1
          updateTemperature(sensor_id, MQTT_SENSOR_IDX_TEMP1, temperature);
#endif
        } else {
#ifdef MQTT_SENSOR_IDX_TEMP2
          updateTemperature(sensor_id, MQTT_SENSOR_IDX_TEMP2, temperature);
#endif
        }
      }
    } break;

    case BlindEvents::GOT_HUMIDITY:
    {
      ino_u8 sensor_id;
      ino_float humidity;

      if (m_event_handler.parseEventHumidity(
        event, sensor_id, humidity))
      {
        if (sensor_id==0x1) {
#ifdef MQTT_SENSOR_IDX_TEMP1
          updateHumidity(sensor_id, MQTT_SENSOR_IDX_TEMP1, humidity);
#endif
        } else {
#ifdef MQTT_SENSOR_IDX_TEMP2
          updateHumidity(sensor_id, MQTT_SENSOR_IDX_TEMP2, humidity);
#endif
        }
      }
    } break;

    default:
      break;
  }
}
