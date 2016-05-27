// Copyright (c) 2016 AlertAvert.com. All rights reserved.
// Created by M. Massenzio (marco@alertavert.com) on 5/26/16.

#include <magic.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>


void usage() {
  printf("Usage: file [--mime] FILENAME\n\n"
             "Attempts to deduct the type of the file passed as the first argument\n\n"
             "\t--mime\tOptional flag to emit the MIME type, instead of the textual description\n"
             "\tFILENAME\tThe name of the file whose type we want to deduct\n");
}


int main(int argc, const char* argv[]) {
  int flags = MAGIC_CONTINUE | MAGIC_ERROR;
  const char* filename;
  char* mime = "";

  if (argc < 2 || argc > 3) {
    usage();
    return EXIT_FAILURE;
  }
  if (strcmp(argv[1], "--mime") == 0) {
    flags |= MAGIC_MIME;
    filename = argv[2];
    mime = " MIME";
  } else {
    filename = argv[1];
  }

  magic_t cookie = magic_open(flags);
  if (!cookie) {
    printf("Error (%d) opening magic library: %s", magic_errno(cookie), magic_error(cookie));
    return EXIT_FAILURE;
  }

  // This is REQUIRED - the docs do not make it sound like it is; but it is.
  magic_load(cookie, NULL);

  const char *result = magic_file(cookie, filename);
  printf("The%s type for '%s' is: %s\n", mime, filename, result);

  // This must be closed AFTER using `result` or it will be de-allocated.
  magic_close(cookie);

  return EXIT_SUCCESS;
}
