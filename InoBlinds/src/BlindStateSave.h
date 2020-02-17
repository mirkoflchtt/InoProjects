#ifndef BLIND_STATE_SAVER_H
#define BLIND_STATE_SAVER_H
#include "BlindDefines.h"

class StateSaveContext {
public:

  StateSaveContext(void) :
    m_date_time(0),
    m_firmware(BLIND_FIRMWARE_VERSION),
    m_count_open(0),
    m_count_close(0),
    m_count_reads_temp1(0),
    m_count_reads_temp2(0),
    m_count_on(0),
    m_pos(BLIND_MID_POSITION),
    m_reserved(0)
  {
  }

public:
  ino::datetime_ts  m_date_time;
  ino_u32           m_firmware;
  ino_u32           m_count_open;
  ino_u32           m_count_close;
  ino_u32           m_count_reads_temp1;
  ino_u32           m_count_reads_temp2;
  ino_u32           m_count_on;
  ino_u32           m_pos       : 8;
  ino_u32           m_reserved  : 24;
};

class BlindStateSaver 
{
  public:
     BlindStateSaver(const char* save_file_path);

     void               on(const bool on);

     void               dirty()  { m_dirty+=1; }     
     bool               save(StateSaveContext* ctx, const ino_u32 dirty=0);
     
     StateSaveContext*  load(void);
     
  private:

     void              init(void);

     const char*       m_save_file_path;
     ino_u32           m_dirty;
     bool              m_ok;
     bool              m_on;

     StateSaveContext  m_save_ctx;

     ino_u32           m_data[2];
};

#endif      /*BLIND_STATE_SAVER_H*/
