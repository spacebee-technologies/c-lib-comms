#include "serial_datagram_mux.h"

#include <string.h>

#include "cobs.h"
#include "crc16_ccitt.h"

//******************************************************************************
// Private helpers
//******************************************************************************
static uint16_t le16_read(const uint8_t *p) {
  return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static void le16_write(uint8_t *p, uint16_t v) {
  p[0] = (uint8_t)(v & 0xFF);
  p[1] = (uint8_t)((v >> 8) & 0xFF);
}

//******************************************************************************
// Private methods
//******************************************************************************
static void SerialDatagramMux_handleDecodedFrame(SerialDatagramMux *self, const uint8_t *dec, size_t dec_len) {
  // Minimum: chan(1) + len(2) + crc(2) = 5
  if (dec_len < 5) {
    self->frames_decode_fail++;
    return;
  }

  uint8_t chan_u8 = dec[0];
  if (chan_u8 >= 3) {
    self->frames_decode_fail++;
    return;
  }

  uint16_t len = le16_read(&dec[1]);
  size_t expected = (size_t)1 + 2 + (size_t)len + 2;
  if (dec_len != expected) {
    self->frames_decode_fail++;
    return;
  }

  const uint8_t *payload = &dec[3];
  uint16_t crc_rx = le16_read(&dec[3 + len]);

  uint16_t crc_calc = crc16_ccitt_false(dec, 3 + len);
  if (crc_calc != crc_rx) {
    self->frames_crc_fail++;
    return;
  }

  self->frames_ok++;
  mux_rx_cb_t cb = self->rx_cb[chan_u8];
  if (cb) {
    cb((MuxChannel_t)chan_u8, payload, len, self->rx_user[chan_u8]);
  }
}

//******************************************************************************
// Public methods
//******************************************************************************
void SerialDatagramMux_create(SerialDatagramMux *self, CommunicationInterface *io) {
  self->io = io;

  for (int i = 0; i < 3; i++) {
    self->rx_cb[i] = 0;
    self->rx_user[i] = 0;
  }

  self->rx_enc_cap = sizeof(self->rx_enc_buf);
  self->rx_dec_cap = sizeof(self->rx_dec_buf);
  self->tx_enc_cap = sizeof(self->tx_enc_buf);

  self->rx_enc_len = 0;

  self->frames_ok = 0;
  self->frames_crc_fail = 0;
  self->frames_decode_fail = 0;
  self->frames_oversize = 0;
}

void SerialDatagramMux_setHandler(SerialDatagramMux *self, MuxChannel_t chan, mux_rx_cb_t cb, void *user) {
  if ((int)chan < 0 || (int)chan >= 3) return;
  self->rx_cb[(int)chan] = cb;
  self->rx_user[(int)chan] = user;
}

void SerialDatagramMux_pump(SerialDatagramMux *self) {
  // Read some bytes from the underlying stream
  uint8_t tmp[64];
  size_t got = 0;

  uint8_t ret = CommunicationInterface_receive(self->io, tmp, sizeof(tmp), &got);
  if (ret != 0 || got == 0) return;

  for (size_t i = 0; i < got; i++) {
    uint8_t b = tmp[i];

    if (b == 0x00) {
      // End of one COBS block
      if (self->rx_enc_len == 0) {
        // Ignore empty frame
        continue;
      }

      size_t dec_len = cobs_decode(self->rx_enc_buf, self->rx_enc_len, self->rx_dec_buf, self->rx_dec_cap);
      if (dec_len == 0) {
        self->frames_decode_fail++;
      } else {
        SerialDatagramMux_handleDecodedFrame(self, self->rx_dec_buf, dec_len);
      }

      self->rx_enc_len = 0;
      continue;
    }

    if (self->rx_enc_len >= self->rx_enc_cap) {
      // Oversize: drop until next delimiter
      self->frames_oversize++;
      // Keep discarding bytes until we see 0x00
      // Easiest: cap at capacity and just don't append anymore
      continue;
    }

    self->rx_enc_buf[self->rx_enc_len++] = b;
  }
}

bool SerialDatagramMux_send(SerialDatagramMux *self, MuxChannel_t chan, const uint8_t *payload, size_t len) {
  if ((int)chan < 0 || (int)chan >= 3) return false;
  if (!payload && len != 0) return false;
  if (len > 0xFFFF) return false;

  // Layout: chan(1) + len(2) + payload(len) + crc(2)
  size_t frame_len = 1u + 2u + len + 2u;
  if (frame_len > sizeof(self->tx_frame_buf)) return false;

  self->tx_frame_buf[0] = (uint8_t)chan;
  le16_write(&self->tx_frame_buf[1], (uint16_t)len);

  if (len > 0) {
    memcpy(&self->tx_frame_buf[3], payload, len);
  }

  uint16_t crc = crc16_ccitt_false(self->tx_frame_buf, 3u + len);
  le16_write(&self->tx_frame_buf[3 + len], crc);

  // COBS encode into tx buffer
  size_t enc_len = cobs_encode(self->tx_frame_buf, frame_len, self->tx_enc_buf, self->tx_enc_cap);
  if (enc_len == 0) return false;

  // Send encoded bytes + delimiter 0x00
  if (CommunicationInterface_send(self->io, self->tx_enc_buf, enc_len) != 0) return false;

  uint8_t delim = 0x00;
  if (CommunicationInterface_send(self->io, &delim, 1) != 0) return false;

  return true;
}
