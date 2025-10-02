#include "communication_interface.h"

uint8_t CommunicationInterface_send(CommunicationInterface *self, const uint8_t *buffer,
                                    size_t bufferSize) {
  return self->send(self->instance, buffer, bufferSize);
}

uint8_t CommunicationInterface_receive(CommunicationInterface *self, uint8_t *buffer,
                                       size_t bufferSize, size_t *receivedSize) {
  return self->receive(self->instance, buffer, bufferSize, receivedSize);
}
