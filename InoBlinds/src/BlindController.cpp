#include "BlindController.h"

#define TIME_OFFSET             (10000000)
#define RELAY_STATE_OFF         (false)
#define RELAY_STATE_ON          (true)
#define UNIT_STEP_MS            (10)

INO_STATIC
void onEventDummy(
  const ino_handle caller, const BlindPos pos, const BlindDirection direction)
{
  INO_UNUSED(caller)
  INO_UNUSED(pos)
  INO_UNUSED(direction)
}

INO_STATIC
void listenerDefault(
  const BlindEventHandler::Event& event,
  ino_handle cookie)
{
  ((BlindController*)cookie)->parse_event(event);
}

BlindController::BlindController(
  BlindEventHandler& event_handler,
  const bool swap_high_low,
  const ino_u8 pin_open_cmd,
  const ino_u8 pin_close_cmd,
  BlindEventFunc on_start_func,
  BlindEventFunc on_stop_func,
  const ino_handle caller) :
m_event_handler(event_handler),
m_event_listener(BlindEvents::BLIND_MOVE_TO, listenerDefault, this),
m_on(true),
m_stopped(true),
m_swap_high_low(swap_high_low),
m_pin_open_cmd(pin_open_cmd),
m_pin_close_cmd(pin_close_cmd),
m_direction(BLIND_IDLE),
m_time_open(0),
m_time_close(0),
m_time_base(0),
m_time_curr(0),
m_time_end(0),
m_on_start_func((on_start_func) ? on_start_func : onEventDummy),
m_on_stop_func( (on_stop_func)  ? on_stop_func  : onEventDummy),
m_caller(caller)
{
  pinMode(m_pin_open_cmd, OUTPUT);
  pinMode(m_pin_close_cmd, OUTPUT);

  sendCmd(RELAY_STATE_OFF, RELAY_STATE_OFF);
}

/***  Public Methods Section  *************************************************/
bool BlindController::on(void)
{
  return m_on;
}

void BlindController::on(const bool _on)
{
  m_on = _on;
  
  if (m_on) {
    const BlindPos pos = currentPosition();
    m_on_start_func(m_caller, pos, BLIND_IDLE);
    m_event_handler.pushEventOnStart(pos, BLIND_IDLE);
  } else {
    /* 
     * put in idle blind control
     * when disabling the module
     */
    stop();
  }
}

void BlindController::init(
  const ino_u32 time_open_ms,
  const ino_u32 time_close_ms,
  const BlindPos start_pos)
{
  m_event_handler.pushListener(m_event_listener);

  m_time_open         = time_open_ms;
  m_time_close        = time_close_ms;

  /* start_pos is in range [0..100] */
  m_time_curr         = time_close_ms * BLIND_GET_UNIT_POSITION(start_pos);
  m_time_end          = m_time_curr;
  m_time_base         = ino::clock_ms();

  sendCmd(RELAY_STATE_OFF, RELAY_STATE_OFF);
}

bool BlindController::idle(void)
{
  return ( (!on()) || (m_time_end==m_time_curr) );
}

bool BlindController::validPosition(const BlindPos pos)
{
  return BLIND_VALID_POSITION(pos);  
}

BlindPos BlindController::currentPosition(void) const
{
  return (BlindPos)BLIND_GET_POSITION((float)m_time_curr/m_time_close);
}

void BlindController::swapUpDown(void)
{
  const ino_u8 t  = m_pin_open_cmd;
  m_pin_open_cmd   = m_pin_close_cmd;
  m_pin_close_cmd  = t;
}

bool BlindController::swapHighLow(void)
{
  m_swap_high_low = (m_swap_high_low) ? false : true;
  return m_swap_high_low;
}

void BlindController::moveTo(
  const BlindPos position,
  const bool wait)
{
  /* Trivial check: if out-of-bound position or blinds are disabled return */
  if (!(validPosition(position) && on())) {
    return;
  }

  /* 
   * This handle the special case when the blind is already moving and an 
   * up/down command is received: if this happen the blind should be stopped
   */
  if (!idle()) {
    stop();
    return;  
  }
 
  const ino_timestamp t = m_time_close * BLIND_GET_UNIT_POSITION(position);
  /* Trivial check: if move to same position do nothing */
  if ((position==currentPosition()) || (t==m_time_curr)) {
    stop();
    return;
  }

  /*
   * Set the new time endpoint (t is in the range [0..m_time_close])
   * when t==0 means the blind is completely open
   * when t==m_time_close means the blind is completely closed
   */
  m_time_end = t;

  m_direction = BLIND_IDLE;

  if (m_time_end!=m_time_curr)
  {  
    const BlindPos pos = currentPosition();
    m_direction = (m_time_end>m_time_curr) ? BLIND_CLOSE : BLIND_OPEN;
    m_on_start_func(m_caller, pos, m_direction);
    m_event_handler.pushEventOnStart(pos, m_direction);

    if ( m_direction==BLIND_CLOSE )
    {
      /* we want to close the blind now */
      INO_LOG_DEBUG( "BLIND CLOSE TO : %u %%", position)    
      sendCmd(RELAY_STATE_OFF, RELAY_STATE_ON);
    } else {
      /* we want to open the blind now */
      INO_LOG_DEBUG( "BLIND OPEN  TO : %u %%", position)
      sendCmd(RELAY_STATE_ON, RELAY_STATE_OFF);
    }

    m_time_base    = ino::clock_ms();

    /*
     * we are waiting for the command to be executed: this is useful when
     * we want to complete a commmand without stopping its execution
     */
    if (wait)
    {
      const ino_interval how_much = (m_time_end>m_time_curr) 
        ? (m_time_end-m_time_curr) : (m_time_curr-m_time_end);

      /* Add 500 ms more to be sure it is really ended */
      updateCurrentTime(how_much+500);
      stop();
    }
  }
}

