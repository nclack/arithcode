/*
 * Introduction
 * ------------
 *
 * Implementation after Amir Said's Algorithm 22-29[1].
 *
 * The idea here is to get a more precise idea of what it takes to implement an
 * arithmetic coder, and to get a reference implementation.
 *
 * My first attempt used floating point arithmetic, but here I'll try to use 
 * integer arithmetic only.  I'll also use a wider bit-depth for the output
 * symbols.
 *
 * Following Amir's notation, 
 *
 *  D is the number of symbols in the output alphabet.
 *  P is the number of output symbols in the active block (see Eq 2.3 in [1]).
 *    Need 2P for multiplication.
 *
 * So, have 2P*bitsof(D) = 64 (widest integer type I'll have access to)
 *
 * The smallest codable probabililty is p = D/D^P = D^(1-P).  Want to minimize
 * p wrt constraint. Enumerating the possibilities:
 *
 *   bitsof(D)  P                D^(1-P)
 *   --------   --               -------
 *   1          32                2^-32
 *   8          4    (2^8)^(-3) = 2^-24
 *   16         2                 2^-16
 *   32         1                 1
 *
 * One can see there's a efficiency verses precision trade off here (higher D
 * means less renormalizing and so better efficiency).
 *
 * I'll choose bitsof(D)=8 and P=4.
 *
 * Notes
 * -----
 * - Should support encoding input and decoding output of different types.
 *   (Not all u32)
 *
 * - Need variably sized streams for other sizes of D.  Right now only u8
 *   supported.  vencode is limited to D<256.
 *
 * - should try automatically using an end-of-stream symbol.
 *   Give it the minimum assignable probability.
 *   Could do this without modifying CDF by changing
 *   encoder/update and decoder/dselect functions appropriately.
 *
 *   - nsym'   <- nsym+1
 *   - pdf[s]' <- pdf[s]*nsym/(nsym+1)        ; s<nsym
 *   =>
 *   - cdf[s]' <- cumsum(pdf,s)*nsym/(nsym+1)
 *   - cdf[s]' <- cdf[s-1]+1 (post scaling)   ; s=nsym
 *
 *   o Using binary search lookup on decode, so just tack the END
 *     symbol onto the end.
 *
 *     - add a special estep_end
 *     - dstep must return a flag indicating the end, which is used to 
 *       terminate the decode loop.
 *
 *   o Could use a "virtual" symbol that doesn't take up any symbol space bit
 *     on the unencoded stream (that is, for a u8 input all 255 symbols could be
 *     used; the 'end' symbol wouldn't require you go to a 16-bit input).  This
 *     would require using a wide symbol register on the decode step.
 *
 *   o COST of the end symbol?
 *     entropy was -sum(p log2 p) and is lower bound of expected bits/symbol
 *     now -sum(p*c log2 p) - sum( p*c log2 c ) - D^(1-P) log2 (D^(1-P))
 *         where c = n/(n+1)
 *         if D = 2^d, and c~1,
 *           -sum(p log2 p) - (d-d*P) * 2^(d-d*P)
 *         for d=8,P=4: extra entropy is
 *           24*2^-24 = 3*2^-16 bits?  -- doesn't seem right
 *     upper bound is entropy + overhead/N, where N is the number of symbols to
 *         code.
 *
 *    -- my guess is it adds log2 D^(1-P) bits.  That's at least true for an
 *       empty input stream;  The first decoded value has to be that end D^(1-P)
 *       interval.
 *          
 *
 * - might be nice to add an interface that will encode chunks.  That is,
 *   you feed it symbols one at a time and every once in a while, when
 *   bits get settled, it outputs some bytes.
 *
 *   For streaming encoders, this would mean the intermediate buffer 
 *   wouldn't have to be quite as big, although worst case that
 *   buffer is the size of the entire output message.
 *   
 *   Example use:
 *
 *     foreach(s in input)
 *       for(i=0;i<encode_one(state,s,buf);++i)
 *         do something with buf[i]; 
 *
 * References
 * ----------
 * [1]:  Said, A. “Introduction to Arithmetic Coding - Theory and Practice.” 
 *       Hewlett Packard Laboratories Report: 2004–2076.
 *       www.hpl.hp.com/techreports/2004/HPL-2004-76.pdf
 *
 */


#include <stdint.h> // for fixed width integer types
#include <stdlib.h> // for size_t
#include <stdio.h>  // for exception handling
#include <string.h> // for memset
#include <math.h>   // for pow

typedef uint8_t   u8;
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

