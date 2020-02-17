#include "BlindStateSave.h"
#include "InoCRC.h"

#define HAS_SPIFFS

#ifdef HAS_SPIFFS
#include <FS.h>
#endif

#define SAVE_MAGIC \
  (0xDEADCAFE)

// Header Version + Header Size (Byte)
#define SAVE_HEADER \
  ( (0x1<<16) | sizeof(m_save_ctx) )
  
#define GET_CRC(data, data_size) \
  getCRC((const ino_u8*)(data), (ino_u32)(data_size))

#if (defined HAS_SPIFFS && defined BLIND_DEBUGGING)
static
void getFSInfo(void)
{
  FSInfo fs_info;
  
  SPIFFS.begin();

  SPIFFS.info(fs_info);

  float fileTotalKB = (float)fs_info.totalBytes / 1024.0;
  float fileUsedKB = (float)fs_info.usedBytes / 1024.0;
  
  float flashChipSize = (float)ESP.getFlashChipSize() / 1024.0 / 1024.0;
  float realFlashChipSize = (float)ESP.getFlashChipRealSize() / 1024.0 / 1024.0;
  float flashFreq = (float)ESP.getFlashChipSpeed() / 1000.0 / 1000.0;
  FlashMode_t ideMode = ESP.getFlashChipMode();
  
  Serial.printf("\n#####################\n");
  
  Serial.printf("__________________________\n\n");
  Serial.println("Firmware: ");
  Serial.printf(" Chip Id: %08X\n", ESP.getChipId());
  Serial.print(" Core version: "); Serial.println(ESP.getCoreVersion());
  Serial.print(" SDK version: "); Serial.println(ESP.getSdkVersion());
  Serial.print(" Boot version: "); Serial.println(ESP.getBootVersion());
  Serial.print(" Boot mode: "); Serial.println(ESP.getBootMode());
  
  Serial.printf("__________________________\n\n");
  
  Serial.println("Flash chip information: ");
  Serial.printf(" Flash chip Id: %08X (for example: Id=001640E0 Manuf=E0, Device=4016 (swap bytes))\n", ESP.getFlashChipId());
  Serial.printf(" Sketch thinks Flash RAM is size: "); Serial.print(flashChipSize); Serial.println(" MB");
  Serial.print(" Actual size based on chip Id: "); Serial.print(realFlashChipSize); Serial.println(" MB ... given by (2^( \"Device\" - 1) / 8 / 1024");
  Serial.print(" Flash frequency: "); Serial.print(flashFreq); Serial.println(" MHz");
  Serial.printf(" Flash write mode: %s\n", (ideMode == FM_QIO ? "QIO" : ideMode == FM_QOUT ? "QOUT" : ideMode == FM_DIO ? "DIO" : ideMode == FM_DOUT ? "DOUT" : "UNKNOWN"));
  
  Serial.printf("__________________________\n\n");
  
  Serial.println("File system (SPIFFS): ");
  Serial.print(" Total KB: "); Serial.print(fileTotalKB); Serial.println(" KB");
  Serial.print(" Used KB: "); Serial.print(fileUsedKB); Serial.println(" KB");
  Serial.printf(" Block size: %u\n", fs_info.blockSize);
  Serial.printf(" Page size: %u\n", fs_info.pageSize);
  Serial.printf(" Maximum open files: %u\n", fs_info.maxOpenFiles);
  Serial.printf(" Maximum path length: %u\n\n", fs_info.maxPathLength);

  Serial.printf("CPU frequency: %u MHz\n\n", ESP.getCpuFreqMHz());
  Serial.printf("#####################\n");

  Dir dir = SPIFFS.openDir("/");
  Serial.println("SPIFFS directory {/} :");
  while (dir.next()) {
    Serial.print(" "); Serial.println(dir.fileName());
    Serial.println(" "); Serial.println(dir.fileSize());
  }
  //SPIFFS.format();
}
#endif

static
ino_u32 getCRC(const ino_u8* data, ino_u32 data_size)
{
  return ino::fletcher32(data, data+data_size);
  //return 0x0; 
}


// format bytes
String formatBytes(const size_t bytes)
{
  if (bytes < 1024){
    return String(bytes)+"B";
  } else if(bytes < (1024 * 1024)){
    return String(bytes/1024.0)+"KB";
  } else if(bytes < (1024 * 1024 * 1024)){
    return String(bytes/1024.0/1024.0)+"MB";
  } else {
    return String(bytes/1024.0/1024.0/1024.0)+"GB";
  }
}


