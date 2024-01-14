#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>

static void pack_u16le(uint8_t* d, uint16_t n) {
    d[0] = ( n       ) & 0xFF;
    d[1] = ( n >> 8  ) & 0xFF;
}

static void pack_u24le(uint8_t* d, uint32_t n) {
    d[0] = ( n       ) & 0xFF;
    d[1] = ( n >> 8  ) & 0xFF;
    d[2] = ( n >> 16 ) & 0xFF;
}

static void pack_u32be(uint8_t* d, uint32_t n) {
    d[0] = ( n >> 24 ) & 0xFF;
    d[1] = ( n >> 16 ) & 0xFF;
    d[2] = ( n >> 8  ) & 0xFF;
    d[3] = ( n       ) & 0xFF;
}

static void pack_u32le(uint8_t* d, uint32_t n) {
    d[0] = ( n       ) & 0xFF;
    d[1] = ( n >> 8  ) & 0xFF;
    d[2] = ( n >> 16 ) & 0xFF;
    d[3] = ( n >> 24 ) & 0xFF;
}

static void pack_s16le(uint8_t* d, int16_t n) {
    pack_u16le(d, (uint16_t) n);
}

static void pack_s32be(uint8_t* d, int32_t n) {
    pack_u32be(d, (uint32_t) n);
}

static void pack_s24le(uint8_t* d, int32_t n) {
    pack_u24le(d, (uint32_t) n);
}

static void pack_s32le(uint8_t* d, int32_t n) {
    pack_u32le(d, (uint32_t) n);
}

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

