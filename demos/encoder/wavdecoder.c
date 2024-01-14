#include "wavdecoder.h"

#define CHUNK_ID_RIFF 0x52494646
#define CHUNK_ID_WAVE 0x57415645
#define CHUNK_ID_FMT  0x666d7420
#define CHUNK_ID_DATA 0x64617461

#define FORMAT_TAG_PCM        0x0001
#define FORMAT_TAG_EXTENSIBLE 0xfffe

static uint16_t unpack_u16le(const uint8_t* d) {
    return (((uint16_t)d[0])    ) |
           (((uint16_t)d[1])<< 8);
}

static uint32_t unpack_u32be(const uint8_t* d) {
    return (((uint32_t)d[0])<<24) |
           (((uint32_t)d[1])<<16) |
           (((uint32_t)d[2])<<8 ) |
           (((uint32_t)d[3])    );
}

static uint32_t unpack_u24le(const uint8_t* d) {
    return (((uint32_t)d[0])    ) |
           (((uint32_t)d[1])<< 8) |
           (((uint32_t)d[2])<<16);
}

static int32_t unpack_s24le(const uint8_t* d) {
    uint32_t val = unpack_u24le(d);
    int32_t r;
    struct {
        signed int x:24;
    } s;
    r = s.x = val;
    return r;

}

static uint32_t unpack_u32le(const uint8_t* d) {
    return (((uint32_t)d[0])    ) |
           (((uint32_t)d[1])<< 8) |
           (((uint32_t)d[2])<<16) |
           (((uint32_t)d[3])<<24);
}

static int32_t unpack_s32le(const uint8_t* d) {
    return (int32_t) unpack_u32le(d);
}

static int16_t unpack_s16le(const uint8_t* d) {
    return (int16_t) unpack_u16le(d);
}

static int read_s16le(FILE* f, int16_t* val) {
    uint8_t buffer[2];
    if(fread(buffer,1,2,f) != 2) return 1;
    *val = unpack_s16le(buffer);
    return 0;
}

static int read_u16le(FILE* f, uint16_t* val) {
    uint8_t buffer[2];
    if(fread(buffer,1,2,f) != 2) return 1;
    *val = unpack_u16le(buffer);
    return 0;
}

static int read_s24le(FILE* f, int32_t* val) {
    uint8_t buffer[3];
    if(fread(buffer,1,3,f) != 3) return 1;
    *val = unpack_s24le(buffer);
    return 0;
}

static int read_s32le(FILE* f, int32_t* val) {
    uint8_t buffer[4];
    if(fread(buffer,1,4,f) != 4) return 1;
    *val = unpack_s32le(buffer);
    return 0;
}

static int read_u32le(FILE* f, uint32_t* val) {
    uint8_t buffer[4];
    if(fread(buffer,1,4,f) != 4) return 1;
    *val = unpack_u32le(buffer);
    return 0;
}

static int read_u32be(FILE* f, uint32_t* val) {
    uint8_t buffer[4];
    if(fread(buffer,1,4,f) != 4) return 1;
    *val = unpack_u32be(buffer);
    return 0;
}

static int readsample_8(wav_decoder* w, int32_t* val) {
    uint8_t v;
    int32_t t;
    if( fread(&v,1,1,w->input) != 1) return 1;
    t = (int32_t)v;
    t -= 128;
    *val = t;
    return 0;
}

static int readsample_16(wav_decoder* w, int32_t* val) {
    int16_t tmp;
    int r;

    if( (r = read_s16le(w->input, &tmp)) != 0) return r;
    *val = (int32_t)tmp;
    return 0;
}

static int readsample_24(wav_decoder* w, int32_t* val) {
    return read_s24le(w->input, val);
}

static int readsample_32(wav_decoder* w, int32_t* val) {
    return read_s32le(w->input, val);
}

#define TRY(x) if( (r = (x)) != 0) return r
#define CHECK(x,msg) if(!(x)) { fprintf(stderr,msg"\n"); return 1; }