BlindStateSaver::BlindStateSaver(const char* save_file_path) :
m_save_file_path(save_file_path),
m_ok(false),
m_on(false)
{
}

void BlindStateSaver::on(const bool _on)
{
  m_on = _on;
}

void BlindStateSaver::init(void)
{
  m_ok = false;
  
  if ( !m_save_file_path ) {
    return;
  }

  memset(&m_data, 0, sizeof(m_data));

#ifdef HAS_SPIFFS
  if ( !SPIFFS.begin() ) {
    INO_LOG_ERROR("BlindStateSaver::SPIFFS Initialization failed!")
    return;
  }
#ifdef BLIND_DEBUGGING
  getFSInfo();
#endif

  m_ok = SPIFFS.exists(m_save_file_path);

  INO_LOG_INFO("BlindStateSaver::init done : %s", m_ok ? "OK" : "KO!")
#endif
}

bool BlindStateSaver::save(
  StateSaveContext* ctx, const ino_u32 dirty)
{
  if ( !(ctx && m_on && m_dirty>dirty) ) {
    return false;
  }
  
  m_dirty = 0;
  m_save_ctx = *ctx;
  
#ifdef HAS_SPIFFS
  File file = SPIFFS.open(m_save_file_path, "w");
  if (file) {
    m_data[0] = SAVE_MAGIC;
    m_data[1] = SAVE_HEADER;
    file.write( (ino_u8*)m_data, sizeof(m_data) );
    file.write( (ino_u8*)&m_save_ctx, sizeof(m_save_ctx) );

    m_data[0] = GET_CRC(&m_save_ctx, sizeof(m_save_ctx));   // CRC Count
    m_data[1] = SAVE_MAGIC;
    
    file.write((ino_u8*)m_data, sizeof(m_data));
    file.close();
    
    INO_LOG_DEBUG("BlindStateSaver:save done OK ( %u+%u )",
      sizeof(m_save_ctx), 2*sizeof(m_data))

    return true;
  }

  INO_LOG_ERROR("BlindStateSaver:save done KO!!")

  return false;
#else
  return true;
#endif
}

StateSaveContext* BlindStateSaver::load(void)
{
#ifdef HAS_SPIFFS
  if (!m_ok) {
    init();
  }
  
  if (m_ok && m_on)
  {
#ifdef BLIND_CONFIG_FILE_WIPE
    m_save_ctx = StateSaveContext();
    save(&m_save_ctx);
#else
    File file = SPIFFS.open(m_save_file_path, "r");

    if( file ) {
      bool res  = false;
      StateSaveContext save_ctx;
      
      file.read( (ino_u8*)m_data, sizeof(m_data) );
      if ( (m_data[0]==SAVE_MAGIC) && ((m_data[1] & 0xFFFF)==sizeof(save_ctx)) )
      {
        file.read((ino_u8*)&save_ctx, sizeof(save_ctx));
        file.read((ino_u8*)m_data, sizeof(m_data));
        if ( (m_data[1]==SAVE_MAGIC) && (m_data[0]==GET_CRC(&save_ctx, sizeof(save_ctx))) )
          res = true;
      }
      file.close();
      
      if (false==res) {
        return NULL;
      }
      
      m_save_ctx = save_ctx;
      
      INO_LOG_INFO("### BlindStateSaver     : load done OK.")
      INO_LOG_INFO("### Firmware Version    : %s",
        INO_TO_CSTRING(ino::getFirmwareVersion(m_save_ctx.m_firmware)))
      INO_LOG_INFO("### DateTime (epoch)    : %s",
        INO_TO_CSTRING(ino::printDateTime(m_save_ctx.m_date_time)))
      INO_LOG_INFO("### Actual position     : %u %%", m_save_ctx.m_pos)

      // Update with current firmware version
      m_save_ctx.m_firmware = BLIND_FIRMWARE_VERSION;
      
      return &m_save_ctx;  
    }
#endif    // BLIND_CONFIG_FILE_WIPE
  }

  INO_LOG_ERROR("### BlindStateSaver:load done KO!!")
#endif

  return NULL;
}
