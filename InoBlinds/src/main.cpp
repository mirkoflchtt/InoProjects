#include <Arduino.h>
#include <Streaming.h>

#include "BlindDefines.h"
#include "BlindState.h"


void onOtaStartEnd(void);

void otaOnError(ota_error_t error);

void otaOnProgress(unsigned int progress, unsigned int total);

void onMqttMessageGet(char* topic, byte* payload, unsigned int length);

GlobalState g_state(
  onMqttMessageGet,
  onOtaStartEnd, otaOnError, otaOnProgress);


void onMqttMessageGet(char* topic, byte* payload, unsigned int length)
{ 
  // Force NULL terminating string
  payload[length] = '\0';

  g_state.handleMessage((const char*)topic, (const char*)payload);
}

void onOtaStartEnd(void)
{
  g_state.led().blink(500,4);
}

void otaOnError(ota_error_t error)
{
  printf("otaOnError[%u] = ", error);
  switch ( error ) {
    case OTA_AUTH_ERROR:    printf(" Authentication Failed\n"); break;
    case OTA_BEGIN_ERROR:   printf(" Begin Failed\n"); break;
    case OTA_CONNECT_ERROR: printf(" Connect Failed\n"); break;
    case OTA_RECEIVE_ERROR: printf(" Receive Failed\n"); break;
    case OTA_END_ERROR:     printf(" End Failed\n"); break;
    default: break;
  }
  
  g_state.led().blink(200,24);
  ESP.restart();
}

void otaOnProgress(unsigned int progress, unsigned int total)
{
  const ino_i32 perc = (100 * progress) / total;
  printf("[Blinds] Progress : %d %%\n", perc);
}

void setup(void)
{
  Serial.begin(BLIND_BAUD_RATE);
  while ( !Serial ) {}
  delay(200);
    
  Serial.printf("\n### Initializing..\n");
  delay(200);
  
  g_state.init(&Serial);
}

void loop(void)
{
  g_state.loop();
}

#if 0

int main(int argc, char*argv[])
{
  setup();
  for (;;) {
    loop();
  }
  return 0;  
}

#endif