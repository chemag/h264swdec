#include <stddef.h>
#include <stdio.h>

#include "h264bsd_deblocking_internal.h"

struct FilterDeltaCase {
  i32 p0;
  i32 q0;
  i32 p1;
  i32 q1;
  i32 expected;
};

int main(void) {
  static const struct FilterDeltaCase cases[] = {
      {100, 88, 90, 80, -5}, {88, 100, 80, 90, 5},    {255, 0, 255, 0, -96},
      {0, 255, 0, 255, 96},  {128, 128, 128, 128, 0},
  };
  size_t index;

  for (index = 0; index < sizeof(cases) / sizeof(cases[0]); ++index) {
    const struct FilterDeltaCase* test = &cases[index];
    i32 actual = h264bsdFilterDelta(test->p0, test->q0, test->p1, test->q1);

    if (actual != test->expected) {
      fprintf(stderr, "case %zu: expected %d, got %d\n", index, test->expected,
              actual);
      return 1;
    }
  }
  return 0;
}
