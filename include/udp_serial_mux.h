#ifndef UDP_SERIAL_MUX_H_
#define UDP_SERIAL_MUX_H_

#include <stdbool.h>

#include "communication_interface.h"

typedef enum MuxChannel {
  MUX_CHAN_TC_IN  = 0,
  MUX_CHAN_TC_RSP = 1,
  MUX_CHAN_TM_OUT = 2,
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

/**
 * @brief Initialize a new UDP serial mux instance
 *
 * @param m Uninitialized mux struct
 * @param io Communication interface to underlying transport
 * @param rx_enc_buf Buffer for accumulating incoming COBS encoded frames
 * @param rx_enc_cap Capacity of rx_enc_buf
 * @param rx_dec_buf Buffer for decoded frames
 * @param rx_dec_cap Capacity of rx_dec_buf
 * @param tx_enc_buf Buffer for encoding outgoing frames
 * @param tx_enc_cap Capacity of tx_enc_buf
 */
void UdpSerialMux_create(UdpSerialMux *m, CommunicationInterface *io,
                         uint8_t *rx_enc_buf, size_t rx_enc_cap,
                         uint8_t *rx_dec_buf, size_t rx_dec_cap,
                         uint8_t *tx_enc_buf, size_t tx_enc_cap);

/**
 * @brief Register callback for received frames on a channel
 *
 * @param m Initialized mux struct
 * @param chan Channel to register callback for
 * @param cb Callback to call when frame received on channel
 * @param user User data pointer to pass to callback
 */
void UdpSerialMux_setHandler(UdpSerialMux *m, MuxChannel_t chan, mux_rx_cb_t cb, void *user);

/**
 * @brief Call periodically (or in a thread) to process incoming bytes and emit frames to callbacks
 *
 * @param m Initialized mux struct
 */
void UdpSerialMux_pump(UdpSerialMux *m);

/**
 * @brief Send one datagram on a channel (best-effort)
 *
 * @param m Initialized mux struct
 * @param chan Channel to send on
 * @param payload Data to send
 * @param len Length of data to send
 * @return true if frame was successfully encoded and sent
 * @return false on error
 */
bool UdpSerialMux_send(UdpSerialMux *m, MuxChannel_t chan, const uint8_t *payload, size_t len);

#endif  // UDP_SERIAL_MUX_H_
