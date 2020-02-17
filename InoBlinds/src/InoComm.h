#ifndef INO_COMM_H
#define INO_COMM_H
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <CamelliaLib.h>

#include "InoPlatform.h"

#define COMM_DEFAULT_PIPE \
  ((ino_u64)0xE8E8F0F0E1)

INO_NAMESPACE

typedef ino_u32    CommPayload;

class CommPacket {
public:
  CommPacket(const CommPayload pay=0);
  
  ino_u32     m_magic;
  ino_u32     m_serial; 
  ino_u8      m_from;
  ino_u8     m_from_mask;  
  ino_u8     m_to;
  ino_u8     m_to_mask;
    
  CommPayload m_payload; 
};

class Comm {
public:
   Comm(const ino_u64 pipe=COMM_DEFAULT_PIPE);
  
  void        init(void);
   
  bool        send(const CommPayload& pay);
  bool        receive(CommPayload& pay, const bool wait=false);
  
private:
  bool        sendPacket(const CommPacket& packet);
  
  ino_u8      m_mode;
  
  ino_u64     m_radio_pipe;
  RF24        m_radio;

  Camellia    m_cipher;
};

INO_NAMESPACE_END

#endif    /*INO_COMM_H*/
