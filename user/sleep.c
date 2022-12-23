#include "kernel/types.h"
#include "user/user.h"

void arg_error() {
  write(1, "Argument error!\n", 16);
  exit(1);
}

int main(int argc, char *argv[]) {
  if (argc != 2)
    arg_error();
  int tick = atoi(argv[1]);
  if (tick < 0)
    arg_error();
  sleep(tick);
  exit(0);
}