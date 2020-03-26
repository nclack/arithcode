/**   
   \file
   Arithmetic coding implementation

   \section Introduction 
  
   Implementation after Amir Said's Algorithm 22-29[1].
  
   Following Amir's notation, 
  
    D is the number of symbols in the output alphabet.
    P is the number of output symbols in the active block (see Eq 2.3 in [1]).
      Need 2P for multiplication.
  
   So, have 2P*bitsof(D) = 64 (widest integer type I'll have access to)
  
   The smallest codable probabililty is p = D/D^P = D^(1-P).  Want to minimize
   p wrt constraint. Enumerating the possibilities:
  
   \verbatim
     bitsof(D)  P                D^(1-P)
     --------   --               -------
     1          32                2^-32
     8          4    (2^8)^(-3) = 2^-24
     16         2                 2^-16
     32         1                 1
   \endverbatim
   
   One can see there's a efficiency verses precision trade off here (higher D
   means less renormalizing and so better efficiency).
  
   \section Notes
   - Need to test more!
     - u1,u4,u16 encoding/decoding not tested
     - random sequences etc...
     - empty stream?
     - streams that generte big carries
  
   - might be nice to add an interface that will encode chunks.  That is,
     you feed it symbols one at a time and every once in a while, when
     bits get settled, it outputs some bytes.  
     For streaming encoders, this would mean the intermediate buffer 
     wouldn't have to be quite as big, although worst case that
     buffer is the size of the entire output message.
  
   - A check-symbol could be encoded in a manner similar to the END-OF-MESSAGE
     symbol.  For example, code the symbol every 2^x symbols and assign it a
     probability of 2^-x.  If the decoder doesn't recieve one of these at the
     expected interval, then there was some error.  For short messages, 
     it wouldn't require the insertion of such a symbol; only failure to show
     up after N symbols would indicate an error.  It would take space
     on the interval, and so would have a negative impact on compression and 
     minimum probability.
     
   \section References
   \verbatim
   [1]:  Said, A. "Introduction to Arithmetic Coding - Theory and Practice."
         Hewlett Packard Laboratories Report: 2004-2076.
         www.hpl.hp.com/techreports/2004/HPL-2004-76.pdf
   \endverbatim
 */

