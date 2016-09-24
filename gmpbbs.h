/* gmpbbs.h: header file for the GMPBBS Blum Blum Shub PRNG

  Copyright 2015 Maria Morisot.

  This file is released under the GPL.

  GMPBBS is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License,
  or (at your option) any later version.

  GMPBBS is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
  for more details.

  You should have received a copy of the GNU General Public License
  along with GMPBBS; see the file LICENSE.  If not, write to
  the Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
  MA 02111-1307, USA.
*/


#ifndef _GMPBBS_H

/* minimum key length (in bits) */
#define GMPBBS_MINKEYLEN 16

#define _GMPBBS_H 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h> /* needed for rndbbs_randint() */
#include <gmp.h>

#ifdef _WIN32
#include <windows.h>
#include <wincrypt.h>

#else

#ifndef HWRANDOM
#ifdef OPENBSD
/* openbsd reserves /dev/random for special hardware RNG's on crypto chips */
/* /dev/srandom returns reliable random data though */
#define HWRANDOM "/dev/srandom"
#else
#define HWRANDOM "/dev/random"
#endif
#endif /* HWRANDOM */

#define URANDOM "/dev/urandom"

#endif /* _WIN32 */

/* from the manual: 5-10 should be sufficient, higher increases probability */
#ifndef MPZ_PROBAB_PRIME_REPS
#define MPZ_PROBAB_PRIME_REPS 13
#endif

typedef struct
{
  size_t key_bitlen;
  mpz_t blumint;
  mpz_t x;
  int improved;
  int xor_urandom;
} rndbbs_t;

int rndbbs_gen_blumint(rndbbs_t *bbs, unsigned int key_bitlen);
int rndbbs_gen_x(rndbbs_t *bbs);

rndbbs_t *rndbbs_new();
int rndbbs_destroy(rndbbs_t *bbs);

char *rndbbs_randbytes(rndbbs_t *bbs, size_t nbytes);
unsigned int *rndbbs_randint(rndbbs_t *bbs,
			     unsigned int base, size_t nmemb);

#endif /* _GMPBBS_H */
