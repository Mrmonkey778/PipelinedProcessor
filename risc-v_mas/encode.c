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

#include "encode.h"
#include "parser.h"
#include "symtab.h"
#include "writer.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//#define DEBUG 1

void encode_data(struct line *data_start, uint8_t *data)
{
  struct line *curr = data_start;
  uint32_t offset = 0x10000000;
  uint32_t addr = 0;
  struct token_node *tok;
  int n;
  char *c;
  int i;

  assert(curr->type == DATA);

  while (curr != NULL) {
    if (curr->label) {
      curr->label[strlen(curr->label)-1] = 0;
      symtab_add(curr->label, addr+offset);
    }

    switch (curr->type) {
      case ALIGN:
        n = atoi(curr->token_listhead->next->token);
        uint32_t next_addr = ((addr + (1<<n)-1) & ~((1<<n)-1));
        while (addr != next_addr) data[addr++] = 0;
        break;

      case ASCIIZ:
        c = curr->token_listhead->next->token;
        assert(*c++ == '"');
        while (*c != '"') {
          data[addr+3] = *c++;
          if (*c == '"') {
            data[addr+2] = data[addr+1] = data[addr] = 0;
            addr += 4;
            break;
          }
          data[addr+2] = *c++;
          if (*c == '"') {
            data[addr+1] = data[addr] = 0;
            addr += 4;
            break;
          }
          data[addr+1] = *c++;
          if (*c == '"') {
            data[addr] = 0;
            addr += 4;
            break;
          }
          data[addr] = *c++;
          addr += 4;
          if (*c == '"') {
            data[addr++] = 0;
            break;
          }
        }
        while (addr%4 != 0) data[addr++] = 0; /* pad with 0s */
        break;

      case DATA:
        break;

      case SPACE:
        n = atoi(curr->token_listhead->next->token);
        for (i = 0; i < n; i++) {
          data[addr++] = 0;
        }
        break;

      case TEXT:
        curr = NULL;
        goto out;

      case WORD:
        tok = curr->token_listhead->next;
        while (tok != NULL) {
          uint32_t w = (uint32_t)atoi(tok->token);
          /* swap bytes, store in big endian, will be written in little */
          data[addr++] = (w >> 24) & 0xff;
          data[addr++] = (w >> 16) & 0xff;
          data[addr++] = (w >> 8) & 0xff;
          data[addr++] = w & 0xff;
          tok = tok->next;
        }
        break;

      default:
        fprintf(stderr, "Unexpected directive: type = %d\n", curr->type);
        goto out;
    }
#if defined(DEBUG)
    printf("Directive: %d\n", curr->type);
    if (curr->label) {
      printf("%s\t", curr->label);
    }
    for (tok = curr->token_listhead; tok != NULL; tok = tok->next) {
      printf("%s\t", tok->token);
    }
    printf("\n");
#endif
    curr = curr->next;
  }

out:
  if (addr >= 4096) {
    fprintf(stderr, "Data segment overrun, size = %d\n", addr);
  }

  /* zero-initialize the remainder */
  while (addr < 4*DATA_SEGMENT_WORDS) {
    data[addr++] = 0;
  }
}

void encode_text_first_pass(struct line *text_start, uint8_t *text)
{
  struct line *curr = text_start;
  uint32_t offset = 0x00400000;
  uint32_t addr = 0;

  assert(curr->type == TEXT);

  while (curr != NULL) {
    if (curr->label != NULL) {
      curr->label[strlen(curr->label)-1] = 0;
      symtab_add(curr->label, addr+offset);
    }

    if (curr->type == TEXT) {
      curr = curr->next;
      continue;
    }

    if (curr->type >= NUM_DIRECTIVES) {
      addr += 4;
    } else {
      break; /* found something that is not an instruction */
    }

#if defined(DEBUG)
    printf("Type: %d\n", curr->type);
    if (curr->label) {
      printf("%s\t", curr->label);
    }
    for (tok = curr->token_listhead; tok != NULL; tok = tok->next) {
      printf("%s\t", tok->token);
    }
    printf("\n");
#endif

    curr = curr->next;
  }
}

