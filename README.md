# Arithmetic Coding

## Introduction

Implementation after Amir Said's Algorithms 22-29([1]).  This was mostly a
learning exercise.  It hasn't been optimized.

[1]:  http://www.hpl.hp.com/techreports/2004/HPL-2004-76.pdf
      Said, A. "Introduction to Arithmetic Coding - Theory and Practice."
      Hewlett Packard Laboratories Report: 2004-2076.
      
## Features

  - Encoding/decoding to/from variable symbol alphabets.

  - Implicit use of a STOP symbol means that you don't need to know the number of symbols in the decoded message in order
    to decode something.

  - Can encoded messages stored as signed or unsigned chars, shorts, longs or long longs.  Keep in mind, however, that the
    number of encodable symbols may be limiting.  You can't encode 2^64 different integers, sorry.

  - Can encode to streams of variable symbol width; either 1,4,8, or 16 bits.  There is are two tradeoffs here.

      - Smaller bit-width (e.g. 1) give better compression than larger bit-width (e.g. 16), but compression is slower (I think?).

      - The implimentation puts a limit on the smallest probability of an encoded symbol.  Smaller bit-width (e.g. 1) can accomidate
        a larger range of probabilities than large bit-width (e.g. 16).
        
## Example

```C
void encode()
{ unsigned char *input, *output=0;      // input and output buffer
  size_t countof_input, countof_output; // number of symbols in input and output buffer
  float *cdf=0;
  size_t nsym;                          // number of symbols in the input alphabet
  // somehow load the data into input array
  cdf_build(&cdf,&nsym,input,countof_input);  // get the symbol statistics
  encode_u1_u8(                         // encode unsigned chars to a string of bits (1 bit per output symbol)
    (void**)&out,&countof_output,
           input, countof_input,
             cdf, nsym);
  // do something with the output
  free(out);
  free(cdf);
}
```        


      
