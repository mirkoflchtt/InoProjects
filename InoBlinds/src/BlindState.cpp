#include <ArduinoJson.h>
#include "BlindState.h"


INO_API_DECLARE

#ifdef BLIND_ANALOG_BUTTON

static
bool analogButtonOpenTrigger(const ino_u8 pin)
{
  const int v = analogRead(pin);
  //INO_LOG_TRACE("[analogButton-Open] v = %u",v);
  return (v>=BLIND_ANALOG_BUTTON_THR_OPEN);
}

static
bool analogButtonCloseTrigger(const ino_u8 pin)
{
  const int v = analogRead(pin);
  //INO_LOG_TRACE("[analogButton-Close] v = %u",v);
  return (v>=BLIND_ANALOG_BUTTON_THR_CLOSE && 
          v<BLIND_ANALOG_BUTTON_THR_OPEN);
}

#endif    // BLIND_ANALOG_BUTTON

#ifdef BLIND_REMOTE_BUTTON

static
bool remoteButtonTrigger(const ino_u8 pin)
{
  return (digitalRead(pin)==HIGH);
} 

#endif    // BLIND_REMOTE_BUTTON

#ifdef LIGHT01_RELAY
bool analogReadLight01(const ino_u8 pin)
{
  const int v = analogRead(pin);
  INO_LOG_TRACE("[analogReadLight01] v = %u",v);
  return (v>=BLIND_ANALOG_BUTTON_THR_LIGHT1 && 
          v<BLIND_ANALOG_BUTTON_THR_CLOSE);
}
#endif

#ifdef LIGHT02_RELAY
bool analogReadLight02(const ino_u8 pin)
{
   const int v = analogRead(pin);
   INO_LOG_TRACE("[analogReadLight02] v = %u",v);
   return (v>=BLIND_ANALOG_BUTTON_THR_LIGHT2 &&
           v<BLIND_ANALOG_BUTTON_THR_LIGHT1);
}
#endif

void onUpdateTemperature1(
  const ino_handle caller, const float temperature, const float humidity)
{
#ifdef MQTT_SENSOR_IDX_TEMP1
  GlobalState* state = (GlobalState*)caller;
  INO_ASSERT(state)

  state->cloud().updateTemperature(MQTT_SENSOR_IDX_TEMP1, temperature, humidity);
  if ( state->blind().idle() ) {
    state->cloud().updateBlindPosition(state->blind().currentPosition());
  }

#ifdef BLIND_CONFIG_FILE
  state->saverContext().m_count_reads_temp1++;
  state->saverStore(10);
#endif

#else
  INO_UNUSED(caller)
  INO_UNUSED(temperature)
  INO_UNUSED(humidity)
#endif
}

void onUpdateTemperature2(
  const ino_handle caller, const float temperature, const float humidity)
{
#ifdef MQTT_SENSOR_IDX_TEMP2
  GlobalState* state = (GlobalState*)caller;
  INO_ASSERT(state)

  state->cloud().updateTemperature(MQTT_SENSOR_IDX_TEMP2, temperature, humidity);

  if ( state->blind().idle() ) {
    state->cloud().updateBlindPosition(state->blind().currentPosition());
  }
    
#ifdef BLIND_CONFIG_FILE
  state->saverContext().m_count_reads_temp2++;
  state->saverStore(10);
#endif

#else
  INO_UNUSED(caller)
  INO_UNUSED(temperature)
  INO_UNUSED(humidity)
#endif
}

void onBlindStart(
  const ino_handle caller, const ino_u8 pos, const BlindDirection direction)
{
  GlobalState* state = (GlobalState*)caller;
  INO_ASSERT(state)
  INO_UNUSED(pos)
  INO_UNUSED(direction)
  
  state->updateLastCommandTime();
}

void onBlindStop(
  const ino_handle caller, const ino_u8 pos, const BlindDirection direction)
{
  GlobalState* state = (GlobalState*)caller;
  INO_ASSERT(state)
  INO_LOG_TRACE("[onBlindStop] %s : pos(%u)",
    (direction==BLIND_OPEN) ? "Opening" : "Closing", pos)
  state->cloud().updateBlindPosition(pos);
  state->updateLastCommandTime();

#ifdef BLIND_CONFIG_FILE
  state->saverContext().m_count_open  += (direction==BLIND_OPEN);
  state->saverContext().m_count_close += (direction==BLIND_CLOSE);
  state->saverStore(0);
#endif
}

