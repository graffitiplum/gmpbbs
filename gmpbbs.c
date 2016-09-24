/* gmpbbs.c: implementation code for the GMPBBS Blum Blum Shub PRNG

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


#include "gmpbbs.h"

#ifdef _WIN32
/* no random device on windows as far as i know,
   we get (strong?) random data from the operating system */
int _hwrandread(unsigned char *rndbuf, size_t nbytes)
#define FUNC_NAME "_hwrandread"
{
  HCRYPTPROV hp;
  DWORD flags = CRYPT_VERIFYCONTEXT | CRYPT_MACHINE_KEYSET;

  if ( (! CryptAcquireContext( &hp, NULL, NULL, PROV_RSA_FULL, flags )) &&
       ( GetLastError() != NTE_BAD_KEYSET ) )
    {
      flags |= CRYPT_NEWKEYSET;
      if ( ! CryptAcquireContext( &hp, NULL, NULL, PROV_RSA_FULL, flags ) )
	{
	  perror(FUNC_NAME ": CryptAcquireContext");
	  return(0);
	}
    }

  if (!CryptGenRandom(hp, nbytes, rndbuf))
    {
      CryptReleaseContext(hp, 0);
      return(0);
    }

  CryptReleaseContext(hp, 0);

  return(nbytes);  
}
#undef FUNC_NAME
#define _urandread(x,y) _hwrandread(x,y)
#else
int _hwrandread(unsigned char *rndbuf, size_t nbytes)
#define FUNC_NAME "_hwrandread"
{
  FILE *devrnd;
  int nread;

  if ( (devrnd = fopen(HWRANDOM, "r")) == NULL)
    {
      perror(FUNC_NAME ": fopen");
      return(0);
    }
  if ( (nread = fread(rndbuf, 1, nbytes, devrnd)) != nbytes)
    {
      perror(FUNC_NAME ": fread");
    }

  fclose(devrnd);
  return(nread);
}
#undef FUNC_NAME
int _urandread(unsigned char *rndbuf, size_t nbytes)
#define FUNC_NAME "_urandread"
{
  FILE *devrnd;
  int nread;

  if ( (devrnd = fopen(URANDOM, "r")) == NULL)
    {
      perror(FUNC_NAME ": fopen");
      return(0);
    }
  if ( (nread = fread(rndbuf, 1, nbytes, devrnd)) != nbytes)
    {
      perror(FUNC_NAME ": fread");
    }

  fclose(devrnd);
  return(nread);
}
#undef FUNC_NAME
#endif

/*
  initialize bbs->blumint randomly
    key_bitlen may be over by n=pq,
    so the actual keylen may be 1 bit more than requested...
    to modify it, you'd have to set high bit and clear second high bit for p,q.
    (and hope they don't bump up after we find their real values
    at any rate, it's not a big deal really.. just confusing :)
*/
int rndbbs_gen_blumint (rndbbs_t *bbs, unsigned int key_bitlen)
#define FUNC_NAME "rndbbs_gen_blumint"
{
  mpz_t p, q;

  if (key_bitlen < GMPBBS_MINKEYLEN)
    return(0);

  /* init pstr */
  {
    int i, pbits = (key_bitlen)/2 + 1;
    int pbytes = ceil(pbits/8.0);
    char *pstr;
    unsigned char *rnd;

    if ( (rnd = (unsigned char *) malloc(pbytes)) == NULL )
      {
	perror(FUNC_NAME ": malloc");
	return(0);
      }

    /* get random seed for p from random device */
    if (_hwrandread(rnd, pbytes) != pbytes)
      {
	free(rnd);
	perror(FUNC_NAME ": _hwrandread");
	return(0);
      }

    if ( pbits % 8 )
      /* a little bit-fiddling to get *exactly* our number of bits */
      /* then set the high bit all in one pass */
      rnd[0] = ( ( rnd[0] & ~( ~0 << (pbits % 8) ) ) |
		 ( 0x80 >> ( 8 - (pbits%8) ) ) );
    else
      /* just set high bit */
      rnd[0] |= 0x80;

    if ( (pstr = (char *) malloc(2*pbytes+1)) == NULL)
      {
	free(rnd);
	perror(FUNC_NAME ": malloc");
	return(0);
      }
    for (i=0;i<pbytes;i++)
      {
	snprintf(pstr+(2*i), 3, "%02x", rnd[i]);
      }
    free(rnd);

    if (mpz_init_set_str(p, pstr, 16) == -1)
      {
	free(pstr);
	perror(FUNC_NAME ": mpz_init_set_str");
	return(0);
      }
    free(pstr);

    /* now, find p such that ( p prime ) && ( p = 3 (mod 4) ) */
    for(;;)
      {
	mpz_nextprime(p,p);

	/* mpz_tstbit(p, 1) (p!=2) is faster than mpz_fdiv_ui(p, 4)==3 */
	if ( mpz_tstbit(p, 1) &&
	     /* probab_prime: mainly to do an advanced check (higher REPS) */
	     mpz_probab_prime_p(p, MPZ_PROBAB_PRIME_REPS) )
	  break;
      }
  }

  /* init qstr */
  {
    int i, qbits = (key_bitlen)/2 + (key_bitlen%2);
    int qbytes = ceil(qbits/8.0);

    char *qstr;
    unsigned char *rnd;

    if ( (rnd = (unsigned char *) malloc(qbytes)) == NULL )
      {
	perror(FUNC_NAME ": malloc");
	return(0);
      }

    /* get random seed for q from random device */
    if (_hwrandread(rnd, qbytes) != qbytes)
      {
	free(rnd);
	perror(FUNC_NAME ": _hwrandread");
	return(0);
      }

    if ( qbits % 8 )
      /* a little bit-fiddling to get *exactly* our number of bits */
      /* then set the high bit all in one pass */
      rnd[0] = ( ( rnd[0] & ~( ~0 << (qbits % 8) ) ) |
		 ( 0x80 >> ( 8 - (qbits%8) ) ) );
    else
      /* just set high bit */
      rnd[0] |= 0x80;

    if ( (qstr = (char *) malloc(2*qbytes+1)) == NULL)
      {
	free(rnd);
	perror(FUNC_NAME ": malloc");
	return(0);
      }
    for (i=0;i<qbytes;i++)
      {
	snprintf(qstr+(2*i), 3, "%02x", rnd[i]);
      }
    free(rnd);

    if (mpz_init_set_str(q, qstr, 16) == -1)
      {
	free(qstr);
	perror(FUNC_NAME ": mpz_init_set_str");
	return(0);
      }
    free(qstr);

    /* now, find q such that ( q prime ) && ( q = 3 (mod 4) ) */
    for(;;)
      {
	mpz_nextprime(q,q);
	/* mpz_tstbit(q, 1) (q!=2) is faster than mpz_fdiv_ui(q, 4)==3 */
	if ( mpz_tstbit(q, 1) &&
	     /* probab_prime: mainly to do an advanced check (higher REPS) */
	     mpz_probab_prime_p(q, MPZ_PROBAB_PRIME_REPS) )
	  break;
      }
  }

  /* a blum integer is p*q ( p and q both = 3 (mod 4) ) */
  mpz_mul(bbs->blumint, p, q);

  /* we need this later to find how many bits ( log2(bbs->key_bitlen) ) */
  bbs->key_bitlen = mpz_sizeinbase(bbs->blumint, 2);

  /* we want p,q if we're going to use this as a stream cipher */
  
  mpz_clear(p);
  mpz_clear(q);
  
  return(1);
}
#undef FUNC_NAME

