#include "kernel/param.h"
#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
  if (argc < 2) {
    fprintf(2, "Usage: xargs command\n");
    exit(1);
  }
  char buf[512];
  // limit single line to 512 characters
  for (;;) {
    char *end = buf;
    int len = 0;
    while (read(0, end, sizeof(char)) == 1 && *end != '\n' && len < 511)
      ++end, ++len;
    if(*end == '\n') --end;
    *(++end) = '\0';
    if(len == 0) exit(0);
    char *p = buf;
    char *cmd = argv[1];
    int nargc = 1;
    char *args[MAXARG + 1];
    args[0] = argv[1];
    for (int i = 2; i < argc; ++i)
      args[nargc++] = argv[i];
    while (*p != '\0') {
      args[nargc++] = p;
      while (*p != ' ' && *p != '\0')
        ++p;
      if (*p == ' ' || *p == '\n')
        *p++ = '\0';
    }
    args[nargc] = 0;
    if (fork() == 0) {
      exec(cmd, args);
    } else
      wait(0);
  }
  exit(0);
}