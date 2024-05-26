#define TFLAC_IMPLEMENTATION
#include "tflac.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <pthread.h>
#include <linux/futex.h>
#include <sys/syscall.h>
#include <unistd.h>


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

typedef union encoder_atomic_int {
    void *align;
    int val;
} encoder_atomic_int;

typedef union encoder_atomic_u32 {
    void *align;
    tflac_u32 val;
} encoder_atomic_u32;

static inline void encoder_atomic_int_store(encoder_atomic_int *i, int val) {
    __atomic_store_n(&i->val, val, __ATOMIC_SEQ_CST);
}

static inline int encoder_atomic_int_load(encoder_atomic_int *i) {
    return (int)__atomic_load_n(&i->val, __ATOMIC_SEQ_CST);
}

static inline int encoder_atomic_int_inc(encoder_atomic_int *i) {
    return (int)__atomic_fetch_add(&i->val, 1, __ATOMIC_SEQ_CST);
}

static inline int encoder_atomic_int_dec(encoder_atomic_int *i) {
    return (int)__atomic_fetch_sub(&i->val, 1, __ATOMIC_SEQ_CST);
}

static inline int encoder_atomic_int_cas(encoder_atomic_int* i, int expected, int val) {
    return __atomic_compare_exchange_n(&i->val, &expected, val, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST) ? expected : !expected;
}

static inline void encoder_atomic_u32_store(encoder_atomic_u32 *i, tflac_u32 val) {
    __atomic_store_n(&i->val, val, __ATOMIC_SEQ_CST);
}

static inline tflac_u32 encoder_atomic_u32_load(encoder_atomic_u32 *i) {
    return (tflac_u32)__atomic_load_n(&i->val, __ATOMIC_SEQ_CST);
}

static inline tflac_u32 encoder_atomic_u32_inc(encoder_atomic_u32 *i) {
    return (tflac_u32)__atomic_fetch_add(&i->val, 1, __ATOMIC_SEQ_CST);
}

static inline tflac_u32 encoder_atomic_u32_dec(encoder_atomic_u32 *i) {
    return (tflac_u32)__atomic_fetch_sub(&i->val, 1, __ATOMIC_SEQ_CST);
}

static inline tflac_u32 encoder_atomic_u32_cas(encoder_atomic_u32* i, tflac_u32 expected, tflac_u32 val) {
    return __atomic_compare_exchange_n(&i->val, &expected, val, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST) ? expected : !expected;
}

/* a struct to wrap an encoder so it can reference a shared sample buffer */
struct encoder {
    tflac t;
    tflac_u8 *buffer;
    tflac_u32 bufferlen;
    tflac_u32 bufferused;
    void *tflac_mem;
    pthread_t thread;
    sample* samples;
    encoder_atomic_u32* ready;
    encoder_atomic_int* running;
    tflac_u32* frames;
};

typedef struct encoder encoder;

void * encoder_run(void * ud) {
    tflac_u32 counter = 0;
    tflac_u32 ready = 0;
    encoder* e = (encoder *) ud;

    for(;;) {

        while( (ready = encoder_atomic_u32_load(e->ready)) == counter) {
            syscall(SYS_futex, &(e->ready->val), FUTEX_WAIT, counter, 0, 0, 0);
        }
        counter = ready;

        if(encoder_atomic_int_load(e->running) == -1) break;

        if(ENCODE_FUNC(&e->t, *(e->frames), e->samples, e->buffer, e->bufferlen, &e->bufferused) != 0) abort();

        /* we were the last thread so wake up the main thread if its waiting */
        encoder_atomic_int_dec(e->running);
        syscall(SYS_futex, &(e->running->val), FUTEX_WAKE, 1, 0, 0, 0);
    }

    tflac_finalize(&e->t);

    return NULL;
}


void encoder_init(encoder* e, enum TFLAC_CHANNEL_MODE mode, sample* samples, encoder_atomic_u32* ready, encoder_atomic_int* running, tflac_u32* frames) {
    tflac_init(&e->t);

    e->t.samplerate = SAMPLERATE;
    e->t.channels = CHANNELS;
    e->t.bitdepth = BITDEPTH;
    e->t.blocksize = FRAME_SIZE;
    e->t.max_partition_order = 3;
    e->t.enable_md5 = 0;
    e->t.channel_mode = mode;

    e->tflac_mem = malloc(tflac_size_memory(e->t.blocksize));
    if(e->tflac_mem == NULL) abort();

    if(tflac_validate(&e->t, e->tflac_mem, tflac_size_memory(e->t.blocksize)) != 0) abort();

    e->bufferlen = tflac_size_frame(FRAME_SIZE,CHANNELS,BITDEPTH);
    e->buffer = malloc(e->bufferlen);
    if(e->buffer == NULL) abort();

    e->ready = ready;
    e->running = running;
    e->frames = frames;
    e->samples = samples;

    if(pthread_create(&e->thread, NULL, encoder_run, e) != 0) abort();
}