/* initialize bbs->x randomly */
int rndbbs_gen_x (rndbbs_t *bbs)
#define FUNC_NAME "rndbbs_gen_x"
{
  unsigned char *rnd;
  char *xstr;
  int i, nbytes = (mpz_sizeinbase(bbs->blumint, 2)+7)/8;

  if ( (rnd = (unsigned char *) malloc(nbytes)) == NULL )
    {
      perror(FUNC_NAME ": malloc");
      return(0);
    }

  /* get random seed for x from random device */
  if (_hwrandread(rnd, nbytes) != nbytes)
    {
      free(rnd);
      perror(FUNC_NAME ": _hwrandread");
      return(0);
    }

  if ( (xstr = (char *) malloc(2*nbytes+1)) == NULL)
    {
      free(rnd);
      perror(FUNC_NAME ": malloc");
      return(0);
    }
  for (i=0;i<nbytes;i++)
    {
      snprintf(xstr+2*i, 3, "%02x", rnd[i]);
    }
  free(rnd);

  if (mpz_set_str(bbs->x, xstr, 16) == -1)
    {
      free(xstr);
      perror(FUNC_NAME ": mpz_set_str");
      return(0);
    }
  free(xstr);

  /* now, find x such that gcd(blumint,x) = 1. */
  {
    mpz_t tmpgcd;

    mpz_init(tmpgcd);
    for(;;)
      {
	mpz_gcd(tmpgcd, bbs->blumint, bbs->x);
	if (mpz_cmp_ui(tmpgcd, 1) == 0)
	  break;
	mpz_add_ui(bbs->x, bbs->x, 1);
      }
    mpz_clear(tmpgcd);
  }

  /* x[0] = x^2 (mod blumint) */
  mpz_powm_ui(bbs->x, bbs->x, 2, bbs->blumint);

  return(1);
}
#undef FUNC_NAME

rndbbs_t *rndbbs_new()
#define FUNC_NAME "rndbbs_new"
{
  rndbbs_t *bbs = NULL;

  if ( (bbs = (rndbbs_t *) malloc( sizeof(rndbbs_t) ) ) == NULL )
    {
      perror(FUNC_NAME ": malloc");
      return(NULL);
    }

  mpz_init(bbs->blumint);
  mpz_init(bbs->x);
  bbs->key_bitlen = 0;
  bbs->improved = 1;
  bbs->xor_urandom = 0;

  return(bbs);
}
#undef FUNC_NAME

