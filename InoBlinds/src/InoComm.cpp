#include "InoComm.h"

INO_NAMESPACE

/*
- CONNECTIONS: nRF24L01+ Modules See:

http://arduino-info.wikispaces.com/Nrf24L01-2.4GHz-HowTo
  1 - GND
  2 - VCC 3.3V !!! NOT 5V
  3 - CE to Arduino pin 9
  4 - CSN to Arduino pin 10
  5 - SCK to Arduino pin 13
  6 - MOSI to Arduino pin 11
  7 - MISO to Arduino pin 12
  8 - UNUSED

Receive a series of zeros and ones to the sender radio.

*/

#define CE_PIN            (4)
#define CSN_PIN           (15)

#define COMM_MAGIC        (0xDEADBEEF)

enum {
  MODE_SEND,
  MODE_RECEIVE,
  MODE_NONE,
};

CommPacket::CommPacket(const CommPayload pay) :
m_magic(COMM_MAGIC),
m_serial(0),
m_from(0),
m_from_mask(0xff),
m_to(0),
m_to_mask(0xff),
m_payload(pay)
{
}

Comm::Comm(const uint64_t pipe) :
m_mode(MODE_NONE),
m_radio_pipe(pipe),
m_radio(CE_PIN, CSN_PIN),
m_cipher()
{
}

void Comm::init(void)
{
  BYTE key[ 16 ];

  generateLFSRSequence(
    (ino_u16*)key,
    sizeof(key)/sizeof(ino_u16), 
    (ino_u16)( (m_radio_pipe>>24)^(m_radio_pipe>>16)^(m_radio_pipe) ) );

  m_cipher.setkey ( key, 128 );
  
  m_radio.begin();
  m_radio.printDetails();
  
  Serial.println("Comm::init Done!");
}

bool Comm::send(const CommPayload& pay)
{
  CommPacket packet(pay);
  Serial.print("Comm::send   ");
  Serial.println(packet.m_payload);
  
  m_cipher.encrypt((const BYTE*)&packet, (BYTE*)&packet);
  
  return sendPacket(packet);
}

bool Comm::sendPacket(const CommPacket& packet)
{
  if ( MODE_SEND!=m_mode ) {
    m_mode = MODE_SEND;
    if ( MODE_RECEIVE==m_mode )
      m_radio.stopListening();
    m_radio.openWritingPipe( m_radio_pipe );
  }
  m_radio.write(&packet, sizeof(packet));
  
  return true;
}

bool Comm::receive(CommPayload& pay, const bool wait)
{
  if ( MODE_RECEIVE!=m_mode ) {
    m_mode = MODE_RECEIVE;
    m_radio.openReadingPipe(1, m_radio_pipe);
    m_radio.startListening();
    delay( 20 );
  }
  
  while ( !m_radio.available() ) {
    if ( !wait )
      return false;
    delay(200);
  }
    
  CommPacket packet, cipher;
  
  m_radio.read(&cipher, sizeof(cipher));
  m_cipher.decrypt((const BYTE*)&cipher, (BYTE*)&packet);

  Serial.print("Comm::receive ");
  Serial.println(packet.m_payload);

  // Check if valid packet
  if ( COMM_MAGIC!=packet.m_magic )
    return false;
    
  if ( /*(packet.m_to&packet.m_to_mask)!=*/0 ) {
    sendPacket(cipher);
    return false;
  }
  
  pay = packet.m_payload;
  
  return true;
}

INO_NAMESPACE_END
