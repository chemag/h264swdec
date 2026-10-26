#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "h264bsd_stream.h"
#include "h264bsd_util.h"
#include "h264bsd_vlc.h"

struct ExpGolombCase {
  const char* bits;
  u32 expected;
};

/* Packs a string of '0' and '1' characters into bytes, most significant bit
 * first, and pads with 0xff so that bit reads past the code stay in bounds. */
static size_t PackBits(const char* bits, u8* buffer, size_t capacity) {
  size_t count = strlen(bits);
  size_t size = (count + 7U) / 8U + 8U;
  size_t index;

  if (size > capacity) {
    return 0;
  }
  memset(buffer, 0xff, size);
  for (index = 0; index < count; ++index) {
    u8 mask = (u8)(0x80U >> (index % 8U));
    if (bits[index] == '0') {
      buffer[index / 8U] &= (u8)~mask;
    }
  }
  return size;
}

static void Repeat(char* out, char bit, size_t count) {
  memset(out, bit, count);
  out[count] = '\0';
}

int main(void) {
  char long_ones[64];
  char long_zeros[64];
  struct ExpGolombCase cases[5];
  size_t index;

  /* 31 leading zeros, the separator, then 31 info bits. Before the fix,
   * computing 1 << 31 in int was undefined behavior. */
  Repeat(long_ones, '0', 31);
  strcat(long_ones, "1");
  Repeat(long_ones + 32, '1', 31);
  Repeat(long_zeros, '0', 31);
  strcat(long_zeros, "1");
  Repeat(long_zeros + 32, '0', 31);

  cases[0].bits = "1";
  cases[0].expected = 0;
  cases[1].bits = "010";
  cases[1].expected = 1;
  cases[2].bits = "00000111111";
  cases[2].expected = 62;
  cases[3].bits = long_zeros;
  cases[3].expected = 0x7fffffffU;
  cases[4].bits = long_ones;
  cases[4].expected = 0xfffffffeU;

  for (index = 0; index < sizeof(cases) / sizeof(cases[0]); ++index) {
    u8 buffer[32];
    strmData_t stream;
    u32 value = 0;
    size_t size = PackBits(cases[index].bits, buffer, sizeof(buffer));

    if (size == 0) {
      fprintf(stderr, "case %zu: buffer too small\n", index);
      return 1;
    }
    memset(&stream, 0, sizeof(stream));
    stream.pStrmBuffStart = buffer;
    stream.pStrmCurrPos = buffer;
    stream.strmBuffSize = (u32)size;

    if (h264bsdDecodeExpGolombUnsigned(&stream, &value) != HANTRO_OK ||
        value != cases[index].expected ||
        stream.strmBuffReadBits != strlen(cases[index].bits)) {
      fprintf(stderr, "case %zu: got %u after %u bits, expected %u\n", index,
              value, stream.strmBuffReadBits, cases[index].expected);
      return 1;
    }
  }
  return 0;
}
