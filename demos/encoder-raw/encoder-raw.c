#define TFLAC_IMPLEMENTATION
#include "tflac.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <inttypes.h>

/* example that reads in a headerless WAV file and writes
 * out a FLAC file. assumes WAV is 16-bit, 2channel, 44100Hz */

/* headerless wav can be created via ffmpeg like:
 *     ffmpeg -i your-audio.mp3 -ar 44100 -ac 2 -f s16le your-audio.raw
 */

#define FRAME_SIZE 1152
#define SAMPLERATE 44100
#define BITDEPTH 16
#define CHANNELS     2
#define SAMPLESIZE   2

static uint16_t unpack_u16le(const uint8_t* d) {
    return (((uint16_t)d[0])    ) |
           (((uint16_t)d[1])<< 8);
}

static int16_t unpack_s16le(const uint8_t* d) {
    return (int16_t)unpack_u16le(d);
}

void
repack_samples(int16_t *s, uint32_t channels, uint32_t num) {
    uint32_t i = 0;
    while(i < (channels*num)) {
        s[i] = unpack_s16le( (uint8_t*) (&s[i]) );
        i++;
    }
}

int main(int argc, const char *argv[]) {
    uint8_t *buffer = NULL;
    uint32_t bufferlen = 0;
    uint32_t bufferused = 0;
    FILE *input = NULL;
    FILE *output = NULL;
    uint32_t frames = 0;
    int16_t *samples = NULL;
    void *tflac_mem = NULL;
    unsigned int i = 0;
    unsigned int j = 0;
    unsigned int dump_subframe_types = 0;
    unsigned int dump_sizes = 0;
    tflac t;

    if(argc < 3) {
        printf("Usage: %s /path/to/raw /path/to/flac\n",argv[0]);
        return 1;
    }

    tflac_init(&t);

    t.samplerate = SAMPLERATE;
    t.channels = CHANNELS;
    t.bitdepth = BITDEPTH;
    t.blocksize = FRAME_SIZE;
    t.max_partition_order = 4;

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

    if(dump_sizes) {
        printf("tflac struct size: %u\n", tflac_size());
        printf("tflac memory size: %u\n", tflac_size_memory(t.blocksize));
        printf("tflac max frame size: %u\n", tflac_size_frame(t.blocksize,t.channels,t.bitdepth));
    }

    tflac_mem = malloc(tflac_size_memory(t.blocksize));
    if(tflac_mem == NULL) abort();

    tflac_set_constant_subframe(&t, 1);
    tflac_set_fixed_subframe(&t, 1);

    if(tflac_validate(&t, tflac_mem, tflac_size_memory(t.blocksize)) != 0) abort();

    /* we could also use the tflac_size_frame() function */
    bufferlen = TFLAC_SIZE_FRAME(FRAME_SIZE,CHANNELS,BITDEPTH);
    buffer = malloc(bufferlen);
    if(buffer == NULL) abort();

    samples = (int16_t *)malloc(sizeof(int16_t) * CHANNELS * FRAME_SIZE);
    if(!samples) abort();

    fwrite("fLaC",1,4,output);

    /* we'll write out an empty STREAMINFO block and overwrite later to get MD5 checksum
     * and sample count */
    tflac_encode_streaminfo(&t, 1, buffer, bufferlen, &bufferused);
    fwrite(buffer,1,bufferused,output);

    while((frames = fread(samples,sizeof(int16_t) * CHANNELS, FRAME_SIZE, input)) > 0) {
        repack_samples(samples, CHANNELS, frames);

        if(tflac_encode_int16i(&t, frames, samples, buffer, bufferlen, &bufferused) != 0) abort();
        fwrite(buffer,1,bufferused,output);
    }

    /* this will calculate the final MD5 */
    tflac_finalize(&t);

    /* now we overwrite our original STREAMINFO with an updated one */
    fseek(output,4,SEEK_SET);
    tflac_encode_streaminfo(&t, 1, buffer, bufferlen, &bufferused);
    fwrite(buffer,1,bufferused,output);

    if(dump_subframe_types) {
        for(i=0;i<2;i++) {
            printf("channel %u:\n",i+1);
            for(j=0;j<TFLAC_SUBFRAME_TYPE_COUNT;j++) {
                printf("  %s: %" PRIu64 "\n", tflac_subframe_types[j], t.subframe_type_counts[i][j]);
            }
        }
    }

    fclose(input);
    fclose(output);
    free(tflac_mem);
    free(samples);
    free(buffer);

    return 0;
}

