#include "udp_serial_mux.h"

#include "cobs.h"
#include "crc16_ccitt.h"

static uint16_t le16_read(const uint8_t *p) {
  return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static void le16_write(uint8_t *p, uint16_t v) {
  p[0] = (uint8_t)(v & 0xFF);
  p[1] = (uint8_t)((v >> 8) & 0xFF);
}

void udp_serial_mux_init(UdpSerialMux *m, CommunicationInterface *io,
                         uint8_t *rx_enc_buf, size_t rx_enc_cap,
                         uint8_t *rx_dec_buf, size_t rx_dec_cap,
                         uint8_t *tx_enc_buf, size_t tx_enc_cap) {
  m->io = io;

  for (int i = 0; i < 3; i++) {
    m->rx_cb[i] = 0;
    m->rx_user[i] = 0;
  }

  m->rx_enc_buf = rx_enc_buf;
  m->rx_enc_cap = rx_enc_cap;
  m->rx_enc_len = 0;

  m->rx_dec_buf = rx_dec_buf;
  m->rx_dec_cap = rx_dec_cap;

  m->tx_enc_buf = tx_enc_buf;
  m->tx_enc_cap = tx_enc_cap;

  m->frames_ok = 0;
  m->frames_crc_fail = 0;
  m->frames_decode_fail = 0;
  m->frames_oversize = 0;
}

void udp_serial_mux_set_handler(UdpSerialMux *m, MuxChannel_t chan, mux_rx_cb_t cb, void *user) {
  if ((int)chan < 0 || (int)chan >= 3) return;
  m->rx_cb[(int)chan] = cb;
  m->rx_user[(int)chan] = user;
}

static void handle_decoded_frame(UdpSerialMux *m, const uint8_t *dec, size_t dec_len) {
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

void udp_serial_mux_pump(UdpSerialMux *m) {
  // Read some bytes from the underlying stream
  uint8_t tmp[64];
  size_t got = 0;

  uint8_t ret = m->io->receive(m->io->instance, tmp, sizeof(tmp), &got);
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

bool udp_serial_mux_send(UdpSerialMux *m, MuxChannel_t chan, const uint8_t *payload, size_t len) {
  if ((int)chan < 0 || (int)chan >= 3) return false;
  if (!payload && len != 0) return false;
  if (len > 0xFFFF) return false;

  // Build decoded frame into rx_dec_buf temporarily (reuse as scratch)
  // Layout: chan(1) + len(2) + payload(len) + crc(2)
  size_t dec_len = 1 + 2 + len + 2;
  if (dec_len > m->rx_dec_cap) return false;

  m->rx_dec_buf[0] = (uint8_t)chan;
  le16_write(&m->rx_dec_buf[1], (uint16_t)len);
  for (size_t i = 0; i < len; i++) {
    m->rx_dec_buf[3 + i] = payload[i];
  }

  uint16_t crc = crc16_ccitt_false(m->rx_dec_buf, 3 + len);
  le16_write(&m->rx_dec_buf[3 + len], crc);

  // COBS encode into tx buffer
  size_t enc_len = cobs_encode(m->rx_dec_buf, dec_len, m->tx_enc_buf, m->tx_enc_cap);
  if (enc_len == 0) return false;

  // Send encoded bytes + delimiter 0x00
  if (m->io->send(m->io->instance, m->tx_enc_buf, enc_len) != 0) return false;
  uint8_t delim = 0x00;
  if (m->io->send(m->io->instance, &delim, 1) != 0) return false;

  return true;
}
