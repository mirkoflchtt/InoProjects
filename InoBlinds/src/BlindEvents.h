#ifndef BLIND_EVENTS_H
#define BLIND_EVENTS_H

#define EVENT_PUSH_HIGH(code_, cookie_) \
{ \
  const event_t event_ = { .code=(code_), \
  .cookie=(event_handle_t)(cookie_), .timestamp=ino::clock_ms() }; \
  ino::handlerPushEvent(HIGH_PRIORITY_QUEUE, &event_); \
}

#define EVENT_PUSH_LOW(code_, cookie_) \
{ \
  const event_t event_ = { .code=(code_), \
  .cookie=(event_handle_t)(cookie_), .timestamp=ino::clock_ms() }; \
  ino::handlerPushEvent(LOW_PRIORITY_QUEUE, &event_); \
}

#define EVENT_BLIND_MOVE_TO           (0x00000001)

#endif      /*BLIND_EVENTS_H*/
