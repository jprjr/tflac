#include "wavdecoder.h"

#define TFLAC_IMPLEMENTATION
#include "tflac.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

int main(int argc, const char *argv[]) {
    tflac_u8 *buffer = NULL;
    tflac_u32 bufferlen = 0;
    tflac_u32 bufferused = 0;
    FILE *input = NULL;
    FILE *output = NULL;
    tflac_u32 frames = 0;
    tflac_s32 *samples = NULL;
    void *tflac_mem = NULL;
    tflac_u32 frame_size = 1152;
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
    t.max_partition_order = 3;

    tflac_mem = malloc(tflac_size_memory(t.blocksize));
    if(tflac_mem == NULL) abort();

    tflac_set_constant_subframe(&t, 1);
    tflac_set_fixed_subframe(&t, 1);

    if(tflac_validate(&t, tflac_mem, tflac_size_memory(t.blocksize)) != 0) abort();

    bufferlen = tflac_size_frame(t.blocksize,t.channels,t.bitdepth);
    buffer = malloc(bufferlen);
    if(buffer == NULL) abort();

    samples = (tflac_s32 *)malloc(sizeof(tflac_s32) * t.channels * t.blocksize);
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
        if(tflac_encode_s32i(&t, frames, samples, buffer, bufferlen, &bufferused) != 0) abort();
        fwrite(buffer,1,bufferused,output);
    }

    /* this will calculate the final MD5 */
    tflac_finalize(&t);

    /* now we overwrite our original STREAMINFO with an updated one */
    fseek(output,4,SEEK_SET);
    tflac_encode_streaminfo(&t, 1, buffer, bufferlen, &bufferused);
    fwrite(buffer,1,bufferused,output);

    fclose(input);
    fclose(output);
    free(tflac_mem);
    free(samples);
    free(buffer);

    return 0;
}