static uint8_t get_opcode(struct line *insn)
{
  linetype t = insn->type;
  uint8_t op = 0xff;

  switch (t) {
    case LW:
      op = 0x03;
      break;

    case ADDI:
    case SLLI:
    case SLTI:
    case XORI:
    case SRLI:
    case SRAI:
    case ORI:
    case ANDI:
      op = 0x13;
      break;

    case AUIPC:
      op = 0x17;
      break;

    case SW:
      op = 0x23;
      break;

    case ADD:
    case SUB:
    case SLL:
    case XOR:
    case SRL:
    case SRA:
    case OR:
    case AND:
      op = 0x33;
      break;

    case LUI:
      op = 0x37;
      break;

    case BEQ:
    case BNE:
      op = 0x63;
      break;

    case JALR:
      op = 0x67;
      break;

    case JAL:
      op = 0x6F;
      break;

    case ECALL:
      op = 0x73;
      break;

    default:
      fprintf(stderr, "get_opcode: Unknown instruction type: %d\n", t);
      break;
  }

  return op;
}

static uint8_t get_funct3(struct line *insn)
{
  linetype t = insn->type;
  uint8_t f3 = 0xff;

  switch (t) {
    case ADD:
    case ADDI:
    case SUB:
    case BEQ:
    case JALR:
    case ECALL:
      f3 = 0x0;
      break;

    case SLL:
    case SLLI:
    case BNE:
      f3 = 0x1;
      break;

    case LW:
    case SW:
    case SLT:
    case SLTI:
      f3 = 0x2;
      break;

    case XOR:
    case XORI:
      f3 = 0x4;
      break;

    case SRL:
    case SRLI:
    case SRA:
    case SRAI:
      f3 = 0x5;
      break;

    case OR:
    case ORI:
      f3 = 0x6;
      break;

    case AND:
    case ANDI:
      f3 = 0x7;
      break;

    default:
      fprintf(stderr, "get_funct3: Unknown instruction type: %d\n", t);
      break;
  }

  return f3;
}

static uint8_t get_funct7(struct line *insn)
{
  linetype t = insn->type;
  uint8_t f7 = 0xff;

  switch (t) {
    case ADD:
    case AND:
    case OR:
    case SLL:
    case SLLI:
    case SLT:
    case SRL:
    case XOR:
    case ECALL:
      f7 = 0x0;
      break;

    case SRA:
    case SRAI:
    case SUB:
      f7 = 0x20;
      break;

    default:
      fprintf(stderr, "get_funct7: Unknown instruction type: %d\n", t);
      break;
  }

  return f7;
}

uint8_t get_reg(char *name)
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
  int i;

  if (name[0] == 'x') return (uint8_t)atoi(&name[1]);
  if (strncmp(name, "fp", 2) == 0) return 8;

  for (i = 0; i < 31; i++) {
    if (strncmp(name, regnames[i], 4) == 0) return i;
  }

  fprintf(stderr, "get_reg: unknown name: %s\n", name);
  return 0;
}

static uint32_t encode_r_fmt(struct line *insn, uint32_t pc)
{
  struct token_node *tok;
  uint32_t iw = 0;
  uint8_t rs1, rs2, rd, funct7, funct3, opcode;

  tok = insn->token_listhead->next;
  rd = get_reg(tok->token);
  tok = tok->next;
  rs1 = get_reg(tok->token);
  tok = tok->next;
  rs2 = get_reg(tok->token);
  tok = tok->next;
  assert(tok == NULL);

  funct7 = get_funct7(insn);
  funct3 = get_funct3(insn);
  opcode = get_opcode(insn);

  iw = (funct7<<25) | (rs2<<20) | (rs1 << 15) | (funct3 << 12) | (rd << 7) | opcode;

  return iw;
}

static int32_t get_imm(char *s)
{
  int32_t rv = 0;
  int i;
  if (s && s[0] == '0') {
    if (s[1] == 'x') {
      sscanf(s, "0x%x", &rv);
      return rv;
    } else if (s[1] == 'b') {
      int len = strlen(s);
      for (i = 2; i < len; i++) {
        uint32_t b = s[i]-'0';
        rv += b<<(len-i-1);
      }
      return rv;
    }
  }
  return (int32_t)atoi(s);
}

