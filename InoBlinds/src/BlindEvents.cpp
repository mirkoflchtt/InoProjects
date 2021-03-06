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
  const ino_u32 payload = ((((ino_u32)pos) << 8) | ((wait) ? 0x1U : 0x0U));
  // printf("  pushEventMoveTo: pos(%u) wait(%d) payload(0x%x)\n", pos, wait, payload);
  return pushEvent(HIGH_PRIORITY, Event(BlindEvents::BLIND_MOVE_TO, payload));
}

ino_bool BlindEventHandler::parseEventMoveTo(
    const Event& event, BlindPos& pos, bool& wait)
{   
  if (event.get_code()==BlindEvents::BLIND_MOVE_TO) {
    const ino_u32 payload = event.get_u32();
    wait = (payload & 0x1U) ? true : false;
    pos = (payload >> 8) & 0xFF;
    // printf("  parseEventMoveTo: pos(%u) wait(%d) payload(0x%x)\n", pos, wait, payload);

    return BLIND_VALID_POSITION(pos);
  }
  return false;
}

ino_bool BlindEventHandler::pushEventOnStart(
  const BlindPos pos, const BlindDirection direction)
{
  const ino_u32 payload = ((((ino_u32)pos) << 8) | (direction & 0xF));
  // printf("  pushEventOnStart: pos(%u) direction(%d) payload(0x%x)\n", pos, direction, payload);
  return pushEvent(LOW_PRIORITY, Event(BlindEvents::BLIND_ON_START, payload));
}

ino_bool BlindEventHandler::parseEventOnStart(
    const Event& event, BlindPos& pos, BlindDirection& direction)
{
  if (event.get_code()==BlindEvents::BLIND_ON_START) {
    const ino_u32 payload = event.get_u32();
    pos = (payload >> 8) & 0xF;
    direction = (BlindDirection)(payload & 0xFF);
    // printf("  parseEventOnStart: pos(%u) direction(%d) payload(0x%x)\n", pos, direction, payload);
    return true;
  }
  return false;
}

ino_bool BlindEventHandler::pushEventOnStop(
  const BlindPos pos, const BlindDirection direction)
{
  const ino_u32 payload = ((((ino_u32)pos) << 8) | (direction & 0xF));
  // printf("  pushEventOnStop: pos(%u) direction(%d) payload(0x%x)\n", pos, direction, payload);
  return pushEvent(LOW_PRIORITY, Event(BlindEvents::BLIND_ON_STOP, payload));
}

ino_bool BlindEventHandler::parseEventOnStop(
    const Event& event, BlindPos& pos, BlindDirection& direction)
{
  if (event.get_code()==BlindEvents::BLIND_ON_STOP) {
    const ino_u32 payload = event.get_u32();
    pos = (payload >> 8) & 0xFF;
    direction = (BlindDirection)(payload & 0xF);
    // printf("  parseEventOnStop: pos(%u) direction(%d) payload(0x%x)\n", pos, direction, payload);
    return true;
  }
  return false;
}

/**  Private APIs  **********************************************************/
