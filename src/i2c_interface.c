#include "i2c_interface.h"

uint8_t I2cInterface_sendMessage(I2cInterface *self, const uint8_t *txBuff, uint8_t nBytes, uint16_t address) {
  return self->sendMessage(self->instance, txBuff, nBytes, address);
}

uint8_t I2cInterface_readMessage(I2cInterface *self, uint8_t *rxBuff, size_t bufferSize, uint16_t address) {
  return self->readMessage(self->instance, rxBuff, bufferSize, address);
}