static int32_t unpack_s32be(const uint8_t* d) {
    return (int32_t) unpack_u32be(d);
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

static int read_s32be(FILE* f, int32_t* val) {
    uint8_t buffer[4];
    if(fread(buffer,1,4,f) != 4) return 1;
    *val = unpack_s32be(buffer);
    return 0;
}

static int read_u32be(FILE* f, uint32_t* val) {
    uint8_t buffer[4];
    if(fread(buffer,1,4,f) != 4) return 1;
    *val = unpack_u32be(buffer);
    return 0;
}

static int write_u16le(FILE* f, uint16_t val) {
    uint8_t buffer[2];
    pack_u16le(buffer, val);
    if(fwrite(buffer,1,2,f) != 2) return 1;
    return 0;
}

static int write_s16le(FILE* f, int16_t val) {
    return write_u16le(f, (uint16_t) val);
}

static int write_u32be(FILE* f, uint32_t val) {
    uint8_t buffer[4];
    pack_u32be(buffer, val);
    if(fwrite(buffer,1,4,f) != 4) return 1;
    return 0;
}

static int write_u24le(FILE* f, uint32_t val) {
    uint8_t buffer[3];
    pack_u24le(buffer, val);
    if(fwrite(buffer,1,3,f) != 3) return 1;
    return 0;
}

static int write_u32le(FILE* f, uint32_t val) {
    uint8_t buffer[4];
    pack_u32le(buffer, val);
    if(fwrite(buffer,1,4,f) != 4) return 1;
    return 0;
}

static int write_s32be(FILE* f, int32_t val) {
    return write_u32be(f, (uint32_t) val);
}

static int write_s24le(FILE* f, int32_t val) {
    return write_u24le(f, (uint32_t) val);
}

static int write_s32le(FILE* f, int32_t val) {
    return write_u32le(f, (uint32_t) val);
}

static int readsample_32(FILE* f, int32_t* val) {
    return read_s32le(f, val);
}

static int readsample_24(FILE* f, int32_t* val) {
    return read_s24le(f, val);
}

static int readsample_16(FILE* f, int32_t* val) {
    int16_t tmp;
    int r;

    if( (r = read_s16le(f, &tmp)) != 0) return r;
    *val = (int32_t)tmp;
    return 0;
}

static int readsample_8(FILE* f, int32_t* val) {
    uint8_t v;
    int32_t t;
    if( fread(&v,1,1,f) != 1) return 1;
    t = v;
    t -= 128;
    *val = t;
    return 0;
}

static int writesample_8(FILE* f, int32_t val) {
    uint8_t b;
    val += 128;
    b = (uint8_t)val;
    if( fwrite(&b,1,1,f) != 1) return 1;
    return 0;
}

static int writesample_16(FILE* f, int32_t val) {
    return write_s16le(f, (int16_t)val);
}

static int writesample_24(FILE* f, int32_t val) {
    return write_s24le(f, val);
}

static int writesample_32(FILE* f, int32_t val) {
    return write_s32le(f, val);
}

#define CHUNK_ID_RIFF 0x52494646
#define CHUNK_ID_WAVE 0x57415645
#define CHUNK_ID_FMT  0x666d7420
#define CHUNK_ID_DATA 0x64617461

#define FORMAT_TAG_PCM        0x0001
#define FORMAT_TAG_EXTENSIBLE 0xfffe

#define TRY(x) if( (x) != 0 ) goto complete

int main(int argc, const char *argv[]) {
    int r = 1;
    int i = 0;
    uint8_t buffer[4] = { 0, 0, 0, 0 };
    int16_t  temp_s16;
    uint16_t temp_u16;
    int32_t  temp_s32;
    int32_t isample;
    int32_t osample;
    uint32_t temp_u32;
    uint16_t formattag = 0;
    uint16_t channels = 0;
    uint32_t samplerate = 0;
    uint16_t bitdepth = 0;
    uint16_t wastedbits = 0;
    uint16_t downshift = 0;
    uint16_t upshift = 0;
    uint32_t channelmask = 0;
    uint16_t blockalign = 0;
    uint32_t chunk_id = 0;
    uint32_t chunk_len = 0;
    uint32_t guid0 = 0;
    uint32_t guid1 = 0;
    uint32_t guid2 = 0;
    uint32_t guid3 = 0;
    uint16_t outdepth = 5;
    uint32_t bytespersample = 0;
    FILE* input = NULL;
    FILE* output = NULL;
    int (*readsample)(FILE *, int32_t* val) = NULL;
    int (*writesample)(FILE *, int32_t val) = NULL;
    const char *progname = NULL;
    const char *inname = NULL;
    const char *outname = NULL;

    progname = *argv++;
    argc--;

    while(argc--) {
        /* this is a very stupid way to do this */
        if(strcmp(*argv,"-4") == 0) {
            outdepth = 4;
        } else if(strcmp(*argv,"-5") == 0) {
            outdepth = 5;
        } else if(strcmp(*argv,"-6") == 0) {
            outdepth = 6;
        } else if(strcmp(*argv,"-7") == 0) {
            outdepth = 7;
        } else if(strcmp(*argv,"-8") == 0) {
            outdepth = 8;
        } else if(strcmp(*argv,"-9") == 0) {
            outdepth = 9;
        } else if(strcmp(*argv,"-10") == 0) {
            outdepth = 10;
        } else if(strcmp(*argv,"-11") == 0) {
            outdepth = 11;
        } else if(strcmp(*argv,"-12") == 0) {
            outdepth = 12;
        } else if(strcmp(*argv,"-13") == 0) {
            outdepth = 13;
        } else if(strcmp(*argv,"-14") == 0) {
            outdepth = 14;
        } else if(strcmp(*argv,"-15") == 0) {
            outdepth = 15;
        } else if(strcmp(*argv,"-16") == 0) {
            outdepth = 16;
        } else if(strcmp(*argv,"-17") == 0) {
            outdepth = 17;
        } else if(strcmp(*argv,"-18") == 0) {
            outdepth = 18;
        } else if(strcmp(*argv,"-19") == 0) {
            outdepth = 19;
        } else if(strcmp(*argv,"-20") == 0) {
            outdepth = 20;
        } else if(strcmp(*argv,"-21") == 0) {
            outdepth = 21;
        } else if(strcmp(*argv,"-22") == 0) {
            outdepth = 22;
        } else if(strcmp(*argv,"-23") == 0) {
            outdepth = 23;
        } else if(strcmp(*argv,"-24") == 0) {
            outdepth = 24;
        } else if(strcmp(*argv,"-25") == 0) {
            outdepth = 25;
        } else if(strcmp(*argv,"-26") == 0) {
            outdepth = 26;
        } else if(strcmp(*argv,"-27") == 0) {
            outdepth = 27;
        } else if(strcmp(*argv,"-28") == 0) {
            outdepth = 28;
        } else if(strcmp(*argv,"-29") == 0) {
            outdepth = 29;
        } else if(strcmp(*argv,"-30") == 0) {
            outdepth = 30;
        } else if(strcmp(*argv,"-31") == 0) {
            outdepth = 31;
        } else if(strcmp(*argv,"-32") == 0) {
            outdepth = 32;
        } else if(argv[0][0] == '-' && isdigit(argv[0][1]) ) {
            printf("Unsupported bit depth option %s, valid options are -4 through -15\n",
              *argv);
            goto complete;
        }
        else if(inname == NULL) {
            inname = *argv;
        } else if(outname == NULL) {
            outname = *argv;
        }
        argv++;
    }

    if(inname == NULL || outname == NULL) {
        printf("Usage: %s /path/to/input /path/to/output\n", progname);
        goto complete;
    }

    input = fopen(inname,"rb");
    if(input == NULL) goto complete;
    output = fopen(outname,"wb");
    if(output == NULL) goto complete;

    TRY( read_u32be(input, &chunk_id) );
    if(chunk_id != CHUNK_ID_RIFF) {
        printf("This tool only works with RIFF files\n");
        goto complete;
    }

    /* skip file chunk size */
    TRY( read_u32be(input, &temp_u32) );

    /* check the format is WAVE */
    TRY( read_u32be(input, &chunk_id) );
    if(chunk_id != CHUNK_ID_WAVE) {
        printf("This tool only works with WAVE files\n");
        goto complete;
    }

    /* find the fmt chunk */
    TRY( read_u32be(input, &chunk_id) );
    while(chunk_id != CHUNK_ID_FMT) {
        TRY( read_u32le(input, &chunk_len) );
        TRY( fseek(input,chunk_len,SEEK_CUR) );

        TRY( read_u32be(input, &chunk_id) );
    }
    TRY( read_u32le(input, &chunk_len) );

    TRY( read_u16le(input, &formattag) );
    if( ! (formattag == FORMAT_TAG_PCM || formattag == FORMAT_TAG_EXTENSIBLE) ) {
        printf("This tool only works with PCM and EXTENSIBLE WAVE files, formattag=0x%04x\n",formattag);
        goto complete;
    }

    TRY( read_u16le(input, &channels) );
    TRY( read_u32le(input, &samplerate) );
    TRY( read_u32le(input, &temp_u32) ); /* average bytes per second, ignore */
    TRY( read_u16le(input, &blockalign) );
    TRY( read_u16le(input, &bitdepth) );
    if(bitdepth % 8 != 0) {
        printf("weird bitdepth encountered!\n");
        goto complete;
    }
    if(formattag == FORMAT_TAG_EXTENSIBLE) {
        TRY( read_u16le(input, &temp_u16) );
        if(temp_u16 != 22) {
            printf("Unknown cbSize found - %u, expected 22\n", temp_u16);
            goto complete;
        }
        TRY( read_u16le(input, &temp_u16) );
        wastedbits = bitdepth - temp_u16;

        TRY( read_u32le(input, &channelmask) );

        TRY( read_u32le(input, &guid0) );
        TRY( read_u32le(input, &guid1) );
        TRY( read_u32le(input, &guid2) );
        TRY( read_u32le(input, &guid3) );

        if(guid0 != 0x00000001  ||
           guid1 != 0x00100000  ||
           guid2 != 0xaa000080  ||
           guid3 != 0x719b3800) {
            printf("Unknown subformat GUID found\n");
            goto complete;
        }
    } else {
        switch(channels) {
            case 1: channelmask = 0x04; break;
            case 2: channelmask = 0x03; break;
            default: {
                printf("For non-extensible waves this tool only handles 1 or 2 channels\n");
                goto complete;
            }
        }
    }

    if(bitdepth - wastedbits < outdepth) {
        printf("Error converting from %u-bit to %u-bit,"
               " source bit-depth must be greater than dest bit-depth!", bitdepth - wastedbits, outdepth);
        goto complete;
    }

    /* find the data chunk */
    TRY( read_u32be(input, &chunk_id) );
    while(chunk_id != CHUNK_ID_DATA) {
        TRY( read_u32le(input, &chunk_len) );
        TRY( fseek(input,chunk_len,SEEK_CUR) );

        TRY( read_u32be(input, &chunk_id) );
    }
    TRY( read_u32le(input, &chunk_len) );

    switch(bitdepth) {
        case  8: readsample = readsample_8;  downshift = 8  - wastedbits - outdepth; break;
        case 16: readsample = readsample_16; downshift = 16 - wastedbits - outdepth; break;
        case 24: readsample = readsample_24; downshift = 24 - wastedbits - outdepth; break;
        case 32: readsample = readsample_32; downshift = 32 - wastedbits - outdepth; break;
        default: {
            printf("unsupported bitdepth: %u\n",bitdepth);
            goto complete;
        }
    }

    bytespersample = (uint32_t)((outdepth + 7) & 0xFFF8) / 8;

    switch(bytespersample) {
        case 1: writesample = writesample_8;   upshift = 8 - outdepth; break;
        case 2: writesample = writesample_16; upshift = 16 - outdepth; break;
        case 3: writesample = writesample_24; upshift = 24 - outdepth; break;
        case 4: writesample = writesample_32; upshift = 32 - outdepth; break;
        default: {
            printf("this shouldn't happen\n");
            goto complete;
        }
    }

    printf("Source info:\n");
    printf("  bit depth: %u\n", bitdepth);
    printf("Destination info:\n");
    printf("  bit depth: %u\n", outdepth);
    printf("  bytes per sample: %u\n", bytespersample);
    printf("Downshift: %u\n", downshift);
    printf("Upshift: %u\n", upshift);

    /* write out a WAVE header */
    TRY( write_u32be(output, CHUNK_ID_RIFF) );
    TRY( write_u32le(output, 0 ) ); /* file length to fill in later */
    TRY( write_u32be(output, CHUNK_ID_WAVE) );
    TRY( write_u32be(output, CHUNK_ID_FMT) );
    TRY( write_u32le(output, 40) );
    TRY( write_u16le(output, FORMAT_TAG_EXTENSIBLE) );
    TRY( write_u16le(output, channels) );
    TRY( write_u32le(output, samplerate) );
    TRY( write_u32le(output, samplerate * ((uint32_t)channels) * bytespersample ) ); /* bytes per second */
    TRY( write_u16le(output, channels * bytespersample ) ); /* block alignment */
    TRY( write_u16le(output, bytespersample * 8) );
    TRY( write_u16le(output, 22) );
    TRY( write_u16le(output, outdepth) );
    TRY( write_u32le(output, channelmask) );
    TRY( write_u32le(output, 0x00000001) );
    TRY( write_u32le(output, 0x00100000) );
    TRY( write_u32le(output, 0xaa000080) );
    TRY( write_u32le(output, 0x719b3800) );

    TRY( write_u32be(output, CHUNK_ID_DATA) );
    TRY( write_u32le(output, 0 ) ); /* chunk length to fill in later */

    chunk_len /= (bitdepth/8); /* convert to number of samples */
    while(chunk_len--) {
        TRY( readsample(input, &isample) );
        osample = isample;
        osample /= (1 << wastedbits);
        osample /= (1 << downshift);
        osample *= (1 << upshift);
        if(wastedbits == 0 &&
           downshift == 0 &&
           upshift == 0) {
            assert(isample == osample);
        }

        TRY( writesample(output, osample) );
    }

    temp_u32 = ftell(output);
    fseek(output, 4, SEEK_SET);
    TRY( write_u32le(output, temp_u32 - 8 ) ); /* file length */
    fseek(output, 64, SEEK_SET);
    TRY( write_u32le(output, temp_u32 - 68 ) ); /* file length */

    r = 0;

    complete:
    if(input != NULL) fclose(input);
    if(output != NULL) fclose(output);

    return r;
}