void BlindController::stop(void)
{
  const BlindPos pos = currentPosition();
  
  INO_LOG_DEBUG("BlindController::stop = %u %%", pos)

  m_time_end   = m_time_curr;
  m_time_base  = ino::clock_ms();
  
  sendCmd(RELAY_STATE_OFF, RELAY_STATE_OFF);

  m_on_stop_func(m_caller, pos, m_direction);

  m_event_handler.pushEventOnStop(pos, m_direction);

  m_direction = BLIND_IDLE;
}

bool BlindController::loop(void)
{ 
  /* current blind position reached the end: no need to do nothing */
  if (idle()) {
    return false;
  }
  
  /* 
   * OK, we need to move the blind a little bit for a time of X milliseconds
   * calculated as a percentage of total s->m_time_close time.
   */
  if (updateCurrentTime()) {
    stop();
    return false;
  }
  
  return true;
}

void BlindController::parse_event(
  const BlindEventHandler::Event& event)
{
  const ino_timestamp _now = ino::clock_ms();
  BlindPos pos;
  ino_bool wait;
  if (m_event_handler.parseEventMoveTo(event, pos, wait)) 
  {
    INO_LOG_SUDO("  [BlindController::parse_event] code(%u) pos(%u) wait(%d) ts("CLOCK_FMT") latency(%d)",
      event.get_code(), pos, wait, event.get_timestamp(), _now - event.get_timestamp())
  }
}

/***  Private Methods Section  ************************************************/
void BlindController::sendCmd(
  const bool openState, const bool closeState)
{
  const bool sameState = ( openState==closeState );
  m_stopped = ((openState==RELAY_STATE_OFF) && (closeState==RELAY_STATE_OFF));
  /* Safe check on blind state: they could not be both on ON-ON state! */
  if (sameState && (!m_stopped)) {
    return;
  }

  const ino_u8 _on  = (m_swap_high_low) ? LOW  : HIGH;
  const ino_u8 _off = (m_swap_high_low) ? HIGH : LOW;

  /* 1st always put them in OFF-OFF */
  digitalWrite(m_pin_close_cmd, _off);
  digitalWrite(m_pin_open_cmd,  _off);
  delay(RELAY_INTER_DELAY_MSEC);    

  /* Handle OFF - OFF state */
  if (sameState) {
    return;
  }
  
  /* 
   * here the only possible states are:
   * Close - Open
   * OFF   - ON
   * ON    - OFF
   */
  if (RELAY_STATE_ON==openState) {
    // digitalWrite(m_pin_close_cmd, _off);
    // delay(RELAY_INTER_DELAY_MSEC);
    digitalWrite(m_pin_open_cmd, _on);
    delay(RELAY_INTER_DELAY_MSEC);    
  } else if (RELAY_STATE_ON==closeState) {
    // digitalWrite(m_pin_open_cmd, _off);
    // delay(RELAY_INTER_DELAY_MSEC);       
    digitalWrite(m_pin_close_cmd, _on);
    delay(RELAY_INTER_DELAY_MSEC);
  }
}

bool BlindController::updateCurrentTime(const ino_interval how_much)
{
  if (how_much>0) {
    //INO_LOG_DEBUG( "how_much : "DELAY_FMT"", how_much)
    ino::wait_ms(how_much);
  }

  const ino_timestamp time_now = ino::clock_ms();

  //if (time_now<m_time_base+UNIT_STEP_MS) {
  if (!ino::trigger_event(time_now, m_time_base, UNIT_STEP_MS)) {
    //printf("\ttime_elapsed(%u)\r\n", time_elapsed);
    return false;
  }

  /* calculate effective elapsed time from last update */
  const ino_interval time_elapsed = ino::elapsed_ms(time_now, m_time_base);
  
  /* update the reference time adding last elapsed time */
  m_time_base = time_now;

  if (m_direction==BLIND_CLOSE)
  {
    const ino_timestamp curr = TIME_OFFSET + m_time_curr + time_elapsed;
    m_time_curr = (curr>(TIME_OFFSET+m_time_end)) ? m_time_end : (curr-TIME_OFFSET);
  } else {
    //const float ratio_open_close = (float)m_time_open/m_time_close;
    //const ino_timestamp curr = (m_time_curr+TIME_OFFSET) - (ratio_open_close*time_elapsed));
    const ino_timestamp curr = TIME_OFFSET + m_time_curr - ((m_time_open*time_elapsed)/m_time_close);
    m_time_curr = (curr<(TIME_OFFSET+m_time_end)) ? m_time_end : (curr-TIME_OFFSET);
  }

  return (m_time_curr==m_time_end);
}

#undef TIME_OFFSET
#undef RELAY_STATE_OFF
#undef RELAY_STATE_ON
#undef UNIT_STEP_MS
