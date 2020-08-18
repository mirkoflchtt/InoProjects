#include "BlindEvents.h"

BlindEventHandler::BlindEventHandler(
  const ino_size event_queue_size) :
ino::EventHandler(event_queue_size)
{
}

/**  Public APIs  ***********************************************************/
ino_bool BlindEventHandler::pushEventMoveTo(
  const BlindPos pos, const bool wait)
{
    Event::event_param param = (wait) ? 0x1 : 0x0;
  return pushEvent(HIGH_PRIORITY, Event(BlindEvents::BLIND_MOVE_TO, param, (ino_i32)pos));
}

ino_bool BlindEventHandler::parseEventMoveTo(
    const Event& event, BlindPos& pos, bool& wait)
{   
  if (event.get_code()==BlindEvents::BLIND_MOVE_TO) {
    wait = (event.get_param() & 0x1);
    pos = event.get_i32();
    return true;
  }
  return false;
}

ino_bool BlindEventHandler::pushEventOnStart(
  const BlindPos pos, const BlindDirection direction)
{
  return pushEvent(LOW_PRIORITY, Event(BlindEvents::BLIND_ON_START, direction, (ino_i32)pos));
}

ino_bool BlindEventHandler::pushEventOnStop(
  const BlindPos pos, const BlindDirection direction)
{
  return pushEvent(LOW_PRIORITY, Event(BlindEvents::BLIND_ON_STOP, direction, (ino_i32)pos));
}


/**  Private APIs  **********************************************************/
