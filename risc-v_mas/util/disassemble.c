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

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#define DEBUG

#define DATA_BEGIN (0x10000000)
#define TEXT_BEGIN (0x00400000)

static char *get_reg_name(uint8_t reg_idx)
{
  char *regnames[] = {
    "zero",
    "ra",
    "sp",
    "gp",
    "tp",
    "t0",
    "t1",
    "t2",
    "s0",
    "s1",
    "a0",
    "a1",
    "a2",
    "a3",
    "a4",
    "a5",
    "a6",
    "a7",
    "s2",
    "s3",
    "s4",
    "s5",
    "s6",
    "s7",
    "s8",
    "s9",
    "s10",
    "s11",
    "t3",
    "t4",
    "t5",
    "t6"
  };

  assert(reg_idx < 32);
  return regnames[reg_idx];
}


static char *decode_operation(uint8_t opcode, uint8_t funct3, uint8_t funct7)
{
  switch (opcode) {
    case 0x3:
      if (funct3 == 0x2) return "lw";
      break;

    case 0x13: /* immediates */
      switch (funct3) {
        case 0x0:
          return "addi";
        case 0x1:
          if (funct7 == 0) return "slli";
          break;
        case 0x2:
          return "slti";
        case 0x4:
          return "xori";
        case 0x5:
          if (funct7 == 0) return "srli";
          if (funct7 == 0x20) return "srai";
          break;
        case 0x6:
          return "ori";
        case 0x7:
          return "andi";
      }
      break;

    case 0x17:
      return "auipc";
      break;

    case 0x23:
      if (funct3 == 0x2) return "sw";
      break;

    case 0x33:
      switch (funct3) {
        case 0x0:
          if (funct7 == 0) return "add";
          else if (funct7 == 0x20) return "sub";
          break;
        case 0x1:
          if (funct7 == 0) return "sll";
          break;
        case 0x2:
          if (funct7 == 0) return "slt";
          break;
        case 0x4:
          if (funct7 == 0) return "xor";
          break;
        case 0x5:
          if (funct7 == 0) return "srl";
          else if (funct7 == 0x20) return "sra";
          break;
        case 0x6:
          if (funct7 == 0) return "or";
          break;
        case 0x7:
          if (funct7 == 0) return "and";
          break;
      }
      break;

    case 0x37:
      return "lui";
      break;

    case 0x63:
      switch (funct3) {
        case 0x0:
          return "beq";
        case 0x1:
          return "bne";
      }
      break;

    case 0x67:
      if (funct3 == 0) return "jalr";
      break;
      
    case 0x6F:
      return "jal";
      break;

    case 0x73:
      if (funct3 == 0 && funct7 == 0) return "ecall";
      break;

    default:
      break;
  }

  return "unknown";
}

static uint8_t get_rs1(uint32_t iw)
{
  return (iw >> 15) & 0x1f;
}

static uint8_t get_rs2(uint32_t iw)
{
  return (iw >> 20) & 0x1f;
}

static uint8_t get_rd(uint32_t iw)
{
  return (iw >> 7) & 0x1f;
}

static int16_t get_imm(uint32_t iw)
{
  return ((int32_t)iw >> 20);
}

static char *get_lw_operands(uint32_t iw)
{
  char *s = malloc(4+2+5+6+1);
  uint8_t rs1, rd;
  rs1 = get_rs1(iw);
  rd = get_rd(iw);

  snprintf(s, 4+2+5+6+1, "%s, %d(%s)",
      get_reg_name(rd), get_imm(iw), get_reg_name(rs1));
  return s;
}

static char *get_i_fmt_operands(uint32_t iw)
{
  char *s = malloc(4+2+4+2+5+1);
  uint8_t rs1, rd;
  rs1 = get_rs1(iw);
  rd = get_rd(iw);

  snprintf(s, 4+2+4+2+5+1, "%s, %s, %d",
      get_reg_name(rd), get_reg_name(rs1), get_imm(iw));
  return s;
}

static char *get_s_fmt_operands(uint32_t iw)
{
  char *s = malloc(4+2+5+6+1);
  uint8_t rs1, rs2;
  uint16_t imm;
  rs1 = get_rs1(iw);
  rs2 = get_rs2(iw);

  imm = (((int32_t)iw >> 20) & ~(0x1f)) | ((iw >> 7) & 0x1f);

  snprintf(s, 4+2+5+6+1, "%s, %d(%s)",
      get_reg_name(rs2), imm, get_reg_name(rs1));
  return s;
}

static char *get_r_fmt_operands(uint32_t iw)
{
  char *s = malloc(4+2+4+2+4+2+1);
  uint8_t rs1, rs2, rd;
  rs1 = get_rs1(iw);
  rs2 = get_rs2(iw);
  rd = get_rd(iw);

  snprintf(s, 4+2+4+2+4+2+1, "%s, %s, %s",
      get_reg_name(rd), get_reg_name(rs1), get_reg_name(rs2));
  return s;
}

static char *get_u_fmt_operands(uint32_t iw)
{
  char *s = malloc(4+2+8+1);
  uint8_t rd;
  uint32_t long_imm;
  rd = get_rd(iw);

  long_imm = (int32_t)iw >> 12;

  snprintf(s, 4+2+8+1, "%s, %d",
      get_reg_name(rd), long_imm);
  return s;

}

static char *get_sb_fmt_operands(uint32_t iw)
{
  char *s = malloc(4+2+4+2+5+1);
  uint8_t rs1, rs2;
  int16_t imm;
  rs1 = get_rs1(iw);
  rs2 = get_rs2(iw);

  imm = (((int32_t)iw >> 19) & ~(0xfff)) |
        ((iw & (1<<7))<<4) |
        (((iw >> 25) & 0x3f) << 5) |
        ((iw >> 7) & 0x1e);

  snprintf(s, 4+2+4+2+5+1, "%s, %s, %d",
      get_reg_name(rs1), get_reg_name(rs2), imm);
  return s;
}

