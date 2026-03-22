#include "mux_transport.h"

#include <string.h>

#include <zephyr/kernel.h>

#define MUTEX_RETRIES 5

static void _mux_on_rx(MuxChannel_t chan, const uint8_t *payload, size_t len, void *user) {
  MuxTransport *self = (MuxTransport *)user;

  if (chan != MUX_CHAN_TC_IN) {
    // Rover typically only receives TC_IN from host; ignore other inbound channels
    return;
  }

  if (len > USB_MUX_MAX_DGRAM) {
    // Drop oversize datagrams (UDP-like)
    return;
  }

  MuxTransportTcPacket pkt = {0};
  pkt.len = (uint16_t)len;
  memcpy(pkt.data, payload, len);

  // Best-effort enqueue: drop if queue is full
  (void)k_msgq_put(&self->tc_in_q, &pkt, K_NO_WAIT);
}

//******************************************************************************
// TC interface implementation
//******************************************************************************
static uint8_t _tc_send(void *instance, const uint8_t *buffer, size_t bufferSize) {
  MuxTransport *self = (MuxTransport *)instance;

  uint8_t retries = MUTEX_RETRIES;
  while (MutexInterface_acquire(self->tx_lock) == false) {  // Wait until the lock is available
    if (retries-- == 0) {
      return 1;  // Failed to acquire lock after several tries: give up on this frame
    }
  }

  bool ok = UdpSerialMux_send(&self->mux, MUX_CHAN_TC_RSP, buffer, bufferSize);
  MutexInterface_release(self->tx_lock);

  return ok ? 0 : 1;
}

static uint8_t _tc_receive(void *instance, uint8_t *buffer, size_t bufferSize, size_t *receivedSize) {
  MuxTransport *self = (MuxTransport *)instance;

  if (receivedSize == NULL) return 1;
  *receivedSize = 0;

  // If we already have a queued TC datagram, return it immediately
  MuxTransportTcPacket pkt;
  if (k_msgq_get(&self->tc_in_q, &pkt, K_NO_WAIT) == 0) {
    if (pkt.len > bufferSize) {
      // Caller buffer too small: safest is to drop and signal error
      return 2;
    }
    memcpy(buffer, pkt.data, pkt.len);
    *receivedSize = pkt.len;
    return 0;
  }

  // If no queued TC yet. Pump the mux once to pull bytes from CDC and parse frames
  // Only TC thread should call this receive() to avoid RX races
  UdpSerialMux_pump(&self->mux);

  // Try again after pumping.
  if (k_msgq_get(&self->tc_in_q, &pkt, K_NO_WAIT) == 0) {
    if (pkt.len > bufferSize) {
      return 2;
    }
    memcpy(buffer, pkt.data, pkt.len);
    *receivedSize = pkt.len;
    return 0;
  }

  // Still nothing available: return 0 bytes, success
  return 0;
}

//******************************************************************************
// TM interface implementation
//******************************************************************************
static uint8_t _tm_send(void *instance, const uint8_t *buffer, size_t bufferSize) {
  MuxTransport *self = (MuxTransport *)instance;

  uint8_t retries = MUTEX_RETRIES;
  while (MutexInterface_acquire(self->tx_lock) == false) {  // Wait until the lock is available
    if (retries-- == 0) {
      return 1;  // Failed to acquire lock after several tries: give up on this frame
    }
  }

  bool ok = UdpSerialMux_send(&self->mux, MUX_CHAN_TM_OUT, buffer, bufferSize);
  MutexInterface_release(self->tx_lock);

  return ok ? 0 : 1;
}

static uint8_t _tm_receive(void *instance, uint8_t *buffer, size_t bufferSize, size_t *receivedSize) {
  (void)instance;
  (void)buffer;
  (void)bufferSize;

  if (receivedSize == NULL) return 1;
  *receivedSize = 0;

  // Telemetry thread should not consume RX from the shared stream
  return 0;
}

//******************************************************************************
// Public methods
//******************************************************************************
uint8_t MuxTransport_create(MuxTransport *self, CommunicationInterface *io, MutexInterface *tx_lock) {
  if (self == NULL) return 1;

  self->tx_lock = tx_lock;

  // Init TC queue
  k_msgq_init(&self->tc_in_q, self->tc_in_q_storage, sizeof(MuxTransportTcPacket), MUX_TRANSPORT_TC_QUEUE_DEPTH);

  // Init mux over the byte stream
  UdpSerialMux_create(&self->mux, io);

  // Register RX handler for TC_IN frames
  UdpSerialMux_setHandler(&self->mux, MUX_CHAN_TC_IN, _mux_on_rx, self);

  // Create two CommunicationInterface "views"
  self->tc_iface.instance = self;
  self->tc_iface.send = _tc_send;
  self->tc_iface.receive = _tc_receive;

  self->tm_iface.instance = self;
  self->tm_iface.send = _tm_send;
  self->tm_iface.receive = _tm_receive;

  return 0;
}

CommunicationInterface *MuxTransport_tcInterface(MuxTransport *self) {
  return &self->tc_iface;
}

CommunicationInterface *MuxTransport_tmInterface(MuxTransport *self) {
  return &self->tm_iface;
}