/** \mainpage Arithmetic Coding

    \section Contents
    
    - \ref Example
    - \ref Features
    - \ref History
    - \ref CDFs
    - \ref Encoding
    - \ref Decoding

    \section Example
    \code
    void encode()
    { unsigned char *input, *output=0;      // input and output buffer
      size_t countof_input, countof_output; // number of symbols in input and output buffer
      float *cdf=0;
      size_t nsym;                          // number of symbols in the input alphabet
      // somehow load the data into input array
      cdf_build(&cdf,&nsym,input,countof_input);
      encode_u1_u8(                         // encode unsigned chars to a string of bits (1 bit per output symbol)
        (void**)&out,&countof_output,
               input, countof_input,
                 cdf, nsym);
      // do something with the output
      free(out);
      free(cdf);
    }
    \endcode      

    \section Features

      - Encoding/decoding to/from variable symbol alphabets.

      - Implicit use of a STOP symbol means that you don't need to know the number of symbols in the decoded message in order
        to decode something.

      - Can encoded messages stored as signed or unsigned chars, shorts, longs or long longs.  Keep in mind, however, that the
        number of encodable symbols may be limiting.  You can't encode 2^64 different integers, sorry.

      - Can encode to streams of variable symbol width; either 1,4,8, or 16 bits.  There is are two tradeoffs here.

          - Smaller bit-width (e.g. 1) give better compression than larger bit-width (e.g. 16), but compression is slower (I think?).

          - The implimentation puts a limit on the smallest probability of an encoded symbol.  Smaller bit-width (e.g. 1) can accomidate
            a larger range of probabilities than large bit-width (e.g. 16).

    \todo Markov chain adaptive encoding/decoding.

    \section History History and Caveats

    I wrote this in order to learn about arithmetic coding.  The end goal was to get to the point where I had an adaptive
    encoder/decoder that could compress markov chains.  I got a little distracted along the way by trying to encode to
    a variable symbol alphabet.  Variable symbol encoding alphabets are fun because you can encode to non-whitespace ASCII 
    characters (94 symbols) and that means the encoded message can be embeded in a text file format.

    Arithmetic coding is a computationally expensive coding method.  I've done nothing to address this problem; I've probably
    made it worse.

    To learn more about arithmetic coding see this excelent reference:
    \verbatim
    Said, A. “Introduction to Arithmetic Coding - Theory and Practice.” 
         Hewlett Packard Laboratories Report: 2004–2076.
         www.hpl.hp.com/techreports/2004/HPL-2004-76.pdf
   \endverbatim

    \section CDFs Cumulative Distribution Functions (CDFs)

    All the encoding and decoding functions require knowledge of the probability of observing input symbols.  This probability is
    specified as a CDF of a particular form.
    Namely:

      1. \c cdf[0] must be zero (0.0).

      2. The probability density for symbol \a i should be \c cdf[i+1]-cdf[i].

    This implies, for an alphabet with \a M symbols:

      - \a cdf has \a M+1 elements.

      - \c cdf[M] is 1.0 (within floating point precision)

    \see cdf_build() for an example of how to build a CDF from a given input message.

    \section Encoding Encoding Functions

    Encoding functions all have the same form:
    \code
    encode_<TDst>_<TSrc>(void **out, size_t *nout, uint8_t *in, size_t nin, float *cdf, size_t nsym);
    \endcode
    where \a TDst and \a TSrc are the output and input types, respectively.  The output buffer, \a *out, 
    can be an heap-allocated buffer; it's capacity in bytes should be passed as \a *nout.  If \a *out is 
    not large enough (or NULL), it will be reallocated.  The new capacity will be returned in \a *nout.

    - encode_u1_u8()
    - encode_u4_u8()
    - encode_u8_u8()
    - encode_u1_u16()
    - encode_u4_u16()
    - encode_u8_u16()
    - encode_u1_u32()
    - encode_u4_u32()
    - encode_u8_u32()
    - encode_u1_u64()
    - encode_u4_u64()
    - encode_u8_u64()

    Variable symbol encodings are a little different.  These functions take the number of output symbols, \a noutsym,
    as well.  They always encode to a 1-byte stream.  Their form is:
    \code
    vencode_<TSrc>(void **out, size_t *nout, size_t noutsym, uint8_t *in, size_t nin, size_t ninsym, float *cdf);
    \endcode

    - vencode_u8()
    - vencode_u16()
    - vencode_u32()
    - vencode_u64()

    \section Decoding Decoding functions

    Decoding functions all have the same form:
    \code
    decode_<TDst>_<TSrc>(void **out, size_t *nout, uint8_t *in, size_t nin, float *cdf, size_t nsym);
    \endcode
    where \a TDst and \a TSrc are the output and input types, respectively.  The output buffer, \a *out, 
    can be an heap-allocated buffer; it's capacity in bytes should be passed as \a *nout.  If \a *out is 
    not large enough (or NULL), it will be reallocated.  The new capacity will be returned in \a *nout.

    This is basically identical to the encode functions, but here "output" refers to the decoded message
    and "input" refers to an encoded message.

    - decode_u8_u1()
    - decode_u8_u4()
    - decode_u8_u8()
    - decode_u16_u1()
    - decode_u16_u4()
    - decode_u16_u8()
    - decode_u32_u1()
    - decode_u32_u4()
    - decode_u32_u8()
    - decode_u64_u1()
    - decode_u64_u4()
    - decode_u64_u8()

    Variable symbol decoding has the form:
    \code
    vdecode_<TDst>(void **out, size_t *nout, size_t noutsym, uint8_t *in, size_t nin, size_t ninsym, float *cdf);
    \endcode

    - vdecode_u8()
    - vdecode_u16()
    - vdecode_u32()
    - vdecode_u64() 

    \author Nathan Clack <https://github.com/nclack>
*/

#include <stdio.h>
#include <string.h>
#include "stream.h"

typedef uint8_t   u8;
typedef uint8_t   u16;
typedef uint32_t  u32;
typedef uint64_t  u64;
typedef float     real;