int rndbbs_destroy(rndbbs_t *bbs)
#define FUNC_NAME "rndbbs_destroy"
{
  /* if using this as a cipher, we need to append xn+1 to the end. */
  mpz_clear(bbs->blumint);
  mpz_clear(bbs->x);
  free(bbs);

  return(1);
}
#undef FUNC_NAME

char *rndbbs_randbytes(rndbbs_t *bbs, size_t nbytes)
#define FUNC_NAME "rndbbs_randbytes"
{
  char *retbuf;
  char *urandom_buffer = NULL;

  if ( (retbuf = (char *) malloc(nbytes)) == NULL)
    {
      perror(FUNC_NAME ": malloc");
      return(NULL);
    }

  if ( bbs->xor_urandom )
    {
      if ( (urandom_buffer = (char *) malloc(nbytes)) == NULL)
	{
	  perror(FUNC_NAME ": malloc");
	  return(NULL);
	}

      if ( _urandread((unsigned char *) urandom_buffer, nbytes) != nbytes )
	{
	  perror(FUNC_NAME ": _urandread: continuting...");
	  bbs->xor_urandom = 0;
	}
    }

  memset(retbuf, 0, nbytes);

  if (!bbs->improved)
    {
      /* basic implementation without improvements (only keep parity) */
      {
	int i;
	for (i=0;i<nbytes;i++)
	  {
	    int j;

	    /* we keep the parity (least significant bit) of each x_n */
	    for (j=7;j>=0;j--)
	      {
		/* x[n+1] = x[n]^2 (mod blumint) */
		mpz_powm_ui(bbs->x, bbs->x, 2, bbs->blumint);

		/* mpz_fdiv_ui(bbs->x, 2) == mpz_tstbit(bbs->x, 0) */
		retbuf[i] |= (mpz_tstbit(bbs->x, 0) << j);
	      }
	    if (bbs->xor_urandom)
	      retbuf[i] ^= urandom_buffer[i];
	  }
	if ( urandom_buffer != NULL )
	  free(urandom_buffer);
	return(retbuf);
      }
    }
  else
    {
      /* improved implementation (keep log2(log2(blumint)) bits of x[i]) */
      unsigned int loglogblum = log(1.0*bbs->key_bitlen)/log(2.0);

      unsigned int byte=0, bit=0, i;

      for (;;)
	{
	  /* x[n+1] = x[n]^2 (mod blumint) */
	  mpz_powm_ui(bbs->x, bbs->x, 2, bbs->blumint);

	  for (i=0;i<loglogblum;i++)
	    {
	      if (byte == nbytes)
		{
		  if ( urandom_buffer != NULL )
		    free(urandom_buffer);
		  return(retbuf);
		}

	      /* get the ith bit of x */
	      retbuf[byte] |= (mpz_tstbit(bbs->x, i) << (7-bit) );

	      if (bit == 7)
		{
		  if (bbs->xor_urandom)
		    retbuf[byte] ^= urandom_buffer[byte];
		  byte++;
		  bit=0;
		}
	      else
		{
		  bit++;
		}
	    }
	}
    }
}
#undef FUNC_NAME

unsigned int *rndbbs_randint(rndbbs_t *bbs, unsigned int base, size_t nmemb)
#define FUNC_NAME "rndbbs_randint"
{
  int i;
  unsigned char *rndbuf;
  char *mpzstr;
  mpz_t bn;
  unsigned int *rndint = NULL;

  /* we waste a few precious bits here unless we're a power of 256 */
  unsigned int nbytes = ceil((nmemb)*log(base)/log(256));

  rndbuf = (unsigned char *) rndbbs_randbytes(bbs, nbytes);

  if ( (mpzstr = (char *) malloc(2*nbytes+1)) == NULL)
    {
      free(rndbuf);
      perror(FUNC_NAME ": malloc");
      return(NULL);
    }
  for (i=0;i<nbytes;i++)
    {
      snprintf(mpzstr+2*i, 3, "%02x", rndbuf[i]);
    }
  free(rndbuf);

  if (mpz_init_set_str(bn, mpzstr, 16) == -1)
    {
      free(mpzstr);
      perror(FUNC_NAME ": mpz_init_set_str");
      return(NULL);
    }
  free(mpzstr);

  if ((rndint = (unsigned int *) malloc(nmemb * sizeof(unsigned int))) == NULL)
    {
      mpz_clear(bn);
      perror(FUNC_NAME ": malloc");
      return(NULL);
    }

  for (i=0;i<nmemb+1;i++)
    {
      mpz_t tmpmpz;
      mpz_t q;

      mpz_init(q);
      mpz_init_set_ui(tmpmpz, base);
      mpz_pow_ui(tmpmpz, tmpmpz, (nmemb-i) );
      mpz_tdiv_q(q, bn, tmpmpz);
      if (i>0)
	rndint[i-1] = mpz_get_ui(q);

      mpz_mul(tmpmpz, q, tmpmpz);
      mpz_sub(bn, bn, tmpmpz);

      mpz_clear(q);
      mpz_clear(tmpmpz);
    }
  mpz_clear(bn);

  return(rndint);
}
#undef FUNC_NAME
