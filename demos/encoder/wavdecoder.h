#include <stdio.h>
#include <stdint.h>

/* a basic WAV decoder, just works with FILE*, enough for our demo purposes */

struct wav_decoder {
    FILE* input;
    int (*readsample)(struct wav_decoder*, int32_t* sample);
    uint32_t length; /* the length of the data chunk, in samples */
    uint16_t channels;
    uint32_t samplerate;
    uint16_t bitdepth; /* already has wasted_bits subtracted */
    uint32_t channelmask;

    uint16_t wasted_bits;
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
int wav_decoder_decode(wav_decoder*, int32_t* buffer, uint32_t len, uint32_t* read);
