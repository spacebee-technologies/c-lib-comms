#ifndef MUX_TRANSPORT_H_
#define MUX_TRANSPORT_H_

#include <stddef.h>
#include <stdint.h>

#include <zephyr/kernel.h>

#include "communication_interface.h"
#include "serial_datagram_mux.h"
#include "mutex_interface.h"

#define MUX_TRANSPORT_TC_QUEUE_DEPTH 8

// Internal packet stored in the telecommand queue
typedef struct {
  uint16_t len;
  uint8_t data[SERIAL_DATAGRAM_MUX_MAX_PAYLOAD];
} MuxTransportTcPacket;

typedef struct {
  // Shared mux over the single byte stream
  SerialDatagramMux mux;

  // Serialize TX from multiple threads
  MutexInterface *tx_lock;

  // Queue for inbound telecommands
  struct k_msgq tc_in_q;
  uint8_t tc_in_q_storage[MUX_TRANSPORT_TC_QUEUE_DEPTH * sizeof(MuxTransportTcPacket)];

  // Two CommunicationInterface "views"
  CommunicationInterface tc_iface;
  CommunicationInterface tm_iface;
} MuxTransport;

/**
 * @brief Initialize transport and expose two interfaces, one for telemetries
 *        and one for telecommands, multiplexed over the same underlying byte stream
 *
 * @return 0 on success, non-zero on failure.
 */
uint8_t MuxTransport_create(MuxTransport *self, CommunicationInterface *io, MutexInterface *tx_lock);

/**
 * @brief Get interface for telecommand comms
 */
CommunicationInterface *MuxTransport_tcInterface(MuxTransport *self);

/**
 * @brief Get interface for telemetry comms
 */
CommunicationInterface *MuxTransport_tmInterface(MuxTransport *self);

#endif  // MUX_TRANSPORT_H_
