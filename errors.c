#include <stdio.h>
#include <stdlib.h>
#include "errors.h"

char *programname;

void fatalerror(char *message) {
  fprintf(stderr, "%s: %s\n", programname, message);
  perror(programname);
  fflush(stdout);
  exit(1);
}

void errormessage(char *message) {
  fprintf(stderr, "%s: %s\n", programname, message);
  fflush(stdout);
}

