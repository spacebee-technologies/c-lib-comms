#ifndef COMMUNICATION_INTERFACE_H_
#define COMMUNICATION_INTERFACE_H_

#include <stdint.h>
#include <stddef.h>

typedef struct CommunicationInterface {
  void *instance;
  uint8_t (*send)(void *self, const uint8_t *buffer, size_t bufferSize);
  uint8_t (*receive)(void *self, uint8_t *buffer, size_t bufferSize, size_t *receivedSize);
} CommunicationInterface;

/**
 * @brief Send a stream of bytes over an abstract communication channel
 *
 * @param self A CommunicationInterface instance
 * @param buffer Stream of bytes to be sent
 * @param bufferSize Number of bytes to be sent
 * @return uint8_t 0 if successful
 */
uint8_t CommunicationInterface_send(CommunicationInterface *self, const uint8_t *buffer,
                                    size_t bufferSize);

/**
 * @brief Receive a stream of bytes over an abstract communication channel
 *
 * @param self A CommunicationInterface instance
 * @param buffer Pointer to a buffer where received stream will be set by reference
 * @param bufferSize Size of the buffer given by reference, used to check overflow
 * @param receivedSize A pointer where the read buffer size will be returned
 * @return uint8_t 0 if successful
 */
uint8_t CommunicationInterface_receive(CommunicationInterface *self, uint8_t *buffer,
                                       size_t bufferSize, size_t *receivedSize);

#endif  // COMMUNICATION_INTERFACE_H_
