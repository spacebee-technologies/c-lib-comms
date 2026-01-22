#ifndef UDP_SERIAL_MUX_H_
#define UDP_SERIAL_MUX_H_

#include <stdbool.h>

#include "communication_interface.h"

typedef enum MuxChannel {
  MUX_CHAN_TC_IN  = 0,  // port 51524
  MUX_CHAN_TC_RSP = 1,  // port 51525
  MUX_CHAN_TM_OUT = 2,  // port 51526
} MuxChannel_t;

typedef void (*mux_rx_cb_t)(MuxChannel_t chan, const uint8_t *payload, size_t len, void *user);

typedef struct UdpSerialMux {
  CommunicationInterface *io;

  mux_rx_cb_t rx_cb[3];
  void *rx_user[3];

  // RX stream accumulation (COBS block until delimiter 0x00)
  uint8_t *rx_enc_buf;
  size_t rx_enc_cap;
  size_t rx_enc_len;

  // scratch buffers
  uint8_t *rx_dec_buf;
  size_t rx_dec_cap;

  uint8_t *tx_enc_buf;
  size_t tx_enc_cap;

  // stats
  uint32_t frames_ok;
  uint32_t frames_crc_fail;
  uint32_t frames_decode_fail;
  uint32_t frames_oversize;
} UdpSerialMux;

// Init with externally provided buffers to avoid malloc in embedded
void udp_serial_mux_init(UdpSerialMux *m, CommunicationInterface *io,
                         uint8_t *rx_enc_buf, size_t rx_enc_cap,
                         uint8_t *rx_dec_buf, size_t rx_dec_cap,
                         uint8_t *tx_enc_buf, size_t tx_enc_cap);

void udp_serial_mux_set_handler(UdpSerialMux *m, MuxChannel_t chan, mux_rx_cb_t cb, void *user);

// Call periodically (or in a thread) to process incoming bytes and emit frames to callbacks
void udp_serial_mux_pump(UdpSerialMux *m);

// Send one datagram on a channel (best-effort)
bool udp_serial_mux_send(UdpSerialMux *m, MuxChannel_t chan, const uint8_t *payload, size_t len);

#endif  // UDP_SERIAL_MUX_H_
