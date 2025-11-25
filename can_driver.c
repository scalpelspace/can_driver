/*******************************************************************************
 * @file can_driver.c
 * @brief Low level simplified CAN bus communication drivers.
 *******************************************************************************
 */

/** Includes. *****************************************************************/

#include "can_driver.h"
#include "math.h"

/** Private functions. ********************************************************/

static int32_t sign_extend_u32(const uint32_t raw_value,
                               const uint8_t length_bits) {
  if (length_bits == 0 || length_bits >= 32)
    return (int32_t)raw_value;
  const uint32_t shift = 32u - length_bits;
  return (int32_t)((raw_value << shift) >> shift); // Right shift sign-extend.
}

static uint32_t clamp_raw(double physical_value, const uint8_t length_bits,
                          const bool is_signed) {
  const double low = is_signed ? -(double)(1u << (length_bits - 1)) : 0.0;
  const double high =
      is_signed ? (double)((1u << (length_bits - 1)) - 1u)
                : (double)((length_bits == 32) ? 0xFFFFFFFFu
                                               : ((1u << length_bits) - 1u));
  if (physical_value < low)
    physical_value = low;
  if (physical_value > high)
    physical_value = high;
  const long long r = llround(physical_value); // Round before cast.
  if (r < 0) {
    return (uint32_t)r; // Two's complement bit pattern is preserved by cast.
  }
  return (uint32_t)r;
}

static double raw_to_physical(const uint32_t raw_value,
                              const can_signal_t *signal) {
  const int32_t v = signal->is_signed
                        ? sign_extend_u32(raw_value, signal->bit_length)
                        : (int32_t)raw_value;
  return (double)v * signal->scale + signal->offset;
}

/** Public functions. *********************************************************/

uint32_t physical_to_raw(double physical_value, const can_signal_t *signal) {
  if (signal->scale == 0.0) {
    return 0;
  }

  // Clamp physical value to expected range.
  if (physical_value < signal->min_value)
    physical_value = signal->min_value;
  if (physical_value > signal->max_value)
    physical_value = signal->max_value;
  const double scaled = (physical_value - signal->offset) / signal->scale;
  return clamp_raw(scaled, signal->bit_length, signal->is_signed);
}

void pack_signal_raw32(const can_signal_t *signal, uint8_t *data,
                       uint32_t raw_value) {
  if (!signal || !data)
    return;
  if (signal->bit_length == 0)
    return;

  // Mask raw_value to width (safe when bit_length == 32).
  if (signal->bit_length < 32) {
    raw_value &= (uint32_t)((1u << signal->bit_length) - 1u);
  }

  for (uint32_t i = 0; i < signal->bit_length; ++i) {
    uint32_t byte_index, bit_index; // bit_index: 0 = LSB of that byte.

    if (signal->byte_order == CAN_LITTLE_ENDIAN) {
      // Intel: linear walk across payload.
      const uint32_t pos = signal->start_bit + i;
      byte_index = pos >> 3;
      bit_index = pos & 7u;
    } else {
      // Motorola: MSB-first walk within a byte, then previous byte.
      const uint32_t start_byte = signal->start_bit >> 3;
      const uint32_t start_bit_in_byte = 7u - (signal->start_bit & 7u); // 0..7.
      const uint32_t msb_walk = start_bit_in_byte + i;
      byte_index = start_byte - (msb_walk >> 3);
      bit_index = 7u - (msb_walk & 7u);
    }

    const uint8_t bit = (uint8_t)((raw_value >> i) & 1u);

    // Clear then set (safe if buffer is reused between frames).
    data[byte_index] =
        (uint8_t)((data[byte_index] & (uint8_t)~(1u << bit_index)) |
                  (uint8_t)(bit << bit_index));
  }
}

double decode_signal(const can_signal_t *signal, const uint8_t *data) {
  if (!signal || !data)
    return 0.0f;
  if (signal->bit_length == 0)
    return 0.0f;

  uint32_t raw_value = 0;

  for (uint32_t i = 0; i < signal->bit_length; ++i) {
    uint32_t byte_index, bit_index; // bit_index: 0 = LSB of that byte.

    if (signal->byte_order == CAN_LITTLE_ENDIAN) {
      // Intel: linear walk across payload.
      const uint32_t pos = signal->start_bit + i;
      byte_index = pos >> 3;
      bit_index = pos & 7u;
    } else {
      // Motorola: MSB-first walk within a byte, then previous byte.
      const uint32_t start_byte = signal->start_bit >> 3;
      const uint32_t start_bit_in_byte = 7u - (signal->start_bit & 7u); // 0..7.
      const uint32_t msb_walk = start_bit_in_byte + i;
      byte_index = start_byte - (msb_walk >> 3);
      bit_index = 7u - (msb_walk & 7u);
    }

    const uint32_t bit = (uint32_t)((data[byte_index] >> bit_index) & 1u);
    raw_value |= (bit << i); // Assemble raw LSB-first.
  }

  // Phys = raw * scale + offset.
  return raw_to_physical(raw_value, signal);
}
