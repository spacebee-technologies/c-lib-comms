#include "gpio_interface.h"

bool GpioInterface_getPinState(GpioInterface *self) {
  return self->getPinState(self->instance);
}

void GpioInterface_setPinState(GpioInterface *self, uint8_t state) {
  return self->setPinState(self->instance, state);
}
