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

#ifndef PARSER_H_
#define PARSER_H_

#include <stdint.h>

typedef enum {
  ALIGN = 0,
  ASCIIZ = 1,
  DATA = 2,
  SPACE = 3,
  TEXT = 4,
  WORD = 5,
  ADD = 6,
  ADDI = 7,
  AND = 8,
  ANDI = 9,
  AUIPC = 10,
  BEQ = 11,
  BNE = 12,
  JAL = 13,
  JALR = 14,
  LUI = 15,
  LW = 16,
  OR = 17,
  ORI = 18,
  SLT = 19,
  SLTI = 20,
  SLL = 21,
  SLLI = 22,
  SRA = 23,
  SRAI = 24,
  SRL = 25,
  SRLI = 26,
  SUB = 27,
  SW = 28,
  XOR = 29,
  XORI = 30,
  ECALL = 31,
  J = 32,
  LA,
  LI,
  MV,
  NEG,
  NOP,
  NOT,
  RET
} linetype;

#define NUM_DIRECTIVES (6)
extern char *directives[NUM_DIRECTIVES];

#define NUM_INSTS (34)
extern char *instructions[NUM_INSTS];

#define FIRST_PSEUDOINST (J)

struct token_node {
  char *token;
  struct token_node *next;
};

struct line {
  linetype type;  /* What kind of line this is */
  char *label;    /* Assembler label, if any */
  struct token_node* token_listhead;  /* Tokenized line */
  struct line* next;
};

/**
 * Reads in all lines from the file named @infile.
 *
 * Returns an array of allocated and populated struct line objects.
 *
 * Returns NULL if an error occurred.
 */
struct line* get_lines(char *infile);

/**
 * Prints the lines to stdout, for debugging.
 */
void print_lines(struct line* lines_head);

/**
 * Frees all the memory allocated by the list of lines starting at head.
 */
void free_lines(struct line* lines_head);

#endif /* PARSER_H_ */