static char *get_jalr_operands(uint32_t iw)
{
  char *s = malloc(4+2+5+6+1);
  uint8_t rs1, rd;
  rs1 = get_rs1(iw);
  rd = get_rd(iw);

  snprintf(s, 4+2+5+6+1, "%s, %d(%s)",
      get_reg_name(rd), get_imm(iw), get_reg_name(rs1));
  return s;
}

static char *get_jal_operands(uint32_t iw)
{
  char *s = malloc(4+2+8+1);
  uint8_t rd;
  uint32_t long_imm;
  rd = get_rd(iw);

  long_imm = (((int32_t)iw >> 12) & ~(0x7ffff)) |
        (iw & 0xff000) |
        ((iw & 0x100000) >> 9) |
        ((iw & 0x7fe00000) >> 20);

  snprintf(s, 4+2+8+1, "%s, %d",
      get_reg_name(rd), long_imm);
  return s;
}

static char *decode_operands(uint32_t iw)
{
  uint8_t opcode, funct3, funct7;
  opcode = iw&0x7f;
  funct3 = (iw>>12)&0x7;
  funct7 = (iw>>25)&0x7f;
  char *s;

  switch (opcode) {
    case 0x3:
      if (funct3 == 0x2) return get_lw_operands(iw);
      break;

    case 0x13: /* immediates */
      switch (funct3) {
        case 0x1:
        case 0x5:
        if (!(funct7 == 0 || funct7 == 0x20)) break;
        iw = (iw & ~(0xFE000000)); // clear out funct7
        case 0x0:
        case 0x2:
        case 0x4:
        case 0x6:
        case 0x7:
        return get_i_fmt_operands(iw);
      }
      break;

    case 0x17:
      return get_u_fmt_operands(iw);

    case 0x23:
      if (funct3 == 0x2) return get_s_fmt_operands(iw);
      break;

    case 0x33:
      switch (funct3) {
        case 0x0:
        case 0x5:
          if (!(funct7 == 0 || funct7 == 0x20)) break;
          funct7 = 0;
        case 0x1:
        case 0x2:
        case 0x4:
        case 0x6:
        case 0x7:
          if (!(funct7 == 0)) break;
          return get_r_fmt_operands(iw);
      }
      break;

    case 0x37:
      return get_u_fmt_operands(iw);

    case 0x63:
      if (!(funct3 == 0 || funct3 == 1)) break;
      return get_sb_fmt_operands(iw);

    case 0x67:
      if (!(funct3 == 0)) break;
      return get_jalr_operands(iw);

    case 0x6F:
      return get_jal_operands(iw);
      break;

    case 0x73:
      switch (funct3) {
        case 0x0:
          if (!(funct7 == 0)) break;
          return get_i_fmt_operands(iw);
      }
      break;

    default:
      break;
  }

  s = (char*)malloc(1);
  *s = 0;
  return s;
}

/* Decode feature is not fully implemented */
char *decode(uint32_t word) {
  uint8_t opcode, funct3, funct7;
  char *s;
  size_t bytes = 0;
  char *mnemonic;
  char *operands;

  opcode = word&0x7f;
  funct3 = (word>>12)&0x7;
  funct7 = (word>>25)&0x7f;

  mnemonic = decode_operation(opcode, funct3, funct7);
  bytes += strnlen(mnemonic, 8);

  operands = decode_operands(word);
  bytes += strnlen(operands, 30);

  s = malloc(bytes);
  assert(s != NULL);

  snprintf(s, bytes+2, "%s %s", mnemonic, operands);
  free(operands);

  return s;
}


static void read_and_print(char *infile, size_t data_words, size_t text_words)
{
  size_t count;
  FILE *in;
  uint32_t *data = malloc(sizeof(uint32_t)*data_words);
  uint32_t *text = malloc(sizeof(uint32_t)*text_words);

  assert(data != NULL);
  assert(text != NULL);

	in = fopen(infile, "r");
  assert(in);

  count = fread(data, sizeof(uint32_t), data_words, in);
  assert(count == data_words);
  count = fread(text, sizeof(uint32_t), text_words, in);
  assert(count == text_words);

  printf("\n%s:\tfile format cs4200-riscv32\n\n", infile);

  printf(".data\n");
  for (count = 0; count < data_words; count++) {
    char *s = decode(data[count]);
    printf("%.8X:\t%.2x %.2x %.2x %.2x\t%s\n",
        (uint32_t)(DATA_BEGIN + count*4),
        data[count] >> 24 & 0xff,
        data[count] >> 16 & 0xff,
        data[count] >> 8 & 0xff,
        data[count] >> 0 & 0xff,
        s);
    free(s);
  }
  printf("\n");

  printf(".text\n");
  for (count = 0; count < text_words; count++) {
    char *s = decode(text[count]);
    printf("%.8X:\t%.2x %.2x %.2x %.2x\t%s\n",
        (uint32_t)(TEXT_BEGIN + count*4),
        text[count] >> 24 & 0xff,
        text[count] >> 16 & 0xff,
        text[count] >> 8 & 0xff,
        text[count] >> 0 & 0xff,
        s);
    free(s);
  }
  printf("\n");

  fclose(in);
}

void usage(char *name)
{
	printf("Usage: %s [input program]\n\
where:\n\
\t[input program] is a file containing the program in the expected format.\n",
	 	name);
	exit(1);
}

int main( int argc, char *argv[] )
{
	if ( argc < 2 ) usage(argv[0]);
  read_and_print(argv[1], 1024, 1024);
	return 0;
}
