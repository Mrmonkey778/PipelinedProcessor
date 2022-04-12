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

#include "symtab.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TBLSZ (255)
struct symbol {
  char *label;
  uint32_t address;
};
static struct symbol symtab[TBLSZ] = {0};


/* djb2 hash function (public domain) */
static uint32_t hash(char *str) {
  uint32_t hash = 5381;
  int c;
  while (c = *str++) hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
  return hash;
}

void symtab_add(char *lbl, uint32_t addr)
{
  uint32_t hv = hash(lbl) % TBLSZ;

  while (symtab[hv].label) {
    if (strncmp(symtab[hv].label, lbl, strlen(lbl)) == 0) {
      return; /* matching symbol found */
    }
    hv = (hv + 1) % TBLSZ;
  }
  symtab[hv].label = lbl;
  symtab[hv].address = addr;
}

uint32_t symtab_find_address(char *lbl)
{
  uint32_t hv = hash(lbl) % TBLSZ;
  while (symtab[hv].label) {
    if (strncmp(symtab[hv].label, lbl, strlen(lbl)) == 0) {
      return symtab[hv].address; /* matching symbol found */
    }
    hv = (hv + 1) % TBLSZ;
  }
  return 0;
}

void symtab_print()
{
  int i;
  for (i = 0; i < TBLSZ; i++) {
    if (symtab[i].label) {
      printf("%d\t%s\t%x\n", i, symtab[i].label, symtab[i].address);
    }
  }
}

