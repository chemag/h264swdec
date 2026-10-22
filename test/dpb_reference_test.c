#include <stdio.h>

#include "h264bsd_cfg.h"
#include "h264bsd_dpb.h"
#include "h264bsd_util.h"

#define TEST_REF_CAPACITY 16U

int main(void) {
  u8 first_data[32];
  u8 second_data[32];
  u8 foreign_data[32];
  dpbPicture_t foreign = {0};
  dpbPicture_t buffer[TEST_REF_CAPACITY + 1] = {0};
  dpbPicture_t* list[TEST_REF_CAPACITY + 1] = {0};
  dpbStorage_t dpb = {0};

  dpb.buffer = buffer;
  dpb.list = list;
  dpb.dpbSize = 1;
  buffer[0].pAllocatedData = first_data;
  buffer[0].data = ALIGN(first_data, 16);
  buffer[0].status = SHORT_TERM;
  buffer[1].pAllocatedData = second_data;
  buffer[1].data = ALIGN(second_data, 16);
  buffer[1].status = UNUSED;
  list[0] = &buffer[0];

  if (h264bsdGetRefPicData(&dpb, 0) != buffer[0].data) {
    fprintf(stderr, "owned reference was rejected\n");
    return 1;
  }
  if (h264bsdGetRefPicData(&dpb, TEST_REF_CAPACITY) != NULL) {
    fprintf(stderr, "out-of-range index was accepted\n");
    return 1;
  }

  foreign.pAllocatedData = foreign_data;
  foreign.data = ALIGN(foreign_data, 16);
  foreign.status = SHORT_TERM;
  list[0] = &foreign;
  if (h264bsdGetRefPicData(&dpb, 0) != NULL) {
    fprintf(stderr, "foreign picture descriptor was accepted\n");
    return 1;
  }

  list[0] = &buffer[0];
  buffer[0].data = ALIGN(foreign_data, 16);
  if (h264bsdGetRefPicData(&dpb, 0) != NULL) {
    fprintf(stderr, "foreign picture data was accepted\n");
    return 1;
  }
  return 0;
}