static uint32_t encode_i_fmt(struct line *insn, uint32_t pc)
{
  struct token_node *tok;
  uint32_t iw = 0;
  uint8_t rs1, rd, funct3, opcode;
  int16_t imm = 0;

  tok = insn->token_listhead->next;
  rd = get_reg(tok->token);
  tok = tok->next;

  if (tok->token[strlen(tok->token)-1] == ')') {
    char *d = strdup(tok->token);
    char *off = d;
    char *base = d;
    while (*++base != '(');
    *base++ = 0;
    base[strlen(base)-1] = 0;
    rs1 = get_reg(base);
    imm = get_imm(off);
    free(d);
  } else {
    rs1 = get_reg(tok->token);
    tok = tok->next;
    imm = get_imm(tok->token);
  }
  tok = tok->next;
  assert(tok == NULL);


  if (insn->type == SLLI || insn->type == SRLI || insn->type == SRAI) {
    imm |= get_funct7(insn)<<5;
  }

  funct3 = get_funct3(insn);
  opcode = get_opcode(insn);

  iw = (imm<<20) | (rs1 << 15) | (funct3 << 12) | (rd << 7) | opcode;
  return iw;
}

static uint32_t encode_env(struct line *insn, uint32_t pc)
{
  uint32_t iw = 0;
  uint8_t rs1, rd, funct3, opcode;
  uint16_t imm;
  assert(insn->type == ECALL);
  rs1 = rd = 0;
  funct3 = get_funct3(insn);
  opcode = get_opcode(insn);
  imm = get_funct7(insn)<<5;
  iw = (imm<<20) | (rs1 << 15) | (funct3 << 12) | (rd << 7) | opcode;
  return iw;
}

static uint32_t encode_sb_fmt(struct line *insn, uint32_t pc)
{
  struct token_node *tok;
  uint32_t iw = 0;
  uint8_t rs1, rs2, funct3, opcode;
  int16_t imm = 0;
  uint32_t branch_target = 0;

  tok = insn->token_listhead->next;
  rs1 = get_reg(tok->token);
  tok = tok->next;
  rs2 = get_reg(tok->token);
  tok = tok->next;
  branch_target = symtab_find_address(tok->token);
  if (!branch_target) {
    fprintf(stderr, "Unable to find branch target: %s\n", tok->token);
    return 0;
  }
  tok = tok->next;
  assert(tok == NULL);

  funct3 = get_funct3(insn);
  opcode = get_opcode(insn);

  imm = (int32_t)branch_target - (int32_t)pc;

  iw = ((((imm&(1<<12))>>6)|((imm&0x7e0)>>5))<<25)
        | (rs2 << 20)
        | (rs1 << 15)
        | (funct3 << 12)
        | (((imm&0x1e)|((imm&(1<<11))>>11))<<7)
        | opcode;

  return iw;
}

static uint32_t encode_u_fmt(struct line *insn, uint32_t pc)
{
  struct token_node *tok;
  uint32_t iw = 0;
  uint8_t rd, opcode;
  int32_t imm = 0;

  tok = insn->token_listhead->next;
  rd = get_reg(tok->token);
  tok = tok->next;
  imm = get_imm(tok->token);
  tok = tok->next;
  assert(tok == NULL);

  opcode = get_opcode(insn);
 
  iw = (imm&~(0xfffU)) | (rd<<7) | opcode;

  return iw;
}

static uint32_t encode_uj_fmt(struct line *insn, uint32_t pc)
{
  struct token_node *tok;
  uint32_t iw = 0;
  uint8_t rd, opcode;
  int32_t imm = 0;
  uint32_t jump_target;

  tok = insn->token_listhead->next;
  rd = get_reg(tok->token);
  tok = tok->next;
  jump_target = symtab_find_address(tok->token);
  if (!jump_target) {
    fprintf(stderr, "Unable to find jump target: %s\n", tok->token);
    return 0;
  }
  tok = tok->next;
  assert(tok == NULL);

  opcode = get_opcode(insn);

  imm = (int32_t)jump_target - (int32_t)pc;

  iw = ((imm&(1<<20))|((imm&0x7fe)<<8)|((imm&(1<<11))>>4)|((imm&0xff000)>>12))<<12
        | (rd<<7) | opcode;

  return iw;

  return 0;
}

