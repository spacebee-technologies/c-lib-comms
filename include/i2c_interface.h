#ifndef I2C_INTERFACE_H_
#define I2C_INTERFACE_H_

#include <stdint.h>
#include <stddef.h>

typedef struct I2cInterface {
  void *instance;
  uint8_t (*sendMessage)(void *self, const uint8_t *txBuff, uint8_t nBytes, uint16_t address);
  uint8_t (*readMessage)(void *self, uint8_t *rxBuff, size_t bufferSize, uint16_t address);
  uint8_t (*writeRead)(void *self, uint16_t address, const uint8_t *txBuff, size_t txSize, uint8_t *rxBuff, size_t rxSize);
} I2cInterface;

/**
 * @brief Sends an I2C message
 *
 * @param self An I2C interface
 * @param txBuff Data to send
 * @param nBytes Size of buffer in bytes
 * @param address I2C address to send to
 * @return uint8_t 0 on success, 1 if error
 */
uint8_t I2cInterface_sendMessage(I2cInterface *self, const uint8_t *txBuff, uint8_t nBytes, uint16_t address);

/**
 * @brief Receives a message over I2C
 *
 * @param self An I2C interface
 * @param rxBuff Buffer in which to store the received message
 * @param bufferSize Size of the buffer
 * @param address I2C address of the device
 * @return uint8_t 0 on success, 1 if error
 */
uint8_t I2cInterface_readMessage(I2cInterface *self, uint8_t *rxBuff, size_t bufferSize, uint16_t address);

/**
 * @brief Writes and then receives a message over I2C
 *
 * @param self An I2C interface
 * @param address I2C address of the device
 * @param txBuff Data to send
 * @param txSize Size of the transmit buffer in bytes
 * @param rxBuff Buffer in which to store the received message
 * @param rxSize Size of the received buffer in bytes
 * @return uint8_t 0 on success, 1 if error
 */
uint8_t I2cInterface_writeRead(I2cInterface *self, uint16_t address, const uint8_t *txBuff, size_t txSize, uint8_t *rxBuff, size_t rxSize);

#endif  // I2C_INTERFACE_H_
