# tflac

A single-file [FLAC](https://xiph.org/flac/) encoding library. Does not
use any C library functions, does not allocate memory directly (it leaves
that up to you to figure out). Compatible with C89.

Use case: you want to generate FLAC audio without requiring an external
library, and simplicity is your main concern, not compression or speed.

## Status

tflac currently supports the following subframe encodings:

* `CONSTANT`
* `FIXED`
* `VERBATIM`

It's roughly equivalent to compressing with FLAC's "-0" switch.

## Building

In one C file define `TFLAC_IMPLEMENTATION` before including `tflac.h`.

```c
#define TFLAC_IMPLEMENTATION
#include "tflac.h"
```

### Options

There's a few compile-time options you can set:

* Define `TFLAC_32BIT_ONLY` and tflac will not use 64-bit types at all,
and instead emulate 64-bit types with a pair of 32-bit ints.
* Define `TFLAC_DISABLE_SSE2` to disable SSE2 detection.
* Define `TFLAC_PUBLIC` if you need to customize function decorators
for public API functions.
* Define `TFLAC_PRIVATE` if you need to customize function decorators
for private API functions.

For example, if you're building a DLL on Windows and want to export
the public API functions you could define `TFLAC_PUBLIC` as
`__declspec(dllexport)` like so:

```c
#define TFLAC_IMPLEMENTATION
#define TFLAC_PUBLIC __declspec(dllexport)
#include "tflac.h"
```

## Usage

### Detect your CPU features

Call `tflac_detect_cpu()` before doing anything, like as early in your
program as possible, before you create any threads etc.

This will attempt to check your CPU type, and set some internal
global variables. For example, if your CPU supports SSE2, the library
will swap the default fixed-order calculators for a set that use
SSE2.

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
`tflac_s16` or `tflac_s32`. The encode functions are:

* `tflac_encode_s16i`: `tflac_s16` samples in a 1-dimensional array (`tflac_s16*`), interleaved.
* `tflac_encode_s16p`: `tflac_s16` samples in a 2-dimensional array (`tflac_s16**`).
* `tflac_encode_s32i`: `tflac_int32` samples in a 1-dimensional array (`tflac_int32*`), interleaved.
* `tflac_encode_s32p`: `tflac_int32` samples in a 2-dimensional array (`tflac_int32**`).

`tflac_s16` is a typedef for `int16_t`, and `tflac_s32` is a typedef for `int32_t`
in most cases.

```C

#define TFLAC_IMPLEMENTATION
#include "tflac.h"

#define BLOCKSIZE 1152
#define CHANNELS 2
#define BITDEPTH 16
#define SAMPLERATE 44100

int main(int argc, const char *argv[]) {
    /* get the size we need to alloc for tflac internal memory */
    tflac_u32 memory_size = tflac_size_memory(BLOCKSIZE);

    /* get a chunk for tflac internal memory */
    void* memory = malloc(memory_size);

    /* get the size we need to alloc for the output buffer */
    tflac_u32 buffer_size = tflac_size_frame(BLOCKSIZE, CHANNELS, BITDEPTH);

    /* get a hunk for our buffer */
    void* buffer = malloc(buffer_size);

    /* this will be used to know how much of the buffer to write
       after calls to encode */
    tflac_u32 buffer_used = 0;

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
        tflac_s32** samples = get_some_audio_somehow();

        /* encode your audio samples into a FLAC frame */
        tflac_encode_s32p(&t, BLOCKSIZE, samples, buffer, buffer_size, &buffer_used);

        /* and write it out */
        fwrite(buffer, 1, buffer_used, output);
    }

    free(buffer);
    free(ptr);
}
```


## LICENSE

BSD Zero Clause (see the `LICENSE` file).
