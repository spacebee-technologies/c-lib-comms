#ifndef COBS_H_
#define COBS_H_

#include <stddef.h>
#include <stdint.h>

/**
 * @brief Returns encoded length on success, 0 on failure (out buffer too small)
 */
size_t cobs_encode(const uint8_t *in, size_t in_len, uint8_t *out, size_t out_cap);

/**
 * @brief Returns decoded length on success, 0 on failure
 */
size_t cobs_decode(const uint8_t *in, size_t in_len, uint8_t *out, size_t out_cap);

#endif  // COBS_H_
