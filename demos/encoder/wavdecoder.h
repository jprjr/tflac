#include <stdio.h>

/* including tflac.h for types */
#include "tflac.h"

/* a basic WAV decoder, just works with FILE*, enough for our demo purposes */

struct wav_decoder {
    FILE* input;
    int (*readsample)(struct wav_decoder*, tflac_s32* sample);
    tflac_u32 length; /* the length of the data chunk, in samples */
    tflac_u16 channels;
    tflac_u32 samplerate;
    tflac_u16 bitdepth; /* already has wasted_bits subtracted */
    tflac_u32 channelmask;

    tflac_u16 wasted_bits;
};

typedef struct wav_decoder wav_decoder;

#define WAV_DECODER_ZERO { \
  .input = NULL, \
  .readsample = NULL, \
  .length = 0, \
  .channels = 0, \
  .samplerate = 0, \
  .bitdepth = 0, \
  .wasted_bits = 0, \
}

int wav_decoder_open(wav_decoder*, FILE*);
int wav_decoder_decode(wav_decoder*, tflac_s32* buffer, tflac_u32 len, tflac_u32* read);
