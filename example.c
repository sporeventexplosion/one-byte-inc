#include <stdio.h>
#include "one_byte_inc.h"

int main() {
  if (init_one_byte_inc() != 0) {
    printf("Could not initialize one_byte_inc\n");
    return 1;
  }
  for (int i = 0; i < 10; i = one_byte_inc(i)) {
    printf("%d\n", i);
  }
}
