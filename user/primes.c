#include "kernel/types.h"
#include "user/user.h"

__attribute__((noreturn))
void handle(int IN) {
  uint8 prime;
  if (read(IN, &prime, sizeof(prime)) <= 0) {
    close(IN);
    exit(0);
  }
  fprintf(1, "prime %d\n", prime);
  int p[2];
  pipe(p);
  if (fork()) {
    close(p[0]);
    uint8 x;
    while (read(IN, &x, sizeof(x)))
      if (x % prime)
        write(p[1], &x, sizeof(x));
    close(IN);
    close(p[1]);
    wait(0);
    exit(0);
  } else {
    close(p[1]);
    close(IN);
    handle(p[0]);
  }
}

int main() {
  int p[2];
  pipe(p);
  for (uint8 i = 2; i <= 35; ++i)
    write(p[1], &i, sizeof(i));
  close(p[1]);
  handle(p[0]);
  exit(0);
}
