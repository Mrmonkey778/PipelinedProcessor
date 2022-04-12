
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "encode.h"
#include "parser.h"
#include "writer.h"

static void usage(char *name)
{
  printf("Usage: %s [input source]\n\
where:\n\
\t[input source] is a file containing assembly source code.\n\
", name);
  exit(1);
}


int main( int argc, char *argv[] )
{
  struct line* llh;
  uint32_t *text_segment, *data_segment;
  size_t prog_sz;

  if ( argc < 2 ) usage(argv[0]);

  llh = get_lines(argv[1]);
  if (!llh) {
    fprintf(stderr, "Error getting the lines of file: %s\n", argv[1]);
    exit(1);
  }

  print_lines(llh);

  data_segment = malloc(sizeof(uint32_t)*DATA_SEGMENT_WORDS);
  text_segment = malloc(sizeof(uint32_t)*TEXT_SEGMENT_WORDS);

  if (data_segment == NULL || text_segment == NULL) {
    fprintf(stderr, "Uh oh, looks like we ran out of memory!\n");
    exit(1);
  }

  /* TODO: convert the lines in llh into data and text segment binary
   * representations */
  encode(llh, (uint8_t*)data_segment, (uint8_t*)text_segment);

  prog_sz = write_program("a.mxe", text_segment, data_segment);
  assert(prog_sz == DATA_SEGMENT_WORDS+TEXT_SEGMENT_WORDS);

  free_lines(llh);

  return 0;
}

