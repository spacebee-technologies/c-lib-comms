#include "serial_datagram_mux.h"

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

static void handle_decoded_frame(SerialDatagramMux *m, const uint8_t *dec, size_t dec_len) {
  // Minimum: chan(1) + len(2) + crc(2) = 5
  if (dec_len < 5) {
    m->frames_decode_fail++;
    return;
  }

  uint8_t chan_u8 = dec[0];
  if (chan_u8 >= 3) {
    m->frames_decode_fail++;
    return;
  }

  uint16_t len = le16_read(&dec[1]);
  size_t expected = (size_t)1 + 2 + (size_t)len + 2;
  if (dec_len != expected) {
    m->frames_decode_fail++;
    return;
  }

  const uint8_t *payload = &dec[3];
  uint16_t crc_rx = le16_read(&dec[3 + len]);

  uint16_t crc_calc = crc16_ccitt_false(dec, 3 + len);
  if (crc_calc != crc_rx) {
    m->frames_crc_fail++;
    return;
  }

  m->frames_ok++;
  mux_rx_cb_t cb = m->rx_cb[chan_u8];
  if (cb) {
    cb((MuxChannel_t)chan_u8, payload, len, m->rx_user[chan_u8]);
  }
}

//******************************************************************************
// Public methods
//******************************************************************************
void SerialDatagramMux_create(SerialDatagramMux *m, CommunicationInterface *io) {
  m->io = io;

  for (int i = 0; i < 3; i++) {
    m->rx_cb[i] = 0;
    m->rx_user[i] = 0;
  }

  m->rx_enc_cap = sizeof(m->rx_enc_buf);
  m->rx_dec_cap = sizeof(m->rx_dec_buf);
  m->tx_enc_cap = sizeof(m->tx_enc_buf);

  m->rx_enc_len = 0;

  m->frames_ok = 0;
  m->frames_crc_fail = 0;
  m->frames_decode_fail = 0;
  m->frames_oversize = 0;
}

void SerialDatagramMux_setHandler(SerialDatagramMux *m, MuxChannel_t chan, mux_rx_cb_t cb, void *user) {
  if ((int)chan < 0 || (int)chan >= 3) return;
  m->rx_cb[(int)chan] = cb;
  m->rx_user[(int)chan] = user;
}

void SerialDatagramMux_pump(SerialDatagramMux *m) {
  // Read some bytes from the underlying stream
  uint8_t tmp[64];
  size_t got = 0;

  uint8_t ret = CommunicationInterface_receive(m->io, tmp, sizeof(tmp), &got);
  if (ret != 0 || got == 0) return;

  for (size_t i = 0; i < got; i++) {
    uint8_t b = tmp[i];

    if (b == 0x00) {
      // End of one COBS block
      if (m->rx_enc_len == 0) {
        // Ignore empty frame
        continue;
      }

      size_t dec_len = cobs_decode(m->rx_enc_buf, m->rx_enc_len, m->rx_dec_buf, m->rx_dec_cap);
      if (dec_len == 0) {
        m->frames_decode_fail++;
      } else {
        handle_decoded_frame(m, m->rx_dec_buf, dec_len);
      }

      m->rx_enc_len = 0;
      continue;
    }

    if (m->rx_enc_len >= m->rx_enc_cap) {
      // Oversize: drop until next delimiter
      m->frames_oversize++;
      // Keep discarding bytes until we see 0x00
      // Easiest: cap at capacity and just don't append anymore
      continue;
    }

    m->rx_enc_buf[m->rx_enc_len++] = b;
  }
}

bool SerialDatagramMux_send(SerialDatagramMux *m, MuxChannel_t chan, const uint8_t *payload, size_t len) {
  if ((int)chan < 0 || (int)chan >= 3) return false;
  if (!payload && len != 0) return false;
  if (len > 0xFFFF) return false;

  // Layout: chan(1) + len(2) + payload(len) + crc(2)
  size_t frame_len = 1u + 2u + len + 2u;
  if (frame_len > sizeof(m->tx_frame_buf)) return false;

  m->tx_frame_buf[0] = (uint8_t)chan;
  le16_write(&m->tx_frame_buf[1], (uint16_t)len);

  if (len > 0) {
    memcpy(&m->tx_frame_buf[3], payload, len);
  }

  uint16_t crc = crc16_ccitt_false(m->tx_frame_buf, 3u + len);
  le16_write(&m->tx_frame_buf[3 + len], crc);

  // COBS encode into tx buffer
  size_t enc_len = cobs_encode(m->tx_frame_buf, frame_len, m->tx_enc_buf, m->tx_enc_cap);
  if (enc_len == 0) return false;

  // Send encoded bytes + delimiter 0x00
  if (CommunicationInterface_send(m->io, m->tx_enc_buf, enc_len) != 0) return false;

  uint8_t delim = 0x00;
  if (CommunicationInterface_send(m->io, &delim, 1) != 0) return false;

  return true;
}