//
// Byte Stream
// - push appends to end
// - pop  removes from front
// - models a FIFO stream
// - push/pop are not meant to work together.  That is, can't simultaneously
//   encode and decode on the same stream; Can't interleave push/pop.
//
typedef struct _stream_t
{ size_t nbytes; //capacity
  size_t ibyte;  //current byte
  u8*    d;      //data
  int    own;    //ownship flag: should this object be responsible for freeing d
} stream_t;

static void attach(stream_t *self, u8* d, size_t n)
{ if(self->own) SAFE_FREE(self->d);
  self->d = d;
  self->own = 0;
  self->nbytes = n;
}

static void maybe_init(stream_t *self)
{ if(!self->d)
  { memset(self,0,sizeof(*self));
    TRY(self->d=malloc(self->nbytes=4096));
    self->own=1;
  }
  return;
Error:
  abort();
}

static void maybe_resize(stream_t *s)
{ if(s->ibyte>=s->nbytes)
    TRY(s->d = realloc(s->d,s->nbytes=(1.2*s->ibyte+50)));
  return;
Error:
  abort();
}

static void push(stream_t *self, u8 v)
{ //printf("Push: %x"ENDL,v);
  self->d[self->ibyte] = v;
  self->ibyte++;
  maybe_resize(self);
}

static u8 pop(stream_t *self)
{ if(self->ibyte>=self->nbytes)
    return 0;
  return self->d[self->ibyte++];
}

//
// Encoder/Decoder state
//

typedef struct _state_t
{ u64      b,l,v;
  stream_t d;
  size_t   nsym;
  u64      D,shift,mask,lowl;
  u64     *cdf;
} state_t;

static void init(state_t *state,u8 *buf,size_t nbuf,real *cdf,size_t nsym)
{ int P;
  memset(state,0,sizeof(*state));
  
  state->D = 1ULL<<(8*sizeof(*buf));
  P = sizeof(u64)/sizeof(*buf)/2;    // need 2P for multiplies
  TRY(P > 2);                        // see requirement induced by eselect()
  state->shift = P * 8 * sizeof(*buf);
  state->lowl = 1ULL<<(state->shift-8*sizeof(*buf)); 

  state->l = (1ULL<<state->shift)-1; // e.g. 2^32-1 for u64
  state->mask = state->l;            // for modding a u64 to u32 with &

  TRY( state->cdf=malloc(nsym*sizeof(*state->cdf)) );
  { size_t i;
    real s = pow(2,state->shift);
    for(i=0;i<nsym;++i)
      state->cdf[i] = s*cdf[i];
    state->nsym = nsym;
  }

  attach(&state->d,buf,nbuf);
  maybe_init(&state->d); // in case buf is NULL (for encoding)
  return;
Error:
  abort();
}

static  void free_internal(state_t *state)
{ SAFE_FREE(state->cdf);
}

//
// Build CDF
// 
static u32 maximum(u32 *s, size_t n)
{ u32 max=0;
  while(n--)
    max = (s[n]>max)?s[n]:max;
  return max;
}

