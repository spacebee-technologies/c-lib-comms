#ifndef NAV_COMMS_H_
#define NAV_COMMS_H_

#include <stdbool.h>

#include "communication_interface.h"

#define NAV_COMMS_TX_BUFFER_SIZE 32

typedef struct NavComms {
  CommunicationInterface *transport;
  uint8_t txData[NAV_COMMS_TX_BUFFER_SIZE];
} NavComms;

void NavComms_create(NavComms *self, CommunicationInterface *transport);
bool NavComms_sendMessage(NavComms *self);
void NavComms_setMessage(NavComms *self, const char *msg);

#endif  // NAV_COMMS_H_
