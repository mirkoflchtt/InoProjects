#include "InoLog.h"

#include "BlindAlexa.h"

INO_DECLARE_STATIC
void dimChange(EspalexaDevice* d)
{
  BlindEspAlexa* blind = (d && d->getCookie()) ? (BlindEspAlexa*)d->getCookie() : NULL; 
  if (blind) {
    blind->moveTo(d->getPercent());
  }
}

INO_STATIC
Espalexa* g_alexa = NULL;

INO_STATIC
void listenerDefault(
  const BlindEventHandler::Event& event,
  ino_handle cookie)
{
  BlindEspAlexa* alexa = (BlindEspAlexa*)cookie;
  if (alexa) {
    alexa->parse_event(event);
  }
}

BlindEspAlexa::BlindEspAlexa(
  BlindEventHandler& event_handler,
  const char* name, const ino_i32 server_port):
m_event_handler(event_handler),
m_event_listener(
  BlindEvents::WIFI_ON_CONNECT|BlindEvents::WIFI_ON_DISCONNECT,
  listenerDefault, this),
m_espalexa(),
m_server(server_port),
m_device(name, dimChange, EspalexaDeviceType::dimmable, 255, this),
m_initialized(false)
{
  g_alexa = &m_espalexa;
}

ino_bool BlindEspAlexa::init(void)
{
  return m_event_handler.pushListener(m_event_listener);
}

ino_bool BlindEspAlexa::bootstrap(void)
{
  if (!m_initialized) {
    m_server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send(200, "text/plain", "This is an example index page your server may send.");
    });
    m_server.onNotFound([](AsyncWebServerRequest *request) {
      if (!g_alexa->handleAlexaApiCall(request)) // if you don't know the URI, ask espalexa whether it is an Alexa control request
      {
        // whatever you want to do with 404s
        request->send(404, "text/plain", "Page Not Found!");
      }
    });

    // Define your devices here.
    //m_device.setValue(255);
    m_espalexa.addDevice(&m_device);
    m_espalexa.begin(&m_server); //give espalexa a pointer to your server object so it can use your server instead of creating its own
    
    // dimChange(&m_device);

    m_initialized = true;

    INO_LOG_INFO("[BlindEspAlexa::bootstrap] done.")

    return true;
  }

  return false;
}

ino_bool BlindEspAlexa::loop(void)
{
  if (m_initialized) {
    m_espalexa.loop();
    delay(1);
  }
  return true;
}

void BlindEspAlexa::parse_event(
  const BlindEventHandler::Event& event)
{
  INO_LOG_INFO("[BlindEspAlexa::parse_event] event(0x%04x)", event.get_code())

  switch (event.get_code())
  {
    case BlindEvents::WIFI_ON_CONNECT:
      bootstrap();
      break;
    case BlindEvents::WIFI_ON_DISCONNECT:
      m_initialized = false;
      break;
    default:
      break;
  }
}

void BlindEspAlexa::moveTo(
  const uint8_t value)
{
  INO_LOG_INFO("[BlindEspAlexa::moveTo] : value(%u)", value)
  m_event_handler.pushEventMoveTo(value); 
}