static
void listenerDefault(
  const ino::event_t* event, ino::listener_handle_t cookie)
{
  INO_ASSERT(event)
  INO_LOG_SUDO("default_listener : code(%u) cookie(%p) ts("CLOCK_FMT")",
    event->code, event->cookie, event->timestamp)
}

#ifdef BLIND_SENSOR1_TEMPERATURE_PIN
static 
bool loopSensorTemp1(void* fn_arg)
{
  ((ino::SensorTemperature*)fn_arg)->loop();
  return true;
}
#endif

#ifdef BLIND_SENSOR2_TEMPERATURE_PIN
static 
bool loopSensorTemp2(void* fn_arg)
{
  ((ino::SensorTemperature*)fn_arg)->loop();
  return true;
}
#endif

static
bool loopBlind(void* fn_arg)
{
  INO_ASSERT(fn_arg)
  return ((BlindController*)fn_arg)->loop();
}

static
bool loopCloud(void* fn_arg)
{
  INO_ASSERT(fn_arg)
  return ((BlindCloud*)fn_arg)->loop();
}

static
bool loopStateUpdate(void* fn_arg)
{
  INO_ASSERT(fn_arg)
  ((GlobalState*)fn_arg)->updateState();
  ((GlobalState*)fn_arg)->updateIdleTime();
  return true;  
}

INO_API_DECLARE_END


GlobalState::GlobalState(
  MQTT_CALLBACK_SIGNATURE,
  OTA_CALLBACK(onOtaStartEnd),
  OTA_CALLBACK_ERROR(onOtaError),
  OTA_CALLBACK_PROGRESS(onOtaProgress)) :
m_on_off_count(0),
#ifdef BLIND_ANALOG_BUTTON
m_buttonOpen(BLIND_ANALOG_BUTTON, LOW, &analogButtonOpenTrigger, BLIND_BUTTON_LONG_PRESS, BLIND_BUTTON_TIMEOUT, BLIND_BUTTON_DEBOUNCE),
m_buttonClose(BLIND_ANALOG_BUTTON, LOW, &analogButtonCloseTrigger, BLIND_BUTTON_LONG_PRESS, BLIND_BUTTON_TIMEOUT, BLIND_BUTTON_DEBOUNCE),
#endif
#ifdef BLIND_REMOTE_BUTTON
m_remoteOpen(BLIND_REMOTE_OPEN, LOW, &remoteButtonTrigger, BLIND_BUTTON_LONG_PRESS, BLIND_BUTTON_TIMEOUT, BLIND_BUTTON_DEBOUNCE),
m_remoteClose(BLIND_REMOTE_CLOSE, LOW, &remoteButtonTrigger, BLIND_BUTTON_LONG_PRESS, BLIND_BUTTON_TIMEOUT, BLIND_BUTTON_DEBOUNCE),
#endif
m_blind(PIN_SWAP_RELAY, BLIND_OPEN_RELAY, BLIND_CLOSE_RELAY, onBlindStart, onBlindStop, this),
m_cloud(BLIND_WIFI_SSID, BLIND_WIFI_PWD, NULL),
#ifdef BLIND_SENSOR1_TEMPERATURE_PIN
m_temperature1(BLIND_SENSOR1_TEMPERATURE_PIN, DHT22_TYPE, onUpdateTemperature1, this, BLIND_SENSOR1_TEMPERATURE_INTERVAL, true),
#endif
#ifdef BLIND_SENSOR2_TEMPERATURE_PIN
m_temperature2(BLIND_SENSOR2_TEMPERATURE_PIN, DHT22_TYPE, onUpdateTemperature2, this, BLIND_SENSOR2_TEMPERATURE_INTERVAL, true),
#endif
#ifdef LIGHT01_RELAY
m_buttonLight01(BLIND_ANALOG_BUTTON, LOW, &analogReadLight01, BLIND_BUTTON_LONG_PRESS, BLIND_BUTTON_TIMEOUT, BLIND_BUTTON_DEBOUNCE),
m_relayLight01(LIGHT01_RELAY, true),
#endif
#ifdef LIGHT02_RELAY
m_buttonLight02(BLIND_ANALOG_BUTTON, LOW, &analogReadLight02, BLIND_BUTTON_LONG_PRESS, BLIND_BUTTON_TIMEOUT, BLIND_BUTTON_DEBOUNCE),
m_relayLight02(LIGHT02_RELAY, true),
#endif
m_led(BLIND_LED, false, BLIND_LED_SWAP),
m_ota_enable(false),
m_ota(MQTT_CLIENT_NAME, OTA_PASSWORD, OTA_PORT, onOtaStartEnd, onOtaStartEnd, onOtaProgress, onOtaError),
m_now(0),
m_last_command_time(0),
m_idle_time(BLIND_IDLE_TIME)
#ifdef BLIND_CONFIG_FILE
, m_saver(BLIND_CONFIG_FILE)
, m_saver_context()
#endif
{
  this->callback = callback;
  INO_LOG_TRACE("GlobalState::GlobalState Constructor Done.")
}

