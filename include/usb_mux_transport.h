// TODO: Rename as CommsMuxTransport?

#ifndef USB_MUX_TRANSPORT_H_
#define USB_MUX_TRANSPORT_H_

#include <stddef.h>
#include <stdint.h>

#include <zephyr/kernel.h>

#include "communication_interface.h"
#include "udp_serial_mux.h"
#include "mutex_interface.h"

#define USB_MUX_TC_QUEUE_DEPTH 8

// Internal packet stored in the telecommand queue
typedef struct {
  uint16_t len;
  uint8_t data[USB_MUX_MAX_DGRAM];
} UsbMuxTcPacket;

typedef struct {
  // Shared mux over the single byte stream
  UdpSerialMux mux;

  // Serialize TX from multiple threads
  MutexInterface *tx_lock;

  // Queue for inbound telecommands
  struct k_msgq tc_in_q;
  uint8_t tc_in_q_storage[USB_MUX_TC_QUEUE_DEPTH * sizeof(UsbMuxTcPacket)];

  // Two CommunicationInterface "views"
  CommunicationInterface tc_iface;
  CommunicationInterface tm_iface;
} UsbMuxTransport;

/**
 * @brief Initialize transport, enable USB, create the mux, and expose two interfaces:
 *        - tc_iface: RX(TC_IN) + TX(TC_RSP)
 *        - tm_iface: TX(TM_OUT) only
 *
 * @return 0 on success, non-zero on failure.
 */
uint8_t UsbMuxTransport_create(UsbMuxTransport *self, CommunicationInterface *io, MutexInterface *tx_lock);

/**
 * @brief Get interface for telecommand thread:
 *        - receive(): returns one TC_IN datagram payload (former UDP:51524)
 *        - send(): sends TC_RSP payload (former UDP:51525)
 */
CommunicationInterface *UsbMuxTransport_tcInterface(UsbMuxTransport *self);

/**
 * @brief Get interface for telemetry thread:
 *        - send(): sends TM payload (former UDP:51526)
 *        - receive(): always returns 0 bytes
 */
CommunicationInterface *UsbMuxTransport_tmInterface(UsbMuxTransport *self);

#endif  // USB_MUX_TRANSPORT_H_
