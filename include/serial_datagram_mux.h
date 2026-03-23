#ifndef SERIAL_DATAGRAM_MUX_H_
#define SERIAL_DATAGRAM_MUX_H_

#include <stdbool.h>

#include "communication_interface.h"
#include "cobs.h"

#define SERIAL_DATAGRAM_MUX_MAX_PAYLOAD 1024
#define SERIAL_DATAGRAM_MUX_FRAME_OVERHEAD 5  // chan(1) + len(2) + crc(2)

#define SERIAL_DATAGRAM_MUX_MAX_FRAME (SERIAL_DATAGRAM_MUX_MAX_PAYLOAD + SERIAL_DATAGRAM_MUX_FRAME_OVERHEAD)
#define SERIAL_DATAGRAM_MUX_MAX_ENCODED (SERIAL_DATAGRAM_MUX_MAX_FRAME + COBS_MAX_OVERHEAD(SERIAL_DATAGRAM_MUX_MAX_FRAME))

typedef enum MuxChannel {
  MUX_CHAN_TC_IN  = 0,
  MUX_CHAN_TC_RSP = 1,
  MUX_CHAN_TM_OUT = 2,
} MuxChannel_t;

typedef void (*mux_rx_cb_t)(MuxChannel_t chan, const uint8_t *payload, size_t len, void *user);

typedef struct SerialDatagramMux {
  CommunicationInterface *io;

  mux_rx_cb_t rx_cb[3];
  void *rx_user[3];

  uint8_t rx_enc_buf[SERIAL_DATAGRAM_MUX_MAX_ENCODED];
  uint8_t rx_dec_buf[SERIAL_DATAGRAM_MUX_MAX_FRAME];
  uint8_t tx_enc_buf[SERIAL_DATAGRAM_MUX_MAX_ENCODED];
  uint8_t tx_frame_buf[SERIAL_DATAGRAM_MUX_MAX_FRAME];

  size_t rx_enc_len;
  size_t rx_enc_cap;
  size_t rx_dec_cap;
  size_t tx_enc_cap;

  // stats
  uint32_t frames_ok;
  uint32_t frames_crc_fail;
  uint32_t frames_decode_fail;
  uint32_t frames_oversize;
} SerialDatagramMux;

/**
 * @brief Initialize a new serial datagram mux instance
 *
 * @param self Uninitialized mux struct
 * @param io Communication interface to underlying transport
 */
void SerialDatagramMux_create(SerialDatagramMux *self, CommunicationInterface *io);

/**
 * @brief Register callback for received frames on a channel
 *
 * @param self Initialized mux struct
 * @param chan Channel to register callback for
 * @param cb Callback to call when frame received on channel
 * @param user User data pointer to pass to callback
 */
void SerialDatagramMux_setHandler(SerialDatagramMux *self, MuxChannel_t chan, mux_rx_cb_t cb, void *user);

/**
 * @brief Call periodically (or in a thread) to process incoming bytes and emit frames to callbacks
 *
 * @param m Initialized mux struct
 */
void SerialDatagramMux_pump(SerialDatagramMux *self);

/**
 * @brief Send one datagram on a channel (best-effort)
 *
 * @param self Initialized mux struct
 * @param chan Channel to send on
 * @param payload Data to send
 * @param len Length of data to send
 * @return true if frame was successfully encoded and sent
 * @return false on error
 */
bool SerialDatagramMux_send(SerialDatagramMux *self, MuxChannel_t chan, const uint8_t *payload, size_t len);

#endif  // SERIAL_DATAGRAM_MUX_H_
