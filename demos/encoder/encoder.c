#define TFLAC_IMPLEMENTATION
#include "tflac.h"
#include "wavdecoder.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <inttypes.h>

int main(int argc, const char *argv[]) {
    uint8_t *buffer = NULL;
    uint32_t bufferlen = 0;
    uint32_t bufferused = 0;
    FILE *input = NULL;
    FILE *output = NULL;
    uint32_t frames = 0;
    int32_t *samples = NULL;
    void *tflac_mem = NULL;
    unsigned int i = 0;
    unsigned int j = 0;
    unsigned int dump_subframe_types = 1;
    unsigned int dump_sizes = 0;
    uint32_t frame_size = 1152;
    wav_decoder w = WAV_DECODER_ZERO;
    tflac t;

    tflac_init(&t);

    if(argc < 3) {
        printf("Usage: %s /path/to/raw /path/to/flac\n",argv[0]);
        return 1;
    }

    if(strcmp(argv[1],"-") == 0) {
        input = stdin;
    } else {
        input = fopen(argv[1],"rb");
    }

    if(input == NULL) return 1;

    if(wav_decoder_open(&w,input) != 0) return 1;

    t.samplerate = w.samplerate;
    t.channels   = w.channels;
    t.bitdepth   = w.bitdepth;
    t.blocksize  = frame_size;
    t.max_partition_order = 4;

    if(dump_sizes) {
        printf("tflac_md5 struct size: %u\n",sizeof(tflac_md5));
        printf("tflac_bitwriter struct size: %u\n",sizeof(tflac_bitwriter));
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
    bufferlen = tflac_size_frame(t.blocksize,t.channels,t.bitdepth);
    buffer = malloc(bufferlen);
    if(buffer == NULL) abort();

    samples = (int32_t *)malloc(sizeof(int32_t) * t.channels * t.blocksize);
    if(!samples) abort();

    output = fopen(argv[2],"wb");
    if(output == NULL) {
        fclose(input);
        return 1;
    }

    fwrite("fLaC",1,4,output);

    /* we'll write out an empty STREAMINFO block and overwrite later to get MD5 checksum
     * and sample count */
    tflac_encode_streaminfo(&t, 1, buffer, bufferlen, &bufferused);
    fwrite(buffer,1,bufferused,output);

    while( wav_decoder_decode(&w, samples, t.blocksize, &frames) == 0) {
        if(tflac_encode_int32i(&t, frames, samples, buffer, bufferlen, &bufferused) != 0) abort();
        fwrite(buffer,1,bufferused,output);
    }

    /* this will calculate the final MD5 */
    tflac_finalize(&t);

    /* now we overwrite our original STREAMINFO with an updated one */
    fseek(output,4,SEEK_SET);
    tflac_encode_streaminfo(&t, 1, buffer, bufferlen, &bufferused);
    fwrite(buffer,1,bufferused,output);

#ifndef TFLAC_DISABLE_COUNTERS
    if(dump_subframe_types) {
        printf("Subframe type counts:\n");
        for(i=0;i<t.channels;i++) {
            printf("  channel %u:\n",i+1);
            for(j=0;j<TFLAC_SUBFRAME_TYPE_COUNT;j++) {
                printf("    %s: %" PRIu64 "\n", tflac_subframe_types[j], t.subframe_type_counts[i][j]);
            }
        }
    }
#else
    (void)dump_subframe_types;
#endif

    fclose(input);
    fclose(output);
    free(tflac_mem);
    free(samples);
    free(buffer);

    return 0;
}

