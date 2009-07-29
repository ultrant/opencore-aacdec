#ifndef RINGBUFFER_H_
#define RINGBUFFER_H_

#include <sys/types.h>

typedef struct  
{
  char *buf;
  size_t len;
} 
ringbuffer_data_t ;

typedef struct
{
  char *buf;
  volatile size_t write_ptr;
  volatile size_t read_ptr;
  size_t size;
  size_t size_mask;
  int mlocked;
} 
ringbuffer_t ;

/* Create a new ringbuffer to hold at least `sz' bytes of data. The
   actual buffer size is rounded up to the next power of two.  */
ringbuffer_t *ringbuffer_create(int sz);

/* Free all data associated with the ringbuffer `rb'. */
void ringbuffer_free(ringbuffer_t *rb);

/* Lock the data block of `rb' using the system call 'mlock'.  */
int ringbuffer_mlock(ringbuffer_t *rb);

/* Reset the read and write pointers to zero. This is not thread
   safe. */
void ringbuffer_reset(ringbuffer_t *rb);

/* Advance the write pointer `cnt' places. */
void ringbuffer_write_advance(ringbuffer_t *rb, size_t cnt);

/* Advance the read pointer `cnt' places. */
void ringbuffer_read_advance(ringbuffer_t *rb, size_t cnt);

/* Return the number of bytes available for writing.  This is the
   number of bytes in front of the write pointer and behind the read
   pointer.  */
size_t ringbuffer_write_space(ringbuffer_t *rb);

/* Return the number of bytes available for reading.  This is the
   number of bytes in front of the read pointer and behind the write
   pointer.  */
size_t ringbuffer_read_space(ringbuffer_t *rb);

/* The copying data reader.  Copy at most `cnt' bytes from `rb' to
   `dest'.  Returns the actual number of bytes copied. */
size_t ringbuffer_read(ringbuffer_t *rb, char *dest, size_t cnt);

/* The copying data writer.  Copy at most `cnt' bytes to `rb' from
   `src'.  Returns the actual number of bytes copied. */
size_t ringbuffer_write(ringbuffer_t *rb, char *src, size_t cnt);

/* The non-copying data reader.  `vec' is an array of two places.  Set
   the values at `vec' to hold the current readable data at `rb'.  If
   the readable data is in one segment the second segment has zero
   length.  */
void ringbuffer_get_read_vector(ringbuffer_t *rb, ringbuffer_data_t *vec);

/* The non-copying data writer.  `vec' is an array of two places.  Set
   the values at `vec' to hold the current writeable data at `rb'.  If
   the writeable data is in one segment the second segment has zero
   length.  */
void ringbuffer_get_write_vector(ringbuffer_t *rb, ringbuffer_data_t *vec);

#endif /*RINGBUFFER_H_*/
