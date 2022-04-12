/*
 * Copyright (C) 2021 Regents of University of Colorado
 * Written by Gedare Bloom <gbloom@uccs.edu>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "parser.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEBUG

//#define VERBOSE

/* Private Helpers */

/* delimiters between tokens in the assembly syntax */
#define DELIMITERS " ,\t"

char *directives[NUM_DIRECTIVES] = {
  ".align",
  ".asciiz",
  ".data",
  ".space",
  ".text",
  ".word"
};

char *instructions[NUM_INSTS] = {
  "add",
  "addi",
  "and",
  "andi",
  "auipc",
  "beq",
  "bne",
  "jal",
  "jalr",
  "lui",
  "lw",
  "or",
  "ori",
  "slt",
  "slti",
  "sll",
  "slli",
  "sra",
  "srai",
  "srl",
  "srli",
  "sub",
  "sw",
  "xor",
  "xori",
  "ecall",
  /* Psuedoinstructions */
  "j",
  "la",
  "li",
  "mv",
  "neg",
  "nop",
  "not",
  "ret"
};


void free_token_list(struct token_node *token_listhead)
{
  struct token_node *curr = token_listhead, *next;

  while (curr != NULL) {
    next = curr->next;
    free(curr);
    curr = next;
  }
}

static void strip_comments(char *s, size_t len)
{
  int i;
  for (i = 0; i < len; i++) {
    if (s[i] == '#') s[i] = 0;
    if (s[i] == 0) break;
  }
}

/**
 * Reads the next line from the file stream @in.
 *
 * Returns an allocated line or NULL if no more lines or error occured.
 */
static struct line* get_next_line(FILE *in)
{
  int i, sh;
  unsigned int end;
  struct line* next = calloc(1, sizeof(struct line));
  struct token_node *tn, *curr;
  char *linebuf = NULL;
  size_t linesz = 0;
  char *token = NULL;

#ifdef DEBUG
  assert(next);
#endif
  if (!next) return NULL;

  /* Find start of the next line. Eat whitespace and pick up label if any */
  while (token == NULL) {
    if (linebuf) {
      free(linebuf);
      linebuf = NULL;
      linesz = 0;
    }
    if (getline(&linebuf, &linesz, in) <= 0) {
      free(next);
      return NULL;
    }
    end = strnlen(linebuf, linesz);
    while (end > 0 && (linebuf[end-1] == '\n' || linebuf[end-1] == '\r')) {
      linebuf[--end] = 0; // eat newline
    }
    strip_comments(linebuf, linesz);
    token = strtok(linebuf, DELIMITERS);

    /* Check for a label. Only keep one label. */
    if (token && token[strlen(token)-1] == ':') {
      if (next->label) free(next->label);
      next->label = strdup(token);
      token = strtok(NULL, DELIMITERS);
    }
  }

  /* Check for assembler directives */
  for (i = 0; i < NUM_DIRECTIVES; i++) {
    if (strncmp(token, directives[i], sizeof(directives[i])) == 0) {
      next->type = (linetype)i;
      break;
    }
  }

  if (i == NUM_DIRECTIVES) {
    /* Check for instructions */
    for (i = 0; i < NUM_INSTS; i++) {
      if (strncmp(token, instructions[i], sizeof(instructions[i])) == 0) {
        next->type = NUM_DIRECTIVES + i;
        break;
      }
    }
  }

  /* Error if token is not a directive or instruction. */
  if (i == NUM_INSTS) {
    fprintf(stderr, "Parser error, unrecognized symbol: %s\n", token);
    free(linebuf);
    free(next);
    return NULL;
  } 

  next->token_listhead = malloc(sizeof(struct token_node));
  assert(next->token_listhead);
  curr = next->token_listhead;
  curr->token = strdup(token);
  curr->next = NULL;

  while ((token = strtok(NULL, DELIMITERS)) != NULL) {
    tn = malloc(sizeof(struct token_node));
    assert(tn);
    tn->token = strdup(token);
    tn->next = NULL;
    curr->next = tn;
    curr = tn;
    /* Handle strings. If the token starts with ", scan until closing " */
    /* FIXME: There's a bug here, where the delimiter within the string
     * gets eaten. It might also not be working right if there was no delim
     * within the string. */
    if (curr->token[0] == '\"') {
      char *s = strtok(NULL, "\"");
      if (s) {
        size_t sz = strlen(curr->token) + strlen(s) + 2; /* +2 for \" and \0 */
        curr->token = realloc(curr->token, sz);
        assert(curr->token);
        strcat(curr->token, s);
        strcat(curr->token, "\""); /* restore the closing " */
      }
    }
  }

  free(linebuf);
  return next;
}

/* Public Interface */

struct line* get_lines(char *infile)
{
  struct line* head, *curr;
  FILE *in = fopen(infile, "r");

#ifdef DEBUG
  assert(in);
#endif

  if (!in) return NULL;
  
  head = get_next_line(in);
  if (!head) return NULL;

  curr = head;
  do {
    curr->next = get_next_line(in);
    curr = curr->next;
  } while (curr != NULL);

  fclose(in);
  return head;
}

void print_lines(struct line* curr)
{
  struct token_node* tok = NULL;

  while (curr != NULL) {

#ifdef VERBOSE
    if (curr->type < NUM_DIRECTIVES) {
      printf("Directive: %s\t", directives[curr->type]);
    } else if (curr->type < NUM_DIRECTIVES + NUM_INSTS) {
      printf("Instruction: %s\t", instructions[curr->type - NUM_DIRECTIVES]);
    } else {
      printf("Unknown Type: %d\t", curr->type);
    }
#endif

    if (curr->label) {
      printf("%s\t", curr->label);
    }

    for (tok = curr->token_listhead; tok != NULL; tok = tok->next) {
      printf("%s\t", tok->token);
    }
    printf("\n");
    curr = curr->next;
  }

}

void free_lines(struct line* lines_head)
{
  struct line *curr = lines_head, *next;

  while (curr != NULL) {
    next = curr->next;
    free_token_list(curr->token_listhead);
    free(curr->label);
    free(curr);
    curr = next;
  }
}