void cdf_build(real **cdf, size_t *M, u32 *s, size_t N)
{ size_t i,nbytes;
  *M = maximum(s,N)+1; 
  TRY( *cdf=realloc(*cdf,nbytes=sizeof(real)*(M[0]+1)) ); // cdf has M+1 elements
  memset(*cdf,0,nbytes);
  for(i=0;i<     N;++i) cdf[0][s[i]]++;                   // histogram
  for(i=0;i<M[0]  ;++i) cdf[0][s[i]]/=(real)N;            // norm
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

static void carry(state_t *state)
{ size_t n = STREAM->ibyte-1; // point to last written symbol
  while( DATA[n]==(D-1) )
    DATA[n--]=0;
  DATA[n]++;
}

static void update(u32 s,state_t *state)
{ u64 a,x,y;
  // end of interval
  y = L;
  if(s!=(NSYM-1)) //is not last symbol
    y = (y*C[s+1])>>SHIFT;
  a = B;
  x = (L*C[s])>>SHIFT;
  B = (B+x)&MASK;
  L = y-x;
  TRY(L>0);
  if(a>B)
    carry(state);
  return;
Error:
  abort();
}


static void erenorm(state_t *state)
{ 
  const int s = SHIFT-bitsofD;
  while(L<LOWL)
  { push(STREAM, B>>s ); // push top bits off of B.
    L = (L<<bitsofD)&MASK;
    B = (B<<bitsofD)&MASK;
  }
}

static  void eselect(state_t *state)
{ u64 a;
  a=B;
  B=(B+(1ULL<<(SHIFT-bitsofD-1)) )&MASK; // D^(P-1)/2: (2^8)^(4-1)/2 = 2^24/2 = 2^23 = 2^(32-8-1)
  L=(1ULL<<(SHIFT-2*bitsofD))-1;         // requires P>2
  if(a>B)
    carry(state);
  erenorm(state);                        // output last 2 symbols
}

static void estep(state_t *state,u32 s)
{ 
  update(s,state);
  if(L<LOWL)
    erenorm(state);
}

void encode(u8 **out, size_t *nout, u32 *in, size_t nin, real *cdf, size_t nsym)
{ size_t i;
  state_t s,*state=&s;
  init(state,*out,*nout,cdf,nsym);   
  for(i=0;i<nin;++i)
    estep(state,in[i]);
  eselect(state);
  *out  = DATA;
  *nout = STREAM->ibyte; // ibyte points one past last written byte
  free_internal(state);
}

void encode_u8(u8 **out, size_t *nout, u8 *in, size_t nin, real *cdf, size_t nsym)
{ size_t i;
  state_t s,*state=&s;
  init(state,*out,*nout,cdf,nsym);   
  for(i=0;i<nin;++i)
    estep(state,in[i]);
  eselect(state);
  *out  = DATA;
  *nout = STREAM->ibyte; // ibyte points one past last written byte
  free_internal(state);
}

//
// Decode
//

static u32 dselect(state_t *state, u64 *v)
{ u32 s,n;
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
  return s;
}

static void drenorm(state_t *state, u64 *v)
{ 
  while(L<LOWL)
  { *v = ((*v<<bitsofD)&MASK)+pop(STREAM);
    L =   ( L<<bitsofD)&MASK;
  }
}

static void dprime(state_t *state, u64* v) // Prime the pump
{
  size_t i;
  for(i=bitsofD;i<=SHIFT;i+=bitsofD) // 8,16,24,32
    *v += (1ULL<<(SHIFT-i))*pop(STREAM); //(2^8)^(P-n) = 2^(8*(P-n))
}

static u32 dstep(state_t *state,u64 *v)
{ 
  u32 s = dselect(state,v);
  if( L<LOWL )
    drenorm(state,v);
  return s;
}

void decode(u32 *out, size_t nout, u8 *in, size_t nin, real *c, size_t nsym)
{ state_t s,*state=&s;
  u64 v=0;
  size_t i=0;

  init(state,in,nin,c,nsym);
  dprime(state,&v);
  for(i=0;i<nout;++i)
    out[i]=dstep(state,&v);
  free_internal(state);
}

//
// Variable output alphabet encoding
//

// can't quite do the whole thing symbol by symbol 
// due to carries.  Ideally, I'd output a settled
// symbol from the encoder when I could.  The carry
// might not happen till the end though...that would
// be the whole message!
void vencode(u8 **out, size_t *nout, size_t noutsym, u32 *in, size_t nin, size_t ninsym, real *cdf)
{ u8 *t=NULL;
  size_t i,n=0;
  real *tcdf;
  TRY( tcdf=malloc(sizeof(*tcdf)*(noutsym+1)) );
  { size_t i;
    real v = 1.0/(real)noutsym;
    for(i=0;i<=noutsym;++i)
      tcdf[i] = i*v;
  }
  { state_t  s1;
    stream_t d ={0};
    u8 *buf=0;
    u64 v;
    n = 0;
    encode(&buf,&n,in,nin,cdf,ninsym);  //first  encoding
    d.d      = *out;
    d.nbytes = *nout;
    maybe_init(&d);
    init(&s1,buf,n,tcdf,noutsym);       //decode with tcdf
    dprime(&s1,&v);
    for(i=0;i<100;++i)                    // FIXME: how to know how many symbols to decode?
      push(&d,dstep(&s1,&v));             // what's the min length to recapitulate the orig. message?
    *out  = d.d;
    *nout = d.ibyte;
    free_internal(&s1);
    free(buf);
  }
  free(tcdf);
  return;
Error:
  abort();
} 

void vdecode(u32 *out, size_t nout, size_t noutsym, u8 *in, size_t nin, size_t ninsym, real *cdf)
{ u8 *t=NULL;
  size_t i,n=0;
  real *tcdf;
  TRY( tcdf=malloc(sizeof(*tcdf)*(ninsym+1)) );
  { size_t i;
    real v = 1.0/(real)ninsym;
    for(i=0;i<=ninsym;++i)
      tcdf[i] = i*v;
  }
  { u8 *buf=0;
    n = 0;
    encode_u8(&buf,&n,in,nin,tcdf,ninsym); //encode with tcdf
    decode(out,nout,buf,n,cdf,noutsym);
    free(buf);
  }
  free(tcdf);
  return;
Error:
  abort();
}