static uint32_t encode_s_fmt(struct line *insn, uint32_t pc)
{
  struct token_node *tok;
  uint32_t iw = 0;
  uint8_t rs1, rs2, funct3, opcode;
  int16_t imm = 0;

  tok = insn->token_listhead->next;
  rs2 = get_reg(tok->token);
  tok = tok->next;

  if (tok->token[strlen(tok->token)-1] == ')') {
    char *off = tok->token;
    char *base = tok->token;
    while (*++base != '(');
    *base++ = 0;
    base[strlen(base)-1] = 0;
    rs1 = get_reg(base);
    imm = get_imm(off);
  } else {
    fprintf(stderr, "Unrecognized memory operand: %s\n", tok->token);
    return 0;
  }
  tok = tok->next;
  assert(tok == NULL);

  funct3 = get_funct3(insn);
  opcode = get_opcode(insn);

  iw = ((imm&~(0x1f))<<20) | (rs2 << 20) | (rs1 << 15) | (funct3 << 12) |
       ((imm&0x1f)<<7) | opcode;

  return iw;
}


static uint32_t encode_insn(struct line *insn, uint32_t pc)
{

  switch (insn->type) {
    case ADD:
    case AND:
    case OR:
    case SLL:
    case SLT:
    case SRA:
    case SRL:
    case SUB:
    case XOR:
      return encode_r_fmt(insn, pc);
      break;

    case ADDI:
    case ANDI:
    case JALR:
    case LW:
    case ORI:
    case SLLI:
    case SLTI:
    case SRAI:
    case SRLI:
    case XORI:
      return encode_i_fmt(insn, pc);
      break;

    case ECALL:
      return encode_env(insn, pc);
      break;

    case AUIPC:
    case LUI:
      return encode_u_fmt(insn, pc);
      break;

    case BEQ:
    case BNE:
      return encode_sb_fmt(insn, pc);
      break;

    case JAL:
      return encode_uj_fmt(insn, pc);
      break;

    case SW:
      return encode_s_fmt(insn, pc);
      break;

    default:
      fprintf(stderr, "Unknown instruction type: %d\n", insn->type);
      break;
  }
  return 0;
}

