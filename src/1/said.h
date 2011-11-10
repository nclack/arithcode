#include <stdint.h>
#include <stdlib.h>

typedef uint8_t   u8;
typedef uint32_t  u32;
typedef float     real;

typedef struct _bits_t
{ size_t ibyte;  //current byte
  size_t ibit;   //current bit   - FIXME: don't actually need this, just use mask instead
  size_t nbytes; //capacity
  u8     mask;   //
  u8 *d;
} bits_t;

/*
 * decode
 * ------
 *  <out>  output buffer, should be pre-allocated
 *  <nout> must be less than or equal to the actual message size
 *  <in>   the bit string returned in encode's <out.d>
 *  <c>    the CDF over output symbols
 *  <nsym> the number of output symbols
 */

void cdf_build(real **cdf, size_t *nsym, u32 *s, size_t ns);
void encode(bits_t *out, u32 *s, size_t N,real *cdf, size_t nsym);
void decode(u32 *out, size_t nout, u8 *in, size_t nin, real *c, size_t nsym);

