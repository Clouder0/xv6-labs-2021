#include "kernel/types.h"
#include "user/user.h"

int main() {
  int p[2];
  pipe(p);
  if (fork() == 0) {
    char buf;
    read(p[0], &buf, sizeof(buf));
    if (buf == 'A') {
      fprintf(1, "%d: received ping\n", getpid());
      write(p[1], "B", 1);
      exit(0);
    }
    exit(1);
  } else {
    write(p[1], "A", 1);
    wait(0);
    char buf;
    read(p[0], &buf, sizeof(buf));
    if (buf == 'B') {
      fprintf(1, "%d: received pong\n", getpid());
      exit(0);
    }
    exit(1);
  }
  exit(0);
}