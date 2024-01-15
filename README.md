# tflac

A single-file [FLAC](https://xiph.org/flac/) encoding library. Does not
use any C library functions, does not allocate memory directly (it leaves
that up to you to figure out).

Use case: you want to generate FLAC audio without requiring an external
library, and simplicity is your main concern, not compression or speed.

## Status

tflac currently supports the following subframe encodings:

* `CONSTANT`
* `FIXED`
* `VERBATIM`

It's roughly equivalent to compressing with FLAC's "-0" switch but probably
not as good - this doesn't attempt left/side, right/side, or mid/side
encoding, which would probably help compression a bit.

## Building

In one C file define `TFLAC_IMPLEMENTATION` before including `tflac.h`.

Additionally if you need to customize function decorators you can define
`TFLAC_PUBLIC` and/or `TFLAC_PRIVATE` before including `tflac.h`. For
example, if you're building a DLL on Windows you could have:

```c
#define TFLAC_IMPLEMENTATION
#define TFLAC_PUBLIC __declspec(dllexport)
#include "tflac.h"
```

I suspect in most cases you won't have to set this.

Lastly, a few parts of the code use 64-bit data types. You
can specify that you want all operations to be 32-bit by
defining `TFLAC_NO_64BIT`. One warning: this will probably
fail with bit depths > 28, I haven't thoroughly tested
that.

## Usage

### Get memory and initialize things.

You'll have to create a tflac struct. The whole struct definition is
provided so you can allocate it on the stack, or call `tflac_size()`
to get the structure size at runtime. Initialize it with `tflac_init()`.

You then need to set 4 parameters. You can either set the tflac struct fields
yourself, or use setter functions (`tflac_set_blocksize()`,
`tflac_set_samplerate()`, etc).

1. audio block size
2. channels
3. bit depth
4. sample rate

You then need to acquire a memory chunk, to be used by tflac internally
for storing residuals. You can get this needed memory size with a C
macro, or with a function. The only required parameter is your audio block
size.

Once you've set your parameters and acquired memory, call `tflac_validate()`
with the memory you've allocated for tflac's internals. It will make sure
your parameters are sane, that the memory chunk is large enough, and
proceed to set up all of its internal pointers to the memory chunk.

Note: the library will not free this memory for you since the library
didn't allocate it.

### Generate some FLAC!

You'll need to write out the `fLaC` stream marker and a `STREAMINFO` block.
The library has a function for generating a `STREAMINFO` block, since
that's a required metadata block. Other blocks are pretty straightforward
to create, I don't plan to include that functionality.

You'll need a buffer for storing encoded frames. You can find the maximum
required buffer size with a C macro, or with a function. The parameters
are your audio block size, channels, and bit depth.

You can have your audio in interleaved or planar format, and as either
`int16_t` or `int32_t`. The encode functions are:

* `tflac_encode_int16i`: `int16_t` samples in a 1-dimensional array (`int16_t*`), interleaved.
* `tflac_encode_int16p`: `int16_t` samples in a 2-dimensional array (`int16_t**`).
* `tflac_encode_int32i`: `int32_t` samples in a 1-dimensional array (`int32_t*`), interleaved.
* `tflac_encode_int32p`: `int32_t` samples in a 2-dimensional array (`int32_t**`).

```C

#define TFLAC_IMPLEMENTATION
#include "tflac.h"

#define BLOCKSIZE 1152
#define CHANNELS 2
#define BITDEPTH 16
#define SAMPLERATE 44100

int main(int argc, const char *argv[]) {
    /* get the size we need to alloc for tflac internal memory */
    uint32_t memory_size = tflac_size_memory(BLOCKSIZE);

    /* get a chunk for tflac internal memory */
    void* memory = malloc(memory_size);

    /* get the size we need to alloc for the output buffer */
    uint32_t buffer_size = tflac_size_frame(BLOCKSIZE, CHANNELS, BITDEPTH);

    /* get a hunk for our buffer */
    void* buffer = malloc(buffer_size);

    /* this will be used to know how much of the buffer to write
       after calls to encode */
    uint32_t buffer_used = 0;

    tflac t;

    tflac_init(&t);

    /* set some parameters */
    t.blocksize = BLOCKSIZE;
    t.samplerate = SAMPLERATE;
    t.bitdepth = BITDEPTH;
    t.channels = CHANNELS;

    /* validate our parameters and tell tflac what memory to use */
    tflac_validate(&t, memory, memory_size);

    /* open a file and write out the stream marker */
    FILE* output = fopen("some-file.flac","wb");
    fwrite("fLaC", 1, 4, output);

    /* encode the STREAMINFO block and write it out */
    tflac_encode_streaminfo(&t, 1, buffer, buffer_size, &buffer_used);
    fwrite(buffer, 1, buffer_used, output);

    /* loop until you run out of audio */
    while(have_audio()) {

        /* hand-waving over this part, here we're using a 2d array of audio samples
        and assuming it's always BLOCKSIZE samples */
        int32_t** samples = get_some_audio_somehow();

        /* encode your audio samples into a FLAC frame */
        tflac_encode_int32p(&t, BLOCKSIZE, samples, buffer, buffer_size, &buffer_used);

        /* and write it out */
        fwrite(buffer, 1, buffer_used, output);
    }

    free(buffer);
    free(ptr);
}
```


## LICENSE

BSD Zero Clause (see the `LICENSE` file).
