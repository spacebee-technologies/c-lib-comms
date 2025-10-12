#ifndef UDP_BRIDGE_H_
#define UDP_BRIDGE_H_

#include "communication_interface.h"
#include "i2c_interface.h"
#include "mutex_interface.h"

typedef struct UdpBridge {
  CommunicationInterface communicationInterfaceView;
  uint16_t destinationPort;
  I2cInterface *i2c;
  MutexInterface *mutex;
} UdpBridge;

/**
 * @brief Constructor for UDP bridge
 *
 * @param self Uninitialized UDP bridge structure
 * @param destinationPort UDP destination port to send a message to
 * @param i2c Handler for I2C comms
 * @param mutex Mutex used by the communication sequence
 */
void UdpBridge_create(UdpBridge *self, uint16_t destinationPort, I2cInterface *i2c, MutexInterface *mutex);

/**
 * @brief Returns a UDP bridge view as a communication interface
 * 
 * @param self Initialized UDP bridge structure
 * @return CommunicationInterface* Communication interface view
 */
CommunicationInterface *UdpBridge_viewAsCommunicationInterface(UdpBridge *self);

#endif  // UDP_BRIDGE_H_