static uint32_t encode_pseudo_insn(struct line *insn, uint32_t offset, uint32_t addr, char *text)
{
  uint32_t iw = 0;
  struct token_node *tok;
  struct token_node tok1, tok2, tok3, tok4;
  uint32_t address;
  uint32_t imm_long;
  uint16_t imm_short;
  uint8_t rd, rs1, rs2;
  uint8_t opcode, funct3, funct7;
  struct line real_insn = {};

  tok = insn->token_listhead->next;

  switch (insn->type) {
    case J:
      real_insn.type = JAL;
      tok1.token = "jal";
      tok2.token = "x0";
      tok3.token = tok->token;
      tok1.next = &tok2;
      tok2.next = &tok3;
      tok3.next = NULL;
      real_insn.token_listhead = &tok1;
      iw = encode_uj_fmt(&real_insn, offset+addr);
      *((uint32_t*)text) = iw;
      return 4;

    case LA:
      /* This one is a bit sketchy. Not fully tested yet. */
      rd = get_reg(tok->token);
      tok = tok->next;
      address = symtab_find_address(tok->token);
      if (!address) {
        fprintf(stderr, "Unable to find address: %s\n", tok->token);
        return 0;
      }
      tok = tok->next;
      assert(tok == NULL);
      imm_long = (int32_t)address - (int32_t)(addr+offset);

      imm_short = imm_long & 0xfff;
      imm_long >>= 12;
      if (imm_short & 1<<11) {
        imm_long++;
      }

      real_insn.type = AUIPC;
      opcode = get_opcode(&real_insn);
      iw = (imm_long<<12) | (rd<<7) | opcode;
      *((uint32_t*)text) = iw;
      text += 4;

      real_insn.type = ADDI;
      rs1 = rd;
      opcode = get_opcode(&real_insn);
      funct3 = get_funct3(&real_insn);
      iw = (imm_short<<20) | (rs1 << 15) | (funct3 << 12) | (rd << 7) | opcode;
      *((uint32_t*)text) = iw;

      return 8;

    case LI:
      rd = get_reg(tok->token);
      tok = tok->next;
      imm_long = get_imm(tok->token);
      tok = tok->next;
      assert(tok == NULL);

      imm_short = imm_long & 0xfff;
      imm_long >>= 12;
      if (imm_short & 1<<11) {
        imm_long++;
      }

      real_insn.type = LUI;
      opcode = get_opcode(&real_insn);
      iw = (imm_long<<12) | (rd<<7) | opcode;
      *((uint32_t*)text) = iw;
      text += 4;

      real_insn.type = ADDI;
      rs1 = rd;
      opcode = get_opcode(&real_insn);
      funct3 = get_funct3(&real_insn);
      iw = (imm_short<<20) | (rs1 << 15) | (funct3 << 12) | (rd << 7) | opcode;
      *((uint32_t*)text) = iw;

      return 8;

    case MV:
      real_insn.type = ADDI;
      tok1.token = "addi";
      tok2.token = tok->token;
      tok = tok->next;
      tok3.token = tok->token;
      tok4.token = "0";
      tok1.next = &tok2;
      tok2.next = &tok3;
      tok3.next = &tok4;
      tok4.next = NULL;
      real_insn.token_listhead = &tok1;
      iw = encode_i_fmt(&real_insn, offset+addr);
      *((uint32_t*)text) = iw;
      return 4;

    case NEG:
      real_insn.type = SUB;
      tok1.token = "sub";
      tok2.token = tok->token;
      tok = tok->next;
      tok3.token = "x0";
      tok4.token = tok->token;
      tok1.next = &tok2;
      tok2.next = &tok3;
      tok3.next = &tok4;
      tok4.next = NULL;
      real_insn.token_listhead = &tok1;
      iw = encode_r_fmt(&real_insn, offset+addr);
      *((uint32_t*)text) = iw;
      return 4;

    case NOP:
      real_insn.type = ADDI;
      tok1.token = "addi";
      tok2.token = "x0";
      tok3.token = "x0";
      tok4.token = "0";
      tok1.next = &tok2;
      tok2.next = &tok3;
      tok3.next = &tok4;
      tok4.next = NULL;
      real_insn.token_listhead = &tok1;
      iw = encode_i_fmt(&real_insn, offset+addr);
      *((uint32_t*)text) = iw;
      return 4;

    case NOT:
      real_insn.type = XORI;
      tok1.token = "xori";
      tok2.token = tok->token;
      tok = tok->next;
      tok3.token = tok->token;
      tok4.token = "-1";
      tok1.next = &tok2;
      tok2.next = &tok3;
      tok3.next = &tok4;
      tok4.next = NULL;
      real_insn.token_listhead = &tok1;
      iw = encode_i_fmt(&real_insn, offset+addr);
      *((uint32_t*)text) = iw;
      return 4;

    case RET:
      real_insn.type = JALR;
      tok1.token = "jalr";
      tok2.token = "x0";
      tok3.token = "0(ra)";
      tok1.next = &tok2;
      tok2.next = &tok3;
      tok3.next = NULL;
      real_insn.token_listhead = &tok1;
      iw = encode_i_fmt(&real_insn, offset+addr);
      *((uint32_t*)text) = iw;
      return 4;

    default:
      fprintf(stderr, "Unrecognized pseudo instruction type: %d\n", insn->type);
      break;
  }

  return 0;
}


void encode_text_second_pass(struct line *text_start, uint8_t *text)
{
  struct line *curr;
  uint32_t offset = 0x00400000;
  uint32_t addr = 0;
  uint32_t iw;

  assert(text_start->type == TEXT);

  for (curr = text_start; curr != NULL; curr = curr->next) {
    if (curr->type == TEXT) {
      continue;
    }
    
    if (curr->type < NUM_DIRECTIVES) {
      break; /* found something that is not an instruction */
    }

    if (curr->type < FIRST_PSEUDOINST) {
      iw = encode_insn(curr, offset+addr);
      *((uint32_t*)text) = iw;
      text += 4;
      addr += 4;
    } else {
      /* handle pseudoinstructions specially. */
      uint8_t bytes = encode_pseudo_insn(curr, offset, addr, text);
      text += bytes;
      addr += bytes;
    }
  }
}

void encode(struct line *llh, uint8_t *data, uint8_t *text)
{
  struct line *curr = llh;
  struct line *text_start = NULL;

  while (curr != NULL) {
    if (curr->type == DATA) {
      encode_data(curr, data);
    }
    if (curr->type == TEXT) {
      text_start = curr;
      encode_text_first_pass(text_start, text);
    }
    curr = curr->next;
  }
  symtab_print();
  encode_text_second_pass(text_start, text);
}

