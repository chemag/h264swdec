#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "h264bsd_reconstruct_internal.h"

struct ScaleCase {
  i32 input;
  i32 expected4;
  i32 expected16;
};

int main(void) {
  static const struct ScaleCase cases[] = {
      {0, 0, 0},
      {1, 4, 16},
      {-1, -4, -16},
      {738, 2952, 11808},
      {-738, -2952, -11808},
      {INT32_MAX / 16, (INT32_MAX / 16) * 4, (INT32_MAX / 16) * 16},
      {INT32_MIN / 16, (INT32_MIN / 16) * 4, (INT32_MIN / 16) * 16},
  };
  size_t index;

  for (index = 0; index < sizeof(cases) / sizeof(cases[0]); ++index) {
    i32 actual4 = h264bsdScale4(cases[index].input);
    i32 actual16 = h264bsdScale16(cases[index].input);

    if (actual4 != cases[index].expected4 ||
        actual16 != cases[index].expected16) {
      fprintf(stderr, "case %zu: got %d and %d\n", index, actual4, actual16);
      return 1;
    }
  }
  return 0;
}
