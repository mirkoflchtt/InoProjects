#ifndef BLIND_CONTROLLER_H
#define BLIND_CONTROLLER_H
#include "BlindDefines.h"

#define RELAY_INTER_DELAY_MSEC  (25)
#define BLIND_STEP_MS           (25)

typedef enum {
  BLIND_IDLE    = 0x0,
  BLIND_OPEN,
  BLIND_CLOSE,
} BlindDirection;

typedef ino_u8 BlindPos;

typedef void (*BlindEventFunc)(const ino_handle caller, const BlindPos pos, const BlindDirection direction);


class BlindController {
public:

  BlindController(
    const bool swap_high_low,
    const ino_u8 pin_open_cmd,
    const ino_u8 pin_close_cmd,
    BlindEventFunc on_start_func = NULL,
    BlindEventFunc on_stop_func = NULL,
    const ino_handle caller = NULL);

  bool          on(void);
  void          on(const bool on_);

  void          init(
                  const ino_u32 time_open_ms,
                  const ino_u32 time_close_ms,
                  /* 
                   * start_pos is a value in the range BLIND_OPEN_POSITION=all
                   * open and BLIND_CLOSE_POSITION=all close
                   */
                  const BlindPos start_pos); 
  
  bool          idle(void);

  bool          validPosition(const BlindPos pos);
  
  BlindPos      currentPosition(void) const;

  void          swapUpDown(void);

  bool          swapHighLow(void);
  
  void          moveTo(
                  const BlindPos position,
                  const bool wait);
                  
  void          stop(void);
  
  bool          loop(void);
  
private:

  void          sendCmd(
                  const bool openState,
                  const bool closeState);

  bool          updateCurrentTime(
                  const ino_interval how_much=BLIND_STEP_MS);

  bool                  m_on;
  bool                  m_stopped;
  bool                  m_swap_high_low;

  ino_u8                m_pin_open_cmd;
  ino_u8                m_pin_close_cmd;

  BlindDirection        m_direction;
  
  ino_timestamp         m_time_open;
  ino_timestamp         m_time_close;
  ino_timestamp         m_time_base;
  ino_timestamp         m_time_curr;
  ino_timestamp         m_time_end;

  BlindEventFunc        m_on_start_func;
  BlindEventFunc        m_on_stop_func;
  const ino_handle      m_caller;
};

#undef BLIND_STEP_MS

#endif    /*BLIND_CONTROLLER_H*/

