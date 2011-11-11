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

typedef uint8_t   u8;
typedef uint32_t  u32;
typedef uint64_t  u64;
typedef float     real;

#define ENDL "\n"
#define TRY(e) \
  do{ if(!(e)) {\
    printf("%s(%d):"ENDL "%s"ENDL "Expression evaluated as false."ENDL, \
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
{ size_t nbtyes; //capacity
  size_t ibyte;  //current byte
  u8*    d;      //data
  int    own;    //ownship flag: should this object be responsible for freeing d
} stream_t;

static
void attach(stream_t *self, u8* d, size_t n)
{ if(self->own) SAFE_FREE(self->d);
  self->d = d;
  self->own = 0;
  self->nbytes = n;
}

static
void maybe_init(stream_t *self)
{ if(!self->d)
  { memset(self,0,sizeof(*self));
    TRY(self->d=malloc(self->nbytes=4096));
    self->own=1;
  }
  return;
Error:
  abort();
}

static
void maybe_resize(stream_t *s)
{ if(s->ibyte>=s->nbytes)
    TRY(*d = realloc(*d,s->nbytes=(1.2*s->ibyte+50)));
  return;
Error:
  abort();
}

static
void push(stream_t *self, u8 v)
{ 
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

struct _state_t
{ u64      b,l,v;
  stream_t d;
  size_t   nsym;
  u64      D,P,shift,mask;
  u64     *cdf;
} state_t;

static
void init(state_t *state,u8 *buf,size_t nbuf,real *cdf,size_t nsym)
{
  memset(state,0,sizeof(*state));
  
  state->D = 1<<(8*sizeof(*out));
  state->P = sizeof(u64)/sizeof(*out)/2; // need 2P for multiplies
  TRY(shift->P > 2); // see requirement induced by eselect()
  state->shift = state->P * 8 * sizeof(*out);

  state->l = (1<<state->shift)-1; // e.g. 2^32-1 for u64
  state->mask = state->l;         // for modding a u64 to u32 with &

  TRY( state->cdf=malloc(nsym*sizeof(*state->cdf)) );
  { size_t i;
    real s = pow(2,state->shift);
    for(i=0;i<nsym;++i)
      state->cdf[i] = s*cdf[i];
  }

  attach(&state->d,buf,nbuf);
  maybe_init(&state->d); // in case buf is NULL (for encoding)
Error:
  abort();
}

static 
void free_internal(state_t *state)
{ SAFE_FREE(state->cdf);
}

//
// Build CDF
// 
static
u32 maximum(u32 *s, size_t n)
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

#define B       (state->b)
#define L       (state->l)
#define C       (state->cdf)
#define SHIFT   (state->shift)
#define NSYM    (state->nsym)
#define MASK    (state->mask)
#define STREAM  (state->d)
#define DATA    (state->d->d)
#define bitsofD (8)
#define D       (1<<bitsofD(D))

static
void carry(state_t *state)
{ size_t n = STREAM->ibyte;
  while( DATA[n]==(D-1) )
    DATA[n--]=0;
  DATA[n]++;
}

static
void update(u32 s,state_t *state)
{ u64 a,x,y;
  // end of interval
  y = L;
  if(s!=NSYM-1) //is not last symbol
    y = (y*CDF[s+1])>>SHIFT;
  a = B;
  x = L*CDF[s])>>SHIFT;
  B = (B+x)&MASK;
  L = y-x;
  if(a>B)
    carry(state);
}


static
void erenorm(state_t &state)
{ const u64 lowl = 1<<(SHIFT-1);
  while(L<lowl)
  { push(STREAM, B>>(SHIFT-1) ); // push top bits off of B.  D=2^8,1-P=1-4=-3,D^(1-P)=(2^8)^(-3)=2^-24
    L = (L<<bitsofD)&MASK;
    B = (B<<bitsofD)&MASK;
  }
}
                                
static 
void eselect(state_t &state);
{ u64 a;
  a=B;
  B=(B+(1<<(SHIFT-bitsofD-1)))&MASK; // D^(P-1)/2: (2^8)^(4-1)/2 = 2^24/2 = 2^23 = 2^(32-8-1)
  L=(1<<(bitsofD*(P-2)))-1;          // requires P>2
  if(a>B)
    carry(state);
  erenorm(state);
}

static
void estep(state_t *state,u32 s)
{ const u64 lowl = 1<<(SHIFT-1); 
  update(s,&state);
  if(L<lowl)
    erenorm(&state);
}

void encode(u8 **out, size_t *nout, u32 *in, size_t nin, real *cdf, size_t nsym)
{ real   b=0.0, // beginning of the interval
         l=1.0; // length of the interval
  size_t i;
  state_t state;
  init(&state,out,nout,cdf,nsym);   
  for(i=0;i<N;++i)
    estep(in[i],&state);
  eselect(&state);
  // FIXME: cleanup and output
  *out  = DATA;
  *nout = state->d->ibyte+1;
  free_internal(state);
}

//
// Decode
//

static
u32 dselect(state_t *state, u64 *v)
{ u32 s,n;
  u64 x,y;

  s = 0;
  n = NSYM;
  x = 0;
  y = L;
  while( (n-s)>1 )   // bisection search
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

static
void drenorm(real *v,real *b, real *l, stream_t *in)
{ const u64 lowl = 1<<(SHIFT-1);
  while(L<lowl)
  { *v = ((*v<<bitsofD)&MASK)+pop(STREAM);
    L =   ( L<<bitsofD)&MASK;
  }
}

void decode(u32 *out, size_t nout, u8 *in, size_t nin, real *c, size_t nsym)
{ state_t state;
  u64 v=0;
  size_t i;
  const u64 lowl = 1<<(SHIFT-1);  

  init(&state,in,nin,c,nsym);
  // Prime the pump
  for(i=bitsofD;i<=SHIFT;++bitsofD) // 8,16,24,32
    v = (1<<(SHIFT-i))*pop(STREAM); //(2^8)^(P-n) = 2^(8*(P-n))

  for(i=0;i<nout;++i)
  { out[i] = dselect(&state,&v);
    if( L<lowl )
      drenorm(&state,&v);
  }
}
