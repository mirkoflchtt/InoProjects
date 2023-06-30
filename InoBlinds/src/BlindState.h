#ifndef BLIND_STATE_H
#define BLIND_STATE_H
#include "BlindDefines.h"
#include "BlindConfig.h"
#include "BlindController.h"
#include "BlindEvents.h"
#include "BlindCloud.h"
#include "BlindStateSave.h"

#ifdef BLIND_HAS_ALEXA
#include "BlindAlexa.h"
#endif

#define STATE_FLAG_NONE                       (0x0U)
#define STATE_FLAG_OTA_ENABLE                 (0x1U<<0)
#define STATE_FLAG_MOVING                     (0x1U<<1)

class GlobalState {
public:

  GlobalState(
    MQTT_CALLBACK_SIGNATURE,
    OTA_CALLBACK(onOtaStartEnd)=NULL,
    OTA_CALLBACK_ERROR(onOtaError)=NULL,
    OTA_CALLBACK_PROGRESS(onOtaProgress)=NULL);

  bool                init(Stream* stream=NULL);
  bool                reset(void);
  bool                loop(void);

  void                parse_event(
                        const BlindEventHandler::Event& event);

  void                updateLastCommandTime(void) { m_last_command_time = ino::clock_ms(); }
  
  BlindCloud&         cloud(void)  { return m_cloud; }
  BlindController&    blind(void)  { return m_blind; }
  ino::PinOut&        led(void)    { return m_led; }
  
  bool                handleMessage(const char* topic, const char* msg);

#ifdef BLIND_CONFIG_FILE
  bool                saverStore(const ino_u32 dirty=0);
  StateSaveContext&   saverContext(void)   { 
    m_saver.dirty();
    return m_saver_context;
  }
#endif

  bool                updateState(void);
  bool                updateIdleTime(void);
  
private:
  bool                handleDomoticzMessage(const char* msg);
  bool                handleInoMessage(const char* msg);
  
  void                moveTo(const ino_u8 pos);
  
private:

  MQTT_CALLBACK_SIGNATURE;

  ino::LoopHandler    m_looper;
  BlindEventHandler   m_event_handler;
  BlindEventHandler::Listener m_event_listener;

#ifdef BLIND_HAS_ALEXA
  BlindEspAlexa       m_alexa;
#endif
  ino_u8              m_on_off_count;

#ifdef BLIND_ANALOG_BUTTON
  ino::StdButton      m_buttonOpen;
  ino::StdButton      m_buttonClose;
#endif

#ifdef BLIND_REMOTE_BUTTON
  ino::StdButton      m_remoteOpen;
  ino::StdButton      m_remoteClose;
#endif

#ifdef LIGHT01_RELAY
  ino::StdButton      m_buttonLight01;
  ino::PinOut         m_relayLight01;
#endif

#ifdef LIGHT02_RELAY
  ino::StdButton      m_buttonLight02;
  ino::PinOut         m_relayLight02;
#endif

  BlindController     m_blind;
  BlindCloud          m_cloud;

#ifdef BLIND_SENSOR1_TEMPERATURE_PIN
  ino::SensorTemperature m_temperature1;
#endif

#ifdef BLIND_SENSOR2_TEMPERATURE_PIN
  ino::SensorTemperature m_temperature2;
#endif

  uint32_t            m_state;
  ino::PinOut         m_led;

  ino::OTA            m_ota;
  
  ino_timestamp       m_now;
  ino_timestamp       m_last_command_time;
  ino_timestamp       m_idle_time;

#ifdef BLIND_CONFIG_FILE
  BlindStateSaver     m_saver;
  StateSaveContext    m_saver_context;
#endif
};

#endif    /*BLIND_STATE_H*/
