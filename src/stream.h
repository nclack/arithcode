#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdlib.h> // for size_t

//
// Bit Stream
// - will realloc to resize if necessary
// - push appends to end
// - pop  removes from front
// - models a FIFO stream
// - push/pop are not meant to work together.  That is, can't simultaneously
//   encode and decode on the same stream; Can't interleave push/pop.
//
// - carry is somewhat specific to arithmetic coding, but seems like a stream
//   op, so it's included here.  Alternatively, I could add an operation to
//   rewind the stream a bit so that the carry op could be implemented
//   elsewhere.
//

typedef struct _stream_t
{ size_t   nbytes; //capacity
  size_t   ibyte;  //current byte
  size_t   ibit;   //current bit   (for u8 stream this is always 0)
  uint8_t  mask;   //bit is set in the position of the last write
  uint8_t *d;      //data
  int      own;    //ownship flag: should this object be responsible for freeing d [??:used]
} stream_t;

// Attach
// ------
// if <d> is NULL, will allocate some space.
// otherwise, free's the internal buffer (if there is one), 
//            and uses <d> instead.
// <n> is the byte size of <d>
//
// Detach
// ------
// returns the pointer to the streams buffer and it's size via <d> and <n>.
// The stream, disowns (but does not free) the buffer, returning the stream
// to an empty state.
//
void attach  (stream_t *s, void *d, size_t n);
void detach  (stream_t *s, void **d, size_t *n);

void push_u1 (stream_t *s, uint8_t  v);
void push_u4 (stream_t *s, uint8_t  v);
void push_u8 (stream_t *s, uint8_t  v);
void push_u16(stream_t *s, uint16_t v);
void push_u32(stream_t *s, uint32_t v);
void push_u64(stream_t *s, uint64_t v);
void push_i8 (stream_t *s,  int8_t  v);
void push_i16(stream_t *s,  int16_t v);
void push_i32(stream_t *s,  int32_t v);
void push_i64(stream_t *s,  int64_t v);

uint8_t  pop_u1  (stream_t *s);
uint8_t  pop_u4  (stream_t *s);
uint8_t  pop_u8  (stream_t *s);
uint16_t pop_u16 (stream_t *s);
uint32_t pop_u32 (stream_t *s);
uint64_t pop_u64 (stream_t *s);
 int8_t  pop_i8  (stream_t *s);
 int16_t pop_i16 (stream_t *s);
 int32_t pop_i32 (stream_t *s);
 int64_t pop_i64 (stream_t *s);

void carry_u1 (stream_t* s);
void carry_u4 (stream_t* s);
void carry_u8 (stream_t* s);
void carry_u16(stream_t* s);
void carry_u32(stream_t* s);

#ifdef __cplusplus
}
#endif
void carry_u64(stream_t* s);