#ifdef BLIND_CONFIG_FILE
bool GlobalState::saverStore(const uint32_t dirty)
{ 
  m_saver_context.m_date_time     = m_cloud.dateTime().now_ms();
  m_saver_context.m_pos           = m_blind.currentPosition();
  
  return m_saver.save(&m_saver_context, dirty);
}
#endif

bool GlobalState::handleMessage(const char* topic, const char* msg)
{  
  if ( !(topic && msg) ) {
    return false;
  }

  if ( 0==strcmp(topic,MQTT_IN_TOPIC_DOMOTICZ) ) {
    return handleDomoticzMessage(msg);
  }
  else if ( 0==strcmp(topic,MQTT_IN_TOPIC_INO) ) {
    return handleInoMessage(msg);
  }
    
  return false;
}

bool GlobalState::handleDomoticzMessage(const char* msg)
{
  INO_ASSERT(msg)
  
#ifdef BLIND_STATE_COMMAND_DEBOUNCE
  if (!ino::trigger_event(ino::clock_ms(), m_last_command_time, BLIND_STATE_COMMAND_DEBOUNCE)) {
    return false;
  }
#endif

  StaticJsonDocument<MQTT_MAX_PACKET_SIZE> doc;
  const DeserializationError error = deserializeJson(doc, msg);

  if ( error ) {
    INO_LOG_ERROR("GlobalState::handleDomoticzMessage() Json parsing failed")
    return false;
  }

/*
 * *******************************************************
 * Example of a "Blinds Percentage Inverted"
{
   "Battery" : 255,
   "RSSI" : 12,
   "dtype" : "Light/Switch",
   "id" : "03FE0000",
   "idx" : 10,
   "name" : "Camera",
   "nvalue" : 2,
   "stype" : "Selector Switch",
   "svalue1" : "99",
   "switchType" : "Blinds Percentage Inverted",
   "unit" : 1
}

* *******************************************************
 * Example of a "Temp + Humidity"
{
   "Battery" : 255,
   "RSSI" : 12,
   "dtype" : "Temp + Humidity",
   "id" : "82001",
   "idx" : 1,
   "name" : "Cameretta",
   "nvalue" : 0,
   "stype" : "THGN122/123, THGN132, THGR122/228/238/268",
   "svalue1" : "7.50",
   "svalue2" : "51.40",
   "svalue3" : "0",
   "unit" : 1
}

*/

  const String name       = doc["name"];
  //const String dtype      = doc["dtype"];
  //const uint32_t id       = doc["id"];
  const uint32_t cmd_idx  = doc["idx"].as<uint32_t>();
  uint32_t cmd_id         = doc["nvalue"].as<uint32_t>();
  uint32_t cmd_value      = doc["svalue1"].as<uint32_t>();
  bool parsed             = false;
  
  switch (cmd_idx)
  {    
    case MQTT_SENSOR_IDX_BLIND:
    {
      switch ( cmd_id ) {
        /* Pushed OFF button : close the blind completely */
        case 0:
          cmd_id = 2; cmd_value = 0;
          break;
        /* Pushed ON button : open the blind completely */
        case 1:
          cmd_id = 2; cmd_value = 100;
          break;
        default:
          break;
      }
      
      /* Move the blind to a fixed percentage */
      if ( (2==cmd_id) && BLIND_VALID_POSITION(cmd_value) )
      {
          m_blind.moveTo(BLIND_INVERT_POSITION(cmd_value), false);
          INO_LOG_INFO(" GlobalState::handleDomoticzMessage to \"%s\" : moveTo() = %u",
            INO_TO_CSTRING(name), cmd_value)
      }
      parsed            = true;
      //lastCommandTime() = ino::clock_ms();
    } break;
      
    default:
      break;
  }
  
  return parsed;
}

