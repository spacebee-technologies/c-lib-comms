#include "udp_bridge.h"

#include "logger.h"
#include "zephyr_time.h"

#define UDP_BRIDGE_ADDRESS 0x50

//******************************************************************************
// Interface implementations prototypes
//******************************************************************************
static uint8_t UdpBridge_send(void *self, const uint8_t *buffer, size_t bufferSize);
static uint8_t UdpBridge_receive(void *self, uint8_t *buffer, size_t bufferSize, size_t *receivedSize);

//******************************************************************************
// Private methods
//******************************************************************************
static void UdpBridge_initializeInterface(UdpBridge *self) {
  self->communicationInterfaceView.instance = self;
  self->communicationInterfaceView.send = UdpBridge_send;
  self->communicationInterfaceView.receive = UdpBridge_receive;
}

//******************************************************************************
// Interface implementations
//******************************************************************************
/**
 * @brief Sends a message over the UDP bridge
 *
 * @param self Initialized UDP bridge structure
 * @param buffer Contains the raw packet to be sent
 * @param bufferSize Size of the buffer
 * @return uint8_t 0 on success, 1 if error
 */
static uint8_t UdpBridge_send(void *self, const uint8_t *buffer, size_t bufferSize) {
  UdpBridge *_self = (UdpBridge *)self;
  if (MutexInterface_acquire(_self->mutex)) {
    uint8_t startingSequenceMessage[3];
    startingSequenceMessage[0] = 'S';
    startingSequenceMessage[1] = (_self->destinationPort & 0xFF00) >> 8;
    startingSequenceMessage[2] = _self->destinationPort & 0xFF;
    if (I2cInterface_sendMessage(_self->i2c, startingSequenceMessage, 3, UDP_BRIDGE_ADDRESS)) {
      LOG_ERROR("Error sending message");
    }
    uint8_t status = 0;
    if (I2cInterface_sendMessage(_self->i2c, buffer, bufferSize, UDP_BRIDGE_ADDRESS)) {
      status = 1;
    } else {
      status = 0;
    }
    MutexInterface_release(_self->mutex);
    return status;
  } else {
    return 1;
  }
}

/**
 * @brief Receives a message over the UDP bridge
 *
 * @param self Initialized UDP bridge structure
 * @param buffer Pointer to where the received packet should be allocated
 * @param bufferSize Size of the buffer
 * @param receivedSize Size of the received packet
 * @return uint8_t 0 on success, 1 if error
 */
static uint8_t UdpBridge_receive(void *self, uint8_t *buffer, size_t bufferSize, size_t *receivedSize) {
  UdpBridge *_self = (UdpBridge *)self;
  if (MutexInterface_acquire(_self->mutex)) {
    // Sends "P" which is the starting byte to request for a packet length
    uint8_t startSequenceByte[1] = { "P" };
    if (I2cInterface_sendMessage(_self->i2c, startSequenceByte, 1, UDP_BRIDGE_ADDRESS)) {
      LOG_ERROR("Error polling for packet size");
    }
    uint8_t packetSize = 0;
    // Reads last UDP bridge packet size
    uint8_t packetSizeBuffer[1];
    ZephyrTime_busySleepMicro(1000);
    if (I2cInterface_readMessage(_self->i2c, packetSizeBuffer, 1, UDP_BRIDGE_ADDRESS)) {
      MutexInterface_release(_self->mutex);
      return 1;
    }
    packetSize = packetSizeBuffer[0];
    *receivedSize = packetSize;
    if (packetSize > bufferSize) {
      MutexInterface_release(_self->mutex);
      return 1;
    }
    if (packetSize > 0) {
      ZephyrTime_busySleepMicro(1000);
      if (I2cInterface_readMessage(_self->i2c, buffer, packetSize, UDP_BRIDGE_ADDRESS)) {
        LOG_ERROR("Error reading packet from UDP bridge");
        MutexInterface_release(_self->mutex);
        return 1;
      }
    } else {
      MutexInterface_release(_self->mutex);
      return 1;
    }
    MutexInterface_release(_self->mutex);
    return 0;
  } else {
    return 1;
  }
}

//******************************************************************************
// Public methods
//******************************************************************************
void UdpBridge_create(UdpBridge *self, uint16_t destinationPort, I2cInterface *i2c, MutexInterface *mutex) {
  UdpBridge_initializeInterface(self);
  self->destinationPort = destinationPort;
  self->i2c = i2c;
  self->mutex = mutex;
}

CommunicationInterface *UdpBridge_viewAsCommunicationInterface(UdpBridge *self) {
  return &self->communicationInterfaceView;
}