void encoder_free(encoder* e) {
    free(e->buffer);
    free(e->tflac_mem);
}

int main(int argc, const char *argv[]) {
    FILE *input = NULL;
    FILE *output = NULL;
    tflac_u32 frames = 0;
    sample *samples = NULL;
    unsigned int i = 0;
    unsigned int smallest_frame = 0;
    encoder e[TFLAC_CHANNEL_MODE_COUNT];
    encoder_atomic_u32 ready;
    encoder_atomic_int running;

    int r;

    if(argc < 3) {
        printf("Usage: %s /path/to/raw /path/to/flac\n",argv[0]);
        return 1;
    }

    tflac_detect_cpu();

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

    samples = (sample *)malloc(sizeof(sample) * CHANNELS * FRAME_SIZE);
    if(!samples) abort();

    encoder_atomic_u32_store(&ready,0);
    encoder_atomic_int_store(&running,0);

    for(i=0;i<TFLAC_CHANNEL_MODE_COUNT;i++) {
        encoder_init(&e[i],(enum TFLAC_CHANNEL_MODE)i,samples, &ready, &running, &frames);
    }

    /* enable MD5 on the first encoder */
    e[0].t.enable_md5 = 1;

    fwrite("fLaC",1,4,output);

    /* we'll write out an empty STREAMINFO block and overwrite later to get MD5 checksum
     * and sample count */
    tflac_encode_streaminfo(&e[0].t, 0, e[0].buffer, e[0].bufferlen, &e[0].bufferused);
    fwrite(e[0].buffer,1,e[0].bufferused,output);

    while((frames = fread(samples,sizeof(sample) * CHANNELS, FRAME_SIZE, input)) > 0) {
        repack_samples(samples, CHANNELS, frames);

        encoder_atomic_int_store(&running,TFLAC_CHANNEL_MODE_COUNT);
        encoder_atomic_u32_inc(&ready);
        syscall(SYS_futex, &(e->ready->val), FUTEX_WAKE, INT_MAX, 0, 0, 0);
        while( (r = encoder_atomic_int_load(&running)) != 0) {
            syscall(SYS_futex, &(e->ready->val), FUTEX_WAKE, INT_MAX, 0, 0, 0);
            syscall(SYS_futex, &(running.val), FUTEX_WAIT, r, 0, 0, 0);
        }

        smallest_frame = 0;
        for(i=1;i<TFLAC_CHANNEL_MODE_COUNT;i++) {
            if(e[i].bufferused < e[smallest_frame].bufferused) {
                smallest_frame = i;
            }
        }

        fwrite(e[smallest_frame].buffer,1,e[smallest_frame].bufferused,output);

        /* bookkeeping: update our min/max sizes in the first encoder */
        if(e[0].t.min_frame_size > e[smallest_frame].bufferused) {
            e[0].t.min_frame_size = e[smallest_frame].bufferused;
        }
        if(e[0].t.max_frame_size < e[smallest_frame].bufferused) {
            e[0].t.max_frame_size = e[smallest_frame].bufferused;
        }
    }

    encoder_atomic_int_store(&running,-1);
    encoder_atomic_u32_inc(&ready);
    syscall(SYS_futex, &(e->ready->val), FUTEX_WAKE, INT_MAX, 0, 0, 0);

    for(i=0;i<TFLAC_CHANNEL_MODE_COUNT;i++) {
        pthread_join(e[i].thread,NULL);
    }


    /* now we overwrite our original STREAMINFO with an updated one */
    fseek(output,4,SEEK_SET);
    tflac_encode_streaminfo(&e[0].t, 1, e[0].buffer, e[0].bufferlen, &e[0].bufferused);
    fwrite(e[0].buffer,1,e[0].bufferused,output);

    fclose(input);
    fclose(output);
    free(samples);

    for(i=0;i<TFLAC_CHANNEL_MODE_COUNT;i++) {
        encoder_free(&e[i]);
    }

    return 0;
}

