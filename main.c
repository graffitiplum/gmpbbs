/* main.c: sample code for the GMPBBS Blum Blum Shub PRNG.

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


#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

#include "gmpbbs.h"

void usage (const char *me)
{
  fprintf(stderr,
	  "usage: %s [-hX] [-o outfile] [-b base] [-k key_bitlen]\n"
	  "      \t[-p prime] [-q prime] [-x initial] <# of randoms>\n\n"
	  "   -h, --help    :\tthis help message\n"
	  "   -o, --output  :\tfile to write output to\n"
	  "   -B, --binary  :\toutput as bytes\n"
	  "   -H, --hex     :\toutput as hexidecimal digits\n"
	  "   -b, --base    :\tnumber base to use for output\n"
	  "   -k, --keylen  :\trequested key length (k>=%d) (default 1024)\n"
	  "   -s, --slow    :\tdon't use the improved (fast) algorithm\n"
	  "   -X, --xor     :\tXOR BBS output with output from /dev/urandom\n"
	  "   -p            :\tprime p = 3 (mod 4)\n"
	  "   -q            :\tprime q = 3 (mod 4)\n"
	  "   -x            :\tinitial x (to generate x0) (gcd(pq,x)=1)\n"
	  "   # of randoms  :\tthe number of random integers to generate\n"
	  , me, GMPBBS_MINKEYLEN);
}

int main(int argc, char **argv)
{
  char *out_fn = NULL;
  FILE *outf = stdout;
  rndbbs_t *bbs;
  int keylen = 1024;
  int base = 256;
  int representation = 256;
  unsigned int nbytes;
  char *pstr=NULL, *qstr=NULL, *xstr=NULL;

  int opt, option_index=0;

  bbs = rndbbs_new();

  static struct option long_options[] =
    {
      { "output", 1, NULL, 'o' },
      { "binary", 0, NULL, 'B' },
      { "hex", 0, NULL, 'H' },
      { "base64", 0, NULL, 'M' },
      { "base", 1, NULL, 'b' },
      { "keylen", 1, NULL, 'k' },
      { "slow", 0, NULL, 's' },
      { "xor", 0, NULL, 'X' },
      { "help", 0, NULL, 'h' },
    };

  while ((opt =
	  getopt_long(argc, argv, "BHMsXho:k:b:p:q:x:",
		      long_options, &option_index)) != -1)
    {
      switch(opt)
	{
	case 'h':
	  usage(argv[0]);
	  rndbbs_destroy(bbs);
	  return(0);
	  break;
	case 'o':
	  out_fn = optarg;
	  break;
	case 'B':
	  representation=256;
	  break;
	case 'H':
	  representation=16;
	  break;
	case 'M':
	  representation=64;
	  break;
	case 'b':
	  base = atoi(optarg);
	  if (base < 2)
	    {
	      usage(argv[0]);
	      rndbbs_destroy(bbs);
	      return(1);
	    }
	  representation = 10;
	  break;
	case 'k':
	  keylen = atoi(optarg);
	  if (keylen <= GMPBBS_MINKEYLEN)
	    {
	      usage(argv[0]);
	      rndbbs_destroy(bbs);
	      return(1);
	    }
	  break;
	case 's':
	  bbs->improved = 0;
	  break;
	case 'p':
	  pstr = optarg;
	  break;
	case 'q':
	  qstr = optarg;
	  break;
	case 'x':
	  xstr = optarg;
	  break;
	case 'X':
	  bbs->xor_urandom = 1;
	  break;
	default:
	  usage(argv[0]);
	  rndbbs_destroy(bbs);
	  return(1);
	}
    }

  if (argc <= optind)
    {
      rndbbs_destroy(bbs);
      usage(argv[0]);
      return(1);
    }
  nbytes = atoi(argv[optind]);
  if (nbytes < 1)
    {
      rndbbs_destroy(bbs);
      usage(argv[0]);
      return(1);
    }

  if ( (pstr != NULL) && (qstr != NULL) )
    {
      mpz_t p, q;

      mpz_init_set_str(p, pstr, 0);
      mpz_init_set_str(q, qstr, 0);

      if ( (mpz_probab_prime_p(p, MPZ_PROBAB_PRIME_REPS) && mpz_tstbit(p,1)) &&
	   (mpz_probab_prime_p(q, MPZ_PROBAB_PRIME_REPS) && mpz_tstbit(q,1)) )
	{
	  mpz_mul(bbs->blumint, p, q);
	  bbs->key_bitlen = mpz_sizeinbase(bbs->blumint, 2);
	}
      else
	{
	  usage(argv[0]);
	  rndbbs_destroy(bbs);
	  return(1);
	}

      if (xstr != NULL)
	{
	  mpz_t tmpgcd;

	  mpz_init_set_str(bbs->x, xstr, 0);

	  mpz_init(tmpgcd);
	  mpz_gcd(tmpgcd, bbs->blumint, bbs->x);
	  if (mpz_cmp_ui(tmpgcd, 1) != 0)
	    {
	      mpz_clear(tmpgcd);
	      usage(argv[0]);
	      rndbbs_destroy(bbs);
	      return(1);
	    }
	  mpz_clear(tmpgcd);

	  /* x[0] = x^2 (mod n) */
	  mpz_powm_ui(bbs->x, bbs->x, 2, bbs->blumint);
	}
      else
	{
	  rndbbs_gen_x(bbs);
	}
    }
  else if ( (pstr != NULL) || (qstr != NULL) )
    {
      usage(argv[0]);
      rndbbs_destroy(bbs);
      return(1);
    }
  else
    {
      rndbbs_gen_blumint(bbs, keylen);
      rndbbs_gen_x(bbs);
    }

  if (out_fn != NULL)
    {
#ifdef _WIN32
      /* set binary mode for windows if base if 256 (default) */
      char *openflg;
      if (base == 256)
	openflg = "wb";
      else
	openflg = "w";

      if ( (outf = fopen(out_fn, openflg)) == NULL)
#endif /* _WIN32 */
      if ( (outf = fopen(out_fn, "w")) == NULL)
	{
	  perror("fopen");
	  rndbbs_destroy(bbs);
	  return(1);
	}
    }

  switch(representation)
    {
      /* TODO: base64 */
    case 256:
      {
#ifndef WRITE_BLOCK_SIZE
#define WRITE_BLOCK_SIZE 131072
#endif
	unsigned int incr_writed = 0;

	while ( incr_writed < nbytes )
	  {
	    unsigned char *rnd;
	    size_t nb = WRITE_BLOCK_SIZE;
	    size_t nwritten;

	    if ( (incr_writed + WRITE_BLOCK_SIZE) > nbytes )
	      nb = (nbytes - incr_writed);

	    rnd = (unsigned char *)
	      rndbbs_randbytes(bbs, nb);

	    if (rnd == NULL)
	      {
		perror("failed to generate bytes");
		rndbbs_destroy(bbs);
		return(1);
	      }

	    nwritten = fwrite(rnd, 1, nb, outf);
	    incr_writed += nwritten;
	    if ( nwritten != nb )
	      perror("write short of block size");

	    free(rnd);
	  }
	return(0);
#undef WRITE_BLOCK_SIZE
      }
      break;
    case 16:
      {
	unsigned char *rnd;
	int i;
	char *rndstr;

	rnd = (unsigned char *) rndbbs_randbytes(bbs, nbytes);
	if (rnd == NULL)
	  {
	    perror("failed to generate bytes");
	    rndbbs_destroy(bbs);
	    return(1);
	  }
	if ( (rndstr = (char *) malloc(2*nbytes + 1)) == NULL )
	  {
	    perror("failed to generate bytes");
	    free(rnd);
	    rndbbs_destroy(bbs);
	    return(1);
	  }
	for (i=0;i<nbytes;i++)
	  {
	    snprintf(rndstr+(2*i), 3, "%02x", rnd[i]);
	  }
	free(rnd);

	fwrite(rndstr, 1, 2*nbytes, outf);

	/* should this be like binary and skip \n? */
	fprintf(outf, "\n");
	free(rndstr);

	return(0);
      }
      break;
    default:
      {
	int i;
	unsigned int *rndint;

	rndint = rndbbs_randint(bbs, base, nbytes);
	for (i=0;i<nbytes;i++)
	  {
	    fprintf(outf, "%d", rndint[i]);
	    if (i != (nbytes-1))
	      fprintf(outf, " ");
	    else
	      fprintf(outf, "\n");
	  }
	free(rndint);
	rndbbs_destroy(bbs);
	return(0);
      }
      break;
    }

  return(0);
}
