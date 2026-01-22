#include "cobs.h"

// Standard COBS
size_t cobs_encode(const uint8_t *in, size_t in_len, uint8_t *out, size_t out_cap) {
  if (!in || !out) return 0;
  if (out_cap == 0) return 0;

  size_t read_index = 0;
  size_t write_index = 1;  // first code byte placeholder
  size_t code_index = 0;
  uint8_t code = 1;

  while (read_index < in_len) {
    if (write_index >= out_cap) return 0;

    if (in[read_index] == 0) {
      out[code_index] = code;
      code = 1;
      code_index = write_index++;
      read_index++;
    } else {
      out[write_index++] = in[read_index++];
      code++;
      if (code == 0xFF) {
        out[code_index] = code;
        code = 1;
        code_index = write_index++;
        if (write_index > out_cap) return 0;
      }
    }
  }

  if (code_index >= out_cap) return 0;
  out[code_index] = code;
  return write_index;
}

size_t cobs_decode(const uint8_t *in, size_t in_len, uint8_t *out, size_t out_cap) {
  if (!in || !out) return 0;
  if (in_len == 0) return 0;

  size_t read_index = 0;
  size_t write_index = 0;

  while (read_index < in_len) {
    uint8_t code = in[read_index];
    if (code == 0) return 0;  // zero not allowed inside encoded block
    read_index++;

    for (uint8_t i = 1; i < code; i++) {
      if (read_index >= in_len) return 0;
      if (write_index >= out_cap) return 0;
      out[write_index++] = in[read_index++];
    }

    if (code != 0xFF && read_index < in_len) {
      if (write_index >= out_cap) return 0;
      out[write_index++] = 0;
    }
  }

  return write_index;
}