int wav_decoder_open(wav_decoder *w, FILE* f) {
    int r = 1;

    uint16_t tmp_u16;
    uint32_t tmp_u32;
    uint32_t chunk_id;
    uint32_t chunk_len;
    uint16_t formattag;
    uint32_t guid0 = 0;
    uint32_t guid1 = 0;
    uint32_t guid2 = 0;
    uint32_t guid3 = 0;
    uint32_t samplesize = 0;

    w->input = f;
    w->readsample = NULL;
    w->length = 0;
    w->channels = 0;
    w->wasted_bits = 0;

    TRY( read_u32be(w->input, &chunk_id) );
    CHECK(chunk_id == CHUNK_ID_RIFF,"Input file is not RIFF");

    TRY( read_u32be(w->input, &tmp_u32) );
    CHECK(tmp_u32 != 0xFFFFFFFF, "RIFF chunk size set to -1");

    TRY( read_u32be(w->input, &chunk_id) );
    CHECK(chunk_id == CHUNK_ID_WAVE,"Input file is not WAVE");

    TRY( read_u32be(w->input, &chunk_id) );
    while(chunk_id != CHUNK_ID_FMT) {
        TRY( read_u32le(w->input, &chunk_len) );
        TRY( fseek(w->input,chunk_len,SEEK_CUR) );
        TRY( read_u32be(w->input, &chunk_id) );
    }
    TRY( read_u32le(w->input, &chunk_len) );

    TRY( read_u16le(w->input, &formattag) );
    CHECK(formattag == FORMAT_TAG_PCM || formattag == FORMAT_TAG_EXTENSIBLE,"WAVE not in compatible format");

    TRY( read_u16le(w->input, &w->channels) );
    TRY( read_u32le(w->input, &w->samplerate) );
    TRY( read_u32le(w->input, &tmp_u32) ); /* average bytes per second, ignore */
    TRY( read_u16le(w->input, &tmp_u16) );
    TRY( read_u16le(w->input, &w->bitdepth) );
    CHECK(w->bitdepth % 8 == 0, "WAVE file has bitdepth that isn't divisible by 8");
    CHECK(w->bitdepth * w->channels / 8 == tmp_u16, "WAVE file has unexpected block alignment");
    samplesize = w->bitdepth / 8;

    if(formattag == FORMAT_TAG_EXTENSIBLE) {
        TRY( read_u16le(w->input, &tmp_u16) );
        CHECK(tmp_u16 == 22, "WAVE file has FORMAT_TAG_EXTENSIBLE but extensible data length is not 22");
        TRY( read_u16le(w->input, &tmp_u16) );
        w->wasted_bits = w->bitdepth - tmp_u16;
        w->bitdepth -= w->wasted_bits;

        TRY( read_u32le(w->input, &w->channelmask) );

        TRY( read_u32le(w->input, &guid0) );
        TRY( read_u32le(w->input, &guid1) );
        TRY( read_u32le(w->input, &guid2) );
        TRY( read_u32le(w->input, &guid3) );

        CHECK(guid0 == 0x00000001  &&
              guid1 == 0x00100000  &&
              guid2 == 0xaa000080  &&
              guid3 == 0x719b3800, "Unknown subformat GUID found");
    } else {
        switch(w->channels) {
            case 1: w->channelmask = 0x04; break;
            case 2: w->channelmask = 0x03; break;
            default: {
                fprintf(stderr,"For non-extensible waves this tool only handles 1 or 2 channels\n");
                return 1;
            }
        }
    }

    TRY( read_u32be(w->input, &chunk_id) );
    while(chunk_id != CHUNK_ID_DATA) {
        TRY( read_u32le(w->input, &chunk_len) );
        TRY( fseek(w->input,chunk_len,SEEK_CUR) );

        TRY( read_u32be(w->input, &chunk_id) );
    }
    TRY( read_u32le(w->input, &chunk_len) );

    /* we're ready to read samples */
    w->length = chunk_len / samplesize / w->channels;
    switch(samplesize) {
        case 1: w->readsample = readsample_8; break;
        case 2: w->readsample = readsample_16; break;
        case 3: w->readsample = readsample_24; break;
        case 4: w->readsample = readsample_32; break;
        default: {
            fprintf(stderr,"Unknown sample size\n");
            return 1;
        }
    }

    return r;
}


int wav_decoder_decode(wav_decoder* w, int32_t* buffer, uint32_t len, uint32_t* r) {
    uint32_t i = 0;
    if(w->length == 0) return 1;

    len = len > w->length ? w->length : len;

    for(i=0;i<len * ((uint32_t)w->channels);i++) {
        if(w->readsample(w,&buffer[i]) != 0) return -1;
        buffer[i] /= ( 1 << w->wasted_bits) ;
    }

    w->length -= len;
    *r = len;
    return 0;
}
