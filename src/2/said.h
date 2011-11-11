#include <stdint.h>
#include <stdlib.h>

typedef uint8_t   u8;
typedef uint32_t  u32;
typedef float     real;

/*
 * encode
 * ------
 *  <out>   <*out> may be NULL or point to a preallocated buffer with <*nout> 
 *  <nsym>  elements.  The buffer may be reallocated if it's not big   
 *          enough, in which case the new size and adress are output.
 *                                                                      
 * decode                                                               
 * ------                                                               
 *  <out>   output buffer, should be pre-allocated                     
 *  <nout>  must be less than or equal to the actual message size      
 *  <in>    the bit string returned in encode's <out.d>                
 *  <c>     the CDF over output symbols                                
 *  <nsym>  the number of output symbols                               
 */

void cdf_build(real **cdf, size_t *nsym, u32 *s, size_t ns);
void encode(u8 **out, size_t *nout, u32 *in, size_t nin, real *cdf, size_t nsym);
void decode(u32 *out, size_t nout, u8 *in, size_t nin, real *c, size_t nsym);

