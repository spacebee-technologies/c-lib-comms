#include "nav_comms.h"

#include <string.h>

#include "zephyr_time.h"
#include "logger.h"

#define ACK_SIZE 32

void NavComms_create(NavComms *self, CommunicationInterface *transport) {
  self->transport = transport;
}

bool NavComms_sendMessage(NavComms *self) {
  size_t receivedSize;
  uint8_t ack[ACK_SIZE] = {0};
  if (self->txData[0] == '\0') {
    strncpy((char *)self->txData, "test", NAV_COMMS_TX_BUFFER_SIZE - 1);
    self->txData[NAV_COMMS_TX_BUFFER_SIZE - 1] = '\0';  // Ensure null termination
  }

  CommunicationInterface_send(self->transport, self->txData, sizeof(self->txData));
  ZephyrTime_sleepMilli(100);
  if (!CommunicationInterface_receive(self->transport, ack, ACK_SIZE, &receivedSize)) {
    ack[receivedSize] = '\0';
    LOG_DEBUG("Received: %s", (char *)ack);
    memset(self->txData, 0, NAV_COMMS_TX_BUFFER_SIZE);
    return true;
  }
  memset(self->txData, 0, NAV_COMMS_TX_BUFFER_SIZE);
  return false;
}

void NavComms_setMessage(NavComms *self, const char *msg) {
  strncpy((char *)self->txData, msg, NAV_COMMS_TX_BUFFER_SIZE - 1);
  size_t messageLength = strlen(msg);
  self->txData[messageLength] = '\0';
}
