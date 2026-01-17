#ifndef GPIO_INTERFACE_H_
#define GPIO_INTERFACE_H_

#include <stdint.h>
#include <stdbool.h>

typedef struct GpioInterface {
  void *instance;
  bool (*getPinState)(void *self);
  void (*setPinState)(void *self, uint8_t state);
} GpioInterface;

/**
 * @brief Get the state for a given GPIO pin
 *
 * @param self An GPIO interface
 * @return true if signal is in high logic state, false if signal is low
 */
bool GpioInterface_getPinState(GpioInterface *self);

/**
 * @brief Set the state of a given GPIO pin
 *
 * @param self An GPIO interface
 * @param state 1 to set the signal in high state, 0 for low
 */
void GpioInterface_setPinState(GpioInterface *self, uint8_t state);

#endif  // GPIO_INTERFACE_H_