#define ENDL "\n"
#define TRY(e) \
  do{ if(!(e)) {\
    printf("%s(%d):"ENDL "\t%s"ENDL "\tExpression evaluated as false."ENDL, \
        __FILE__,__LINE__,#e); \
    goto Error; \
  }} while(0)

#define SAFE_FREE(e) if(e) { free(e); (e)=NULL; }

/** 
    \defgroup ArithmeticCoding Arithmetic Coding Internals    
    @{
 */

/**
 Encoder/Decoder state

 Used internally. Not externally visible.
 */

typedef struct _state_t
{ u64      b,       ///< Beginning of the current interval.
           l,       ///< Length of the current interval.
           v;       ///< Current value.
  stream_t d;       ///< The attached data stream.
  size_t   nsym;    ///< The number of symbols in the input alphabet.
  u64      D,       ///< The number of symbols in the output alphabet.
           shift,   ///< A utility constant.  log2(D^P) - need 2P to fit in a register for multiplies.
           mask,    ///< Masks the live bits (can't remember exactly?)
           lowl;    ///< The minimum length of an encodable interval.
  u64     *cdf;     ///< The cdf associated with the input alphabet.  Must be an array of N+1 symbols.
} state_t;

/// A helper function that initializes the parts of the \ref state_t structure that do not depend on stream type.
static void init_common(state_t *state,u8 *buf,size_t nbuf,real *cdf,size_t nsym)
{ 
  
  state->l = (1ULL<<state->shift)-1; // e.g. 2^32-1 for u64
  state->mask = state->l;            // for modding a u64 to u32 with &

  nsym++; // add end symbol
  TRY( state->cdf=malloc(nsym*sizeof(*state->cdf)) );
  { size_t i;
    real s = state->l-state->D; // scale to D^P range and adjust for end symbol
    for(i=0;i<(nsym-1);++i)
      state->cdf[i] = s*cdf[i];
    state->cdf[i] = s;
    state->nsym = nsym;
#if 0
    for(i=0;i<nsym;++i)
      printf("CDF[%5zu] = %12llu  cdf[%5zu] = %f"ENDL,i,state->cdf[i],i,cdf[i]);
#endif
  }
  attach(&state->d,buf,nbuf);
  return;
Error:
  abort();
}
/// Initialize the state_t structure for \c u1 streams.
static void init_u1(state_t *state,u8 *buf,size_t nbuf,real *cdf,size_t nsym)
{ memset(state,0,sizeof(*state));
  state->D     = 2;
  state->shift = 32; // log2(D^P) - need 2P to fit in a register for multiplies
  state->lowl  = 1ULL<<31; // 2^(shift - log2(D))
  init_common(state,buf,nbuf,cdf,nsym);
}
/// Initialize the state_t structure for \c u4 streams.
static void init_u4(state_t *state,u8 *buf,size_t nbuf,real *cdf,size_t nsym)
{ memset(state,0,sizeof(*state));
  state->D     = 1ULL<<4;
  state->shift = 32; // log2(D^P) - need 2P to fit in a register for multiplies
  state->lowl  = 1ULL<<28; // 2^(shift - log2(D))
  init_common(state,buf,nbuf,cdf,nsym);
}
/// Initialize the state_t structure for \c u8 streams.
static void init_u8(state_t *state,u8 *buf,size_t nbuf,real *cdf,size_t nsym)
{ memset(state,0,sizeof(*state));
  state->D     = 1ULL<<8;
  state->shift = 32; // log2(D^P) - need 2P to fit in a register for multiplies
  state->lowl  = 1ULL<<24; // 2^(shift - log2(D))
  init_common(state,buf,nbuf,cdf,nsym);
}
/// Initialize the state_t structure for \c u16 streams.
static void init_u16(state_t *state,u8 *buf,size_t nbuf,real *cdf,size_t nsym)
{ memset(state,0,sizeof(*state));
  state->D     = 1ULL<<16;
  state->shift = 32;       // log2(D^P) - need 2P to fit in a register for multiplies
  state->lowl  = 1ULL<<16; // 2^(shift - log2(D))
  init_common(state,buf,nbuf*2,cdf,nsym);
}
/// Releases resources held by the state_t structure.
static  void free_internal(state_t *state)
{ void *d;
  SAFE_FREE(state->cdf);
//detach(&state->d,&d,NULL); // Don't really want to do this - ends up wierd
//SAFE_FREE(d);
}

//
// Build CDF
// 

/// Array maximum.  \returns the maximum value in a u32 array, \a s, with \a n elements.
static u32 maximum(u32 *s, size_t n)
{ u32 max=0;
  while(n--)
    max = (s[n]>max)?s[n]:max;
  return max;
}

/** 
Build a cumulative distribution function (CDF) from an input u32 array.

As provided, this really just serves as a reference implementation showing how
the CDF should be computed.
Namely:

  1. \c cdf[0] must be zero (0.0).

  2. The probability density for symbol \a i should be \c cdf[i+1]-cdf[i].

This implies:

  - \a cdf has \a M+1 elements.

  - \c cdf[(*M)+1]==1.0 (within floating point precision)

\param[in,out] cdf The cumulative distribution function computed over \a s.
                   If *cdf is not null, will realloc() if necessary.
\param[out]    M   The number of symbols in the array \a s.
\param[in]     s   The input message formated as a \c u32 array of \a N elements.
\param[in]     N   The number of elements in the array \a s.

*/
void cdf_build(real **cdf, size_t *M, u32 *s, size_t N)
{ size_t i,nbytes;
  *M = maximum(s,N)+1; 
  TRY( *cdf=realloc(*cdf,nbytes=sizeof(real)*(M[0]+1)) ); // cdf has M+1 elements
  memset(*cdf,0,nbytes);
  for(i=0;i<     N;++i) cdf[0][s[i]]++;                   // histogram
  for(i=0;i<M[0]  ;++i) cdf[0][i]/=(real)N;               // norm
  for(i=1;i<M[0]  ;++i) cdf[0][i]   += cdf[0][i-1];       // cumsum
  for(i=M[0]+1;i>0;--i) cdf[0][i]    = cdf[0][i-1];       // move
  cdf[0][0] = 0.0;
    
#ifdef DEBUG
  TRY( fabs(cdf[M[0]+1]-1.0) < 1e-6 );
#endif
  return;
Error:
  abort();
}

//
// Encoder
//

#define B         (state->b)
#define L         (state->l)
#define C         (state->cdf)
#define SHIFT     (state->shift)
#define NSYM      (state->nsym)
#define MASK      (state->mask)
#define STREAM    (&(state->d))
#define DATA      (state->d.d)
#define OFLOW     (STREAM->overflow)
#define bitsofD   (8)
#define D         (1ULL<<bitsofD)
#define LOWL      (state->lowl)

void carry_null(stream_t *s) {} //no op
void push_null(stream_t *s)  {s->ibyte++;}

#define DEFN_UPDATE(T) \
  static void update_##T(u64 s,state_t *state) \
  { u64 a,x,y;                                \
    y = L; /* End of interval */              \
    if(s!=(NSYM-1)) /*is not last symbol */   \
      y = (y*C[s+1])>>SHIFT;                  \
    a = B;                                    \
    x = (L*C[s])>>SHIFT;                      \
    B = (B+x)&MASK;                           \
    L = y-x;                                  \
    TRY(L>0);                                 \
    if(a>B)                                   \
      carry_##T(STREAM);                      \
    return;                                   \
  Error:                                      \
    abort();                                  \
  }
DEFN_UPDATE(u1);
DEFN_UPDATE(u4);
DEFN_UPDATE(u8);
DEFN_UPDATE(u16);
DEFN_UPDATE(null);

#define DEFN_ERENORM(T) \
static void erenorm_##T(state_t *state) \
{                                      \
  const int s = SHIFT-bitsofD;         \
  while(L<LOWL)                        \
  { push_##T(STREAM, B>>s);             \
    L = (L<<bitsofD)&MASK;             \
    B = (B<<bitsofD)&MASK;             \
  }                                    \
}
DEFN_ERENORM(u1); // typed by output stream type
DEFN_ERENORM(u4);
DEFN_ERENORM(u8);
DEFN_ERENORM(u16);
static void erenorm_null(state_t *state)
{
  const int s = SHIFT-bitsofD;
  while(L<LOWL)
  { push_null(STREAM);
    L = (L<<bitsofD)&MASK;
    B = (B<<bitsofD)&MASK;
  }
}

#define DEFN_ESELECT(T) \
  static void eselect_##T(state_t *state)                                                                \
  { u64 a;                                                                                              \
    a=B;                                                                                                \
    B=(B+(1ULL<<(SHIFT-bitsofD-1)) )&MASK; /* D^(P-1)/2: (2^8)^(4-1)/2 = 2^24/2 = 2^23 = 2^(32-8-1) */  \
    L=(1ULL<<(SHIFT-2*bitsofD))-1;         /* requires P>2 */                                           \
    if(a>B)                                                                                             \
      carry_##T(STREAM);                                                                                  \
    erenorm_##T(state);                     /* output last 2 symbols */                                  \
  }
DEFN_ESELECT(u1); // typed by output stream type
DEFN_ESELECT(u4);
DEFN_ESELECT(u8);
DEFN_ESELECT(u16);

#define DEFN_ESTEP(T) \
  static void estep_##T(state_t *state,u64 s) \
  {                                          \
    update_##T(s,state);                      \
    if(L<LOWL)                               \
      erenorm_##T(state);                     \
  }
DEFN_ESTEP(u1); // typed by output stream type
DEFN_ESTEP(u4);
DEFN_ESTEP(u8);
DEFN_ESTEP(u16);
DEFN_ESTEP(null); // doesn't actually write to stream

#define DEFN_ENCODE(TOUT,TIN) \
void encode_##TOUT##_##TIN(void **out, size_t *nout, TIN *in, size_t nin, real *cdf, size_t nsym) \
{ size_t i;                             \
  state_t s;                            \
  init_##TOUT(&s,*out,*nout,cdf,nsym);  \
  for(i=0;i<nin;++i)                    \
    estep_##TOUT(&s,in[i]);             \
  estep_##TOUT(&s,s.nsym-1);            \
  eselect_##TOUT(&s);                   \
  detach(&s.d,out,nout);                \
  free_internal(&s);                    \
}
#define DEFN_ENCODE_OUTS(TIN) \
  DEFN_ENCODE(u1,TIN); \
  DEFN_ENCODE(u4,TIN); \
  DEFN_ENCODE(u8,TIN); \
  DEFN_ENCODE(u16,TIN);
DEFN_ENCODE_OUTS(u8);
DEFN_ENCODE_OUTS(u16);
DEFN_ENCODE_OUTS(u32);
DEFN_ENCODE_OUTS(u64);

//
// Decode
//

static u64 dselect(state_t *state, u64 *v, int *isend)
{ u64 s,n;
  u64 x,y;

  s = 0;
  n = NSYM;
  x = 0;
  y = L;
  while( (n-s)>1UL )   // bisection search
  { u32 m = (s+n)>>1;
    u64 z = (L*C[m])>>SHIFT;
    if(z>*v)
      n=m,y=z;
    else
      s=m,x=z;
  }
  *v -= x;
  L = y-x;
  if(s==(NSYM-1))
    *isend=1;
  return s;
}

#define DEFN_DRENORM(T) \
static void drenorm_##T(state_t *state, u64 *v)\
{ while(L<LOWL)                               \
  { *v = ((*v<<bitsofD)&MASK)+pop_##T(STREAM); \
    L =   ( L<<bitsofD)&MASK;                 \
  }                                           \
}
DEFN_DRENORM(u1);
DEFN_DRENORM(u4);
DEFN_DRENORM(u8);
DEFN_DRENORM(u16);

#define DEFN_DPRIME(T) \
static void dprime_##T(state_t *state, u64* v)                             \
{ size_t i;                                                               \
  *v = 0;                                                                 \
  for(i=bitsofD;i<=SHIFT;i+=bitsofD)                                      \
    *v += (1ULL<<(SHIFT-i))*pop_##T(STREAM); /*(2^8)^(P-n) = 2^(8*(P-n))*/ \
}
DEFN_DPRIME(u1);
DEFN_DPRIME(u4);
DEFN_DPRIME(u8);
DEFN_DPRIME(u16);

#define DEFN_DSTEP(T) \
static u64 dstep_##T(state_t *state,u64 *v,int *isend) \
{                                 \
  u64 s = dselect(state,v,isend); \
  if( L<LOWL )                    \
    drenorm_##T(state,v);         \
  return s;                       \
}
DEFN_DSTEP(u1);
DEFN_DSTEP(u4);
DEFN_DSTEP(u8);
DEFN_DSTEP(u16);

#define DEFN_DECODE(TOUT,TIN) \
void decode_##TOUT##_##TIN(TOUT **out, size_t *nout, u8 *in, size_t nin, real *cdf, size_t nsym) \
{ state_t s;                                   \
  stream_t d={0};                              \
  u64 v,x;                                     \
  size_t i=0;                                  \
  int isend=0;                                 \
  attach(&d,*out,*nout*sizeof(TOUT));          \
  init_##TIN(&s,in,nin,cdf,nsym);              \
  dprime_##TIN(&s,&v);                         \
  x=dstep_##TIN(&s,&v,&isend);                 \
  while(!isend)                                \
  { push_##TOUT(&d,x);                         \
    x=dstep_##TIN(&s,&v,&isend);               \
  }                                            \
  free_internal(&s);                           \
  detach(&d,(void**)out,nout);                 \
  *nout /= sizeof(TOUT);                       \
}
#define DEFN_DECODE_OUTS(TIN) \
  DEFN_DECODE(u8,TIN);  \
  DEFN_DECODE(u16,TIN); \
  DEFN_DECODE(u32,TIN); \
  DEFN_DECODE(u64,TIN);
DEFN_DECODE_OUTS(u1);
DEFN_DECODE_OUTS(u4);
DEFN_DECODE_OUTS(u8);
DEFN_DECODE_OUTS(u16);

//
// Variable output alphabet encoding
//

void sync(stream_t *dest, stream_t *src)
{ dest->d      = src->d;
  dest->nbytes = src->nbytes;
}
void vdecode1(u8 **out, size_t *nout, u8 *in, size_t nin, real *cdf, size_t nsym, real *tcdf, size_t tsym)
{ state_t d0,e1;                                   
  stream_t d={0};                              
  int isend=0;
  u64 v0;
  attach(&d,*out,*nout);          
  init_u8(&d0,in,nin,tcdf,tsym);            // the tcdf decode
  init_u8(&e1, 0,  0,tcdf,tsym);            // the tcdf encode - used to check for end symbol

  dprime_u8(&d0,&v0);
  while(e1.d.ibyte<nin)                     // stop decoding when reencoding reproduces the input string
  { u8 s = dstep_u8(&d0,&v0,&isend);
    push_u8(&d,s);
    estep_null(&e1,s);
  }

  detach(&d,(void**)out,nout);
  { void *b;
    detach(&e1.d,&b,NULL);
    if(b) free(b);
  }
  free_internal(&d0);
  free_internal(&e1);
}

#define DEFN_VENCODE(T) \
void vencode_##T(u8 **out, size_t *nout, size_t noutsym, T *in, size_t nin, size_t ninsym, real *cdf) \
{ u8 *t=NULL;                                          \
  size_t i,n=0;                                        \
  real *tcdf;                                          \
  TRY( tcdf=malloc(sizeof(*tcdf)*(noutsym+1)) );       \
  { size_t i;                                          \
    real v = 1.0/(real)noutsym;                        \
    for(i=0;i<=noutsym;++i)                            \
      tcdf[i] = i*v;                                   \
  }                                                    \
  { void *buf=0;                                       \
    n = 0;                                             \
    encode_u8_##T(&buf,&n,in,nin,cdf,ninsym);          \
    vdecode1(out,nout,buf,n,cdf,ninsym,tcdf,noutsym);  \
    free(buf);                                         \
  }                                                    \
  free(tcdf);                                          \
  return;                                              \
Error:                                                 \
  abort();                                             \
}
DEFN_VENCODE(u8);
DEFN_VENCODE(u16);
DEFN_VENCODE(u32);
DEFN_VENCODE(u64);

#define DEFN_VDECODE(T) \
void vdecode_##T(T **out, size_t *nout, size_t noutsym, u8 *in, size_t nin, size_t ninsym, real *cdf) \
{ u8 *t=NULL;                                          \
  size_t i,n=0;                                        \
  real *tcdf;                                          \
  TRY( tcdf=malloc(sizeof(*tcdf)*(ninsym+1)) );        \
  { size_t i;                                          \
    real v = 1.0/(real)ninsym;                         \
    for(i=0;i<=ninsym;++i)                             \
      tcdf[i] = i*v;                                   \
  }                                                    \
  { void *buf=0;                                       \
    n = 0;                                             \
    encode_u8_u8(&buf,&n,in,nin,tcdf,ninsym);          \
    decode_##T##_u8(out,nout,buf,n,cdf,noutsym);       \
    free(buf);                                         \
  }                                                    \
  free(tcdf);                                          \
  return;                                              \
Error:                                                 \
  abort();                                             \
}
DEFN_VDECODE(u8);
DEFN_VDECODE(u16);
DEFN_VDECODE(u32);
DEFN_VDECODE(u64);

/** @} */ //end addtogroup ac