bool GlobalState::handleInoMessage(const char* msg)
{
  INO_ASSERT(msg)

  StaticJsonDocument<MQTT_MAX_PACKET_SIZE> doc;
  const DeserializationError error = deserializeJson(doc, msg);

  if (error) {
    INO_LOG_ERROR("GlobalState::handleInoMessage() Json parsing failed")
    return false;
  }
  
  INO_LOG_INFO(" GlobalState::handleInoMessage received: \"%s\"", msg)

  /*
  const String name       = doc["name"].as<String>();
  const uint32_t cmd_idx  = doc["idx"].as<uint32_t>();
  uint32_t cmd_id         = doc["nvalue"].as<uint32_t>();
  uint32_t cmd_value      = doc["svalue1"].as<uint32_t>();
  */
  bool parsed             = false;
  parsed = true;
  
  return parsed;
}

void GlobalState::moveTo(const ino_u8 pos)
{
  if ( pos==BLIND_CMD_DISABLE ) {
    m_blind.on( !m_blind.on() );
    m_on_off_count++;

    m_ota_enable = ((m_on_off_count>=OTA_ENABLE_ON_OFF_COUNT) && 
                    !m_blind.on() && m_blind.idle());
    
    if ( m_blind.on() ) {
      m_cloud.updateBlindPosition(m_blind.currentPosition(), false);
    }

    if ( m_ota_enable ) {
      INO_LOG_DEBUG("m_ota_enable    : YES (port: %d)", OTA_PORT)
    } else {
      INO_LOG_DEBUG("m_ota_enable    : NO")
    }
    INO_LOG_DEBUG("m_blind.on()    : %d", m_blind.on())
    INO_LOG_DEBUG("m_on_off_count  : %u", m_on_off_count)

    return;
  }
  
  if ( !m_blind.validPosition(pos) ) {
    return;
  }
    
  if ( m_blind.currentPosition()!=pos ) {
    m_on_off_count = 0;
    m_ota_enable   = false;
  }
    
  m_blind.moveTo(pos, false);
}

bool GlobalState::updateState(void)
{
  // Check if conditions are met to enable OTA server listening
  if (m_ota_enable) {
    m_led.blink(70);
    m_ota.loop();
    return true;
  }

  INO_LOG_TRACE("GlobalState::updateState: m_blind.on() : %d\n", m_blind.on())

  if ( m_blind.on() ) {
    if ( m_cloud.connect() ) {
      m_led.high(false);
      m_cloud.setCallback(callback);
    } else {
      m_led.blink(600);
    }
  } else {
    m_led.high(true);
    m_cloud.setCallback(NULL);
#ifdef BLIND_DISCONNECT_CLOUD
    m_cloud.disconnect();
#endif
  }

  return true;
}

bool GlobalState::updateIdleTime(void)
{
  const ino::clock_ts _now = ino::clock_ms(); 
  m_idle_time = (ino::trigger_event(_now, m_now, m_idle_time))
    ? 0 : ino::elapsed_ms(m_now+m_idle_time, _now);
  if ( m_idle_time>0 ) {
    INO_LOG_DEBUG("### Idle Time: %u", m_idle_time)
    delay(m_idle_time);
    return true;
  }
  return false;
}


