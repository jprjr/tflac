#define TFLAC_IMPLEMENTATION
#include "tflac.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>


/* headerless wav can be created via ffmpeg like:
 *     ffmpeg -i your-audio.mp3 -ar 44100 -ac 2 -f s16le your-audio.raw
 */

#define FRAME_SIZE   1152
#define SAMPLERATE  44100
#define BITDEPTH       16
#define CHANNELS        2

/* example that reads in a headerless WAV file and writes
 * out a FLAC file. assumes WAV has the defined parameters above */

#if (BITDEPTH == 16)

static tflac_u16 unpack_u16le(const tflac_u8* d) {
    return (((tflac_u16)d[0])    ) |
           (((tflac_u16)d[1])<< 8);
}

static tflac_s16 unpack_s16le(const tflac_u8* d) {
    return (tflac_s16)unpack_u16le(d);
}

void
repack_samples(tflac_s16 *s, tflac_u32 channels, tflac_u32 num) {
    tflac_u32 i = 0;
    while(i < (channels*num)) {
        s[i] = unpack_s16le( (tflac_u8*) (&s[i]) );
        i++;
    }
}

typedef tflac_s16 sample;
#define ENCODE_FUNC tflac_encode_s16i

#elif (BITDEPTH == 32)
static tflac_u32 unpack_u32le(const tflac_u8* d) {
    return (((tflac_u32)d[0])     ) |
           (((tflac_u32)d[1])<< 8 ) |
           (((tflac_u32)d[2])<< 16) |
           (((tflac_u32)d[3])<< 24);
}

static tflac_s32 unpack_s32le(const tflac_u8* d) {
    return (tflac_s32)unpack_u32le(d);
}

void
repack_samples(tflac_s32 *s, tflac_u32 channels, tflac_u32 num) {
    tflac_u32 i = 0;
    while(i < (channels*num)) {
        s[i] = unpack_s32le( (tflac_u8*) (&s[i]) );
        i++;
    }
}

typedef tflac_s32 sample;
#define ENCODE_FUNC tflac_encode_s32i
#else
#error "unsupported bit depth"
#endif

#define DUMP_SIZES 1
#define DUMP_COUNTS 1

int main(int argc, const char *argv[]) {
    tflac_u8 *buffer = NULL;
    tflac_u32 bufferlen = 0;
    tflac_u32 bufferused = 0;
    FILE *input = NULL;
    FILE *output = NULL;
    tflac_u32 frames = 0;
    sample *samples = NULL;
    void *tflac_mem = NULL;
    tflac t;

#if DUMP_SIZES
    printf("tflac_size(): %u\n", tflac_size());
    printf("tflac_size_memory(%u): %u\n", FRAME_SIZE, tflac_size_memory(FRAME_SIZE));
    printf("TFLAC_SIZE_MEMORY(%u): %lu\n", FRAME_SIZE, TFLAC_SIZE_MEMORY(FRAME_SIZE));
    printf("tflac_size_frame(%u,%u,%u): %u\n", FRAME_SIZE, CHANNELS, BITDEPTH, tflac_size_frame(FRAME_SIZE, CHANNELS, BITDEPTH));
    printf("TFLAC_SIZE_FRAME(%u,%u,%u): %lu\n", FRAME_SIZE, CHANNELS, BITDEPTH, TFLAC_SIZE_FRAME(FRAME_SIZE, CHANNELS, BITDEPTH));
#endif

    if(argc < 3) {
        printf("Usage: %s /path/to/raw /path/to/flac\n",argv[0]);
        return 1;
    }

    if(tflac_size_memory(FRAME_SIZE) != TFLAC_SIZE_MEMORY(FRAME_SIZE)) {
        printf("Error with needed memory size: %u != %lu\n",
          tflac_size_memory(FRAME_SIZE),TFLAC_SIZE_MEMORY(FRAME_SIZE));
        return 1;
    }

    if(tflac_size_frame(FRAME_SIZE,CHANNELS,BITDEPTH) != TFLAC_SIZE_FRAME(FRAME_SIZE,CHANNELS,BITDEPTH)) {
        printf("Error with needed frame size: %u != %u\n",
          tflac_size_frame(FRAME_SIZE,CHANNELS,BITDEPTH),TFLAC_SIZE_FRAME(FRAME_SIZE,CHANNELS,BITDEPTH));
        return 1;
    }

    tflac_detect_cpu();
    tflac_init(&t);

    t.samplerate = SAMPLERATE;
    t.channels = CHANNELS;
    t.bitdepth = BITDEPTH;
    t.blocksize = FRAME_SIZE;
    t.max_partition_order = 3;
    t.enable_md5 = 1;

    if(strcmp(argv[1],"-") == 0) {
        input = stdin;
    } else {
        input = fopen(argv[1],"rb");
    }

    if(input == NULL) return 1;

    output = fopen(argv[2],"wb");
    if(output == NULL) {
        fclose(input);
        return 1;
    }


    tflac_mem = malloc(tflac_size_memory(t.blocksize));
    if(tflac_mem == NULL) abort();

    tflac_set_constant_subframe(&t, 1);
    tflac_set_fixed_subframe(&t, 1);

    if(tflac_validate(&t, tflac_mem, tflac_size_memory(t.blocksize)) != 0) abort();

    bufferlen = tflac_size_frame(FRAME_SIZE,CHANNELS,BITDEPTH);
    buffer = malloc(bufferlen);
    if(buffer == NULL) abort();

    samples = (sample *)malloc(sizeof(sample) * CHANNELS * FRAME_SIZE);
    if(!samples) abort();

    fwrite("fLaC",1,4,output);

    /* we'll write out an empty STREAMINFO block and overwrite later to get MD5 checksum
     * and sample count */
    tflac_encode_streaminfo(&t, 1, buffer, bufferlen, &bufferused);
    fwrite(buffer,1,bufferused,output);

    while((frames = fread(samples,sizeof(sample) * CHANNELS, FRAME_SIZE, input)) > 0) {
        repack_samples(samples, CHANNELS, frames);

        if(ENCODE_FUNC(&t, frames, samples, buffer, bufferlen, &bufferused) != 0) abort();
        fwrite(buffer,1,bufferused,output);
    }

    /* this will calculate the final MD5 */
    tflac_finalize(&t);

    /* now we overwrite our original STREAMINFO with an updated one */
    fseek(output,4,SEEK_SET);
    tflac_encode_streaminfo(&t, 1, buffer, bufferlen, &bufferused);
    fwrite(buffer,1,bufferused,output);

#if DUMP_COUNTS
    do {
        unsigned int i,j;
        for(i=0;i<2;i++) {
            printf("channel %u:\n",i+1);
            for(j=0;j<TFLAC_SUBFRAME_TYPE_COUNT;j++) {
#ifdef TFLAC_32BIT_ONLY
                printf("  %s: %u\n", tflac_subframe_types[j], t.subframe_type_counts[i][j].lo);
#else
                printf("  %s: %lu\n", tflac_subframe_types[j], t.subframe_type_counts[i][j]);
#endif
            }
        }
    } while(0);
#endif

    fclose(input);
    fclose(output);
    free(tflac_mem);
    free(samples);
    free(buffer);

    return 0;
}