bool GlobalState::init(Stream* stream)
{
    ino_u8 currPos = BLIND_MID_POSITION;
  
    m_cloud.setCallback(this->callback);

    ino::logSetName(MQTT_CLIENT_NAME);
    ino::logSetQuiet(false);
    ino::logSetLevel(LOG_INFO);
    ino::logSetStream(stream);

    ino::handlerInit(listenerDefault, this);

#ifdef BLIND_SENSOR1_TEMPERATURE_PIN
    ino::handlerPushLoop(loopSensorTemp1, &this->m_temperature1);
#endif
#ifdef BLIND_SENSOR2_TEMPERATURE_PIN
    ino::handlerPushLoop(loopSensorTemp2, &this->m_temperature2);
#endif
    ino::handlerPushLoop(loopBlind, &this->m_blind);
    ino::handlerPushLoop(loopCloud, &this->m_cloud);
    ino::handlerPushLoop(loopStateUpdate, this);
    
#ifdef BLIND_CONFIG_FILE
    m_saver.on(true);
    
    const StateSaveContext* ctx  = m_saver.load();
    
    if ( ctx && m_blind.validPosition(ctx->m_pos) )
    {
      m_saver_context = *ctx;
      m_cloud.updateDateTime(ctx->m_date_time);
      currPos = ctx->m_pos;
      
#ifdef BLIND_VERBOSE
      ino::printDateTime(m_cloud.dateTime().now_ms());
#endif
    }
#endif  // BLIND_CONFIG_FILE

#ifndef BLIND_CONFIG_FILE_WIPE
    m_saver_context.m_count_on += 1;
#endif

/*
#ifdef LIGHT01_RELAY
     m_relayLight01.on();
#endif
#ifdef LIGHT02_RELAY
    m_relayLight02.on();
#endif
*/
    m_cloud.setTimeZone(1);
    m_cloud.setDayLightSaving(0);
    m_cloud.connect();
    
    m_blind.init(BLIND_OPEN_TIME_MSEC, BLIND_CLOSE_TIME_MSEC, currPos);

    currPos = m_blind.currentPosition();
    const int32_t deltaPos = (currPos<=BLIND_MID_POSITION) ? 5 : -5;

    INO_LOG_INFO("###### Testing Blinds..")

    m_blind.moveTo(currPos+deltaPos, true);
    m_blind.moveTo(currPos, true);
    currPos = m_blind.currentPosition();
    
    INO_LOG_INFO("###### Testing Blinds OK: Actual position: %u %%" INO_CR, currPos)

#if ( defined BLIND_ANALOG_BUTTON || defined BLIND_REMOTE_BUTTON )
    // Disable blind controller by default
    // Thus it is not possible to send commands via WiFi in this state
    // Only accepted commands are the one from analog or remote buttons
    m_blind.on(false);
#endif

    updateState();

    INO_LOG_INFO("###############################################################")
    INO_LOG_INFO("###### Client Name               : %s", MQTT_CLIENT_NAME)
    INO_LOG_INFO("###### Firmware Revision         : %s",
      INO_TO_CSTRING(ino::getFirmwareVersion(BLIND_FIRMWARE_VERSION)))
    INO_LOG_INFO("###### Current blind position is : %u %%", currPos)
#ifdef BLIND_CONFIG_FILE
    INO_LOG_INFO("###### Number of power on done   : %u", m_saver_context.m_count_on)
    INO_LOG_INFO("###### Number of openings        : %u", m_saver_context.m_count_open)
    INO_LOG_INFO("###### Number of closings        : %u", m_saver_context.m_count_close)
    INO_LOG_INFO("###### Number of temp1 readings  : %u", m_saver_context.m_count_reads_temp1)
    INO_LOG_INFO("###### Number of temp2 readings  : %u", m_saver_context.m_count_reads_temp2)
#endif
    INO_LOG_INFO("###############################################################")

    return true;
}


bool GlobalState::reset(void)
{ 
  INO_LOG_TRACE("GlobalState::reset called")
  m_cloud.reset();
  // m_cloud.updateGlobalTime(0);
  
  return true;
}


bool GlobalState::loop(void)
{
  ino_u8 next_pos                  = BLIND_CMD_IDLE;
  ino::StdButtonEvent openEvent     = ino::EV_NONE;
  ino::StdButtonEvent closeEvent    = ino::EV_NONE;

#ifdef BLIND_ANALOG_BUTTON
  openEvent   = m_buttonOpen.check();
  closeEvent  = m_buttonClose.check();
#endif

#ifdef BLIND_REMOTE_BUTTON
  if ( ino::EV_NONE==openEvent ) {
    openEvent   = m_remoteOpen.check();
  }
  if ( ino::EV_NONE==closeEvent ) {
    closeEvent  = m_remoteClose.check();
  }
#endif

  m_idle_time                           = (m_blind.on() || m_ota_enable) ? BLIND_IDLE_TIME : BLIND_IDLE_TIME_MAX;
  m_now                                 = ino::clock_ms();
  
#ifdef LIGHT01_RELAY
  switch ( m_buttonLight01.check() ) {
    case ino::EV_NONE:
      break;
    default:
      m_relayLight01.toggle();
      break;
  }
#endif

#ifdef LIGHT02_RELAY
  switch ( m_buttonLight02.check() ) {
    case ino::EV_NONE:
      break;
    default:
      m_relayLight02.toggle();
      break;
  }
#endif

  if ( ino::EV_NONE!=openEvent ) {
    next_pos = (ino::EV_TIMEOUT==openEvent)  ? BLIND_CMD_DISABLE : BLIND_OPEN_POSITION;
    //EVENT_PUSH_HIGH(EVENT_BLIND_MOVE_TO, next_pos)
  }
  else if ( ino::EV_NONE!=closeEvent ) {
    next_pos = (ino::EV_TIMEOUT==closeEvent) ? BLIND_CMD_DISABLE : BLIND_CLOSE_POSITION;
    //EVENT_PUSH_HIGH(EVENT_BLIND_MOVE_TO, next_pos)
  }
  
  moveTo(next_pos);

  if ( !m_blind.idle() ) {
    INO_LOG_DEBUG("Blind Position : %u %% Closed", m_blind.currentPosition())
    m_idle_time = 0;
  }
  
  return ino::handlerLoop();
}
