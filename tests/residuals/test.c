#define TFLAC_IMPLEMENTATION
#include "tflac.h"

#include <stdio.h>
#include <stdlib.h>

/* makes sure the residual-calculating functions get expected results */

#define BLOCKSIZE 16


static tflac_s32* samples_unaligned = NULL;
static tflac_s32* residuals_unaligned = NULL;

static tflac_s32* samples = NULL;
static tflac_s32* residuals = NULL;
static tflac_u64 result;

#ifdef TFLAC_32BIT_ONLY
static void print_result(void) {
    printf("%u",result.lo);
}
#else
static void print_result(void) {
    printf("%lu",result);
}
#endif

static const tflac_s32 residuals_order1[] = {
  11056,
  8986,
  -12937,
  -16518,
  -17099,
  9990,
  5727,
  14423,
  -30911,
  37530,
  -28880,
  20186,
  10334,
  -26912,
  -368,
  24809,
};

static const tflac_s32 residuals_order2[] = {
  11056,
  20042,
  -21923,
  -3581,
  -581,
  27089,
  -4263,
  8696,
  -45334,
  68441,
  -66410,
  49066,
  -9852,
  -37246,
  26544,
  25177,
};

static const tflac_s32 residuals_order3[] = {
  11056,
  20042,
  7105,
  18342,
  3000,
  27670,
  -31352,
  12959,
  -54030,
  113775,
  -134851,
  115476,
  -58918,
  -27394,
  63790,
  -1367,
};

static const tflac_s32 residuals_order4[] = {
  11056,
  20042,
  7105,
  -9413,
  -15342,
  24670,
  -59022,
  44311,
  -66989,
  167805,
  -248626,
  250327,
  -174394,
  31524,
  91184,
  -65157,
};

static const char * const passfail[] = {
    "pass",
    "fail",
};

static void* align_ptr(void* ptr) {
    tflac_u8 *d;
    tflac_uptr p1, p2;

    p1 = (tflac_uptr)ptr;
    p2 = (p1 + 15) & ~(tflac_uptr)0x0F;
    d = (tflac_u8*)ptr;

    return &d[p2 - p1];
}

static void test_reset(void) {
    tflac_u32 i = 0;
    for(i=0;i<BLOCKSIZE;i++) {
        samples[i] = 0;
        residuals[i] = 0;
    }
    result = TFLAC_U64_ZERO;
}

/* load up some previously-generated int16_t values */
static void test_set_samples(void) {
    samples[0] = 11056;
    samples[1] = 20042;
    samples[2] = 7105;
    samples[3] = -9413;
    samples[4] = -26512;
    samples[5] = -16522;
    samples[6] = -10795;
    samples[7] = 3628;
    samples[8] = -27283;
    samples[9] = 10247;
    samples[10] = -18633;
    samples[11] = 1553;
    samples[12] = 11887;
    samples[13] = -15025;
    samples[14] = -15393;
    samples[15] = 9416;
}

static int test_order0_results(void) {
    int r = 0;
    int t;

    t = !TFLAC_U64_EQ_WORD(result, 166894);
    if(t) {
        printf("  order 0 error: expected 166894 got ");
        print_result();
        printf("\n");
    }
    r |= t;

    return r;
}

static int test_order1_results(void) {
    int r = 0;
    int t;
    unsigned int i = 0;

    for(i=0;i<BLOCKSIZE;i++) {
        t = !(residuals[i] == residuals_order1[i]);
        if(t) printf("  order 1 residual %u: expected %d got %d\n",i,residuals_order1[i], residuals[i]);
        r |= t;
    }
    t = !TFLAC_U64_EQ_WORD(result, 227169);
    if(t) {
        printf("  order 1 error: expected 227169 got ");
        print_result();
        printf("\n");
    }
    r |= t;

    return r;
}

static int test_order2_results(void) {
    int r = 0;
    int t;
    unsigned int i = 0;

    for(i=0;i<BLOCKSIZE;i++) {
        t = !(residuals[i] == residuals_order2[i]);
        if(t) printf("  order 2 residual %u: expected %d got %d\n",i,residuals_order2[i], residuals[i]);
        r |= t;
    }
    t = !TFLAC_U64_EQ_WORD(result, 368699);
    if(t) {
        printf("  order 2 error: expected 368699 got ");
        print_result();
        printf("\n");
    }
    r |= t;

    return r;
}

static int test_order3_results(void) {
    int r = 0;
    int t;
    unsigned int i = 0;

    for(i=0;i<BLOCKSIZE;i++) {
        t = !(residuals[i] == residuals_order3[i]);
        if(t) printf("  order 3 residual %u: expected %d got %d\n",i,residuals_order3[i], residuals[i]);
        r |= t;
    }
    t = !TFLAC_U64_EQ_WORD(result, 644582);
    if(t) {
        printf("  order 3 error: expected 644582 got ");
        print_result();
        printf("\n");
    }
    r |= t;

    return r;
}


static int test_order4_results(void) {
    int r = 0;
    int t;
    unsigned int i = 0;

    for(i=0;i<BLOCKSIZE;i++) {
        t = !(residuals[i] == residuals_order4[i]);
        if(t) printf("  order 4 residual %u: expected %d got %d\n",i,residuals_order4[i], residuals[i]);
        r |= t;
    }
    t = !TFLAC_U64_EQ_WORD(result, 1239351);
    if(t) {
        printf("  order 4 error: expected 1239351 got ");
        print_result();
        printf("\n");
    }
    r |= t;

    return r;
}


static int test_order1_wide_std_zero(void) {
    int r;

    test_reset();

    tflac_cfr_order1_wide_std(BLOCKSIZE,samples,residuals,&result);

    r = !TFLAC_U64_EQ(result,TFLAC_U64_ZERO);

    printf("test_order1_wide_std_zero: %s\n",passfail[r]);
    return r;
}

static int test_order2_wide_std_zero(void) {
    int r;

    test_reset();

    tflac_cfr_order2_wide_std(BLOCKSIZE,samples,residuals,&result);

    r = !TFLAC_U64_EQ(result,TFLAC_U64_ZERO);

    printf("test_order2_wide_std_zero: %s\n",passfail[r]);
    return r;
}

static int test_order3_wide_std_zero(void) {
    int r;

    test_reset();

    tflac_cfr_order3_wide_std(BLOCKSIZE,samples,residuals,&result);

    r = !TFLAC_U64_EQ(result,TFLAC_U64_ZERO);

    printf("test_order3_wide_std_zero: %s\n",passfail[r]);
    return r;
}

static int test_order4_wide_std_zero(void) {
    int r;

    test_reset();

    tflac_cfr_order4_wide_std(BLOCKSIZE,samples,residuals,&result);

    r = !TFLAC_U64_EQ(result,TFLAC_U64_ZERO);

    printf("test_order4_wide_std_zero: %s\n",passfail[r]);
    return r;
}

static int test_order1_wide_std_max(void) {
    int r;

    test_reset();

    samples[0] = INT32_MIN;
    samples[1] = INT32_MAX;

    tflac_cfr_order1_wide_std(BLOCKSIZE,samples,residuals,&result);

    r = !TFLAC_U64_EQ(result,TFLAC_U64_MAX);

    printf("test_order1_wide_std_max: %s\n",passfail[r]);
    return r;
}

static int test_order2_wide_std_max(void) {
    int r;

    test_reset();

    samples[0] = INT32_MIN;
    samples[1] = INT32_MIN;
    samples[2] = INT32_MAX;

    tflac_cfr_order2_wide_std(BLOCKSIZE,samples,residuals,&result);

    r = !TFLAC_U64_EQ(result,TFLAC_U64_MAX);

    printf("test_order2_wide_std_max: %s\n",passfail[r]);
    return r;
}

static int test_order3_wide_std_max(void) {
    int r;

    test_reset();

    samples[0] = INT32_MIN;
    samples[1] = INT32_MAX;
    samples[2] = INT32_MIN;
    samples[3] = INT32_MAX;

    tflac_cfr_order3_wide_std(BLOCKSIZE,samples,residuals,&result);

    r = !TFLAC_U64_EQ(result,TFLAC_U64_MAX);

    printf("test_order3_wide_std_max: %s\n",passfail[r]);
    return r;
}

static int test_order4_wide_std_max(void) {
    int r;

    test_reset();

    samples[0] = INT32_MAX;
    samples[1] = INT32_MIN;
    samples[2] = INT32_MAX;
    samples[3] = INT32_MIN;
    samples[4] = INT32_MAX;

    tflac_cfr_order4_wide_std(BLOCKSIZE,samples,residuals,&result);

    r = !TFLAC_U64_EQ(result,TFLAC_U64_MAX);

    printf("test_order4_wide_std_max: %s\n",passfail[r]);
    return r;
}

#define STANDARD_TEST(n,v) test_order ## n ## _ ## v

#define STANDARD_TEST_DEF(n,v) \
static int test_order ## n ## _ ## v (void) { \
    int r; \
    test_reset(); \
    test_set_samples(); \
    tflac_cfr_order ## n ## _ ## v (BLOCKSIZE,samples,residuals,&result); \
    printf("test_order%u_%s:\n",n, #v); \
    r = test_order ## n ## _results(); \
    printf("  %s\n", passfail[r]); \
    return r; \
}

STANDARD_TEST_DEF(0,std)
STANDARD_TEST_DEF(1,std)
STANDARD_TEST_DEF(2,std)
STANDARD_TEST_DEF(3,std)
STANDARD_TEST_DEF(4,std)

STANDARD_TEST_DEF(1,wide_std)
STANDARD_TEST_DEF(2,wide_std)
STANDARD_TEST_DEF(3,wide_std)
STANDARD_TEST_DEF(4,wide_std)

#ifdef TFLAC_ENABLE_SSE2
STANDARD_TEST_DEF(0,sse2)
STANDARD_TEST_DEF(1,sse2)
STANDARD_TEST_DEF(2,sse2)
STANDARD_TEST_DEF(3,sse2)
STANDARD_TEST_DEF(4,sse2)
#endif

#ifdef TFLAC_ENABLE_SSSE3
STANDARD_TEST_DEF(0,ssse3)
STANDARD_TEST_DEF(1,ssse3)
STANDARD_TEST_DEF(2,ssse3)
STANDARD_TEST_DEF(3,ssse3)
STANDARD_TEST_DEF(4,ssse3)
#endif

#ifdef TFLAC_ENABLE_SSE4_1
STANDARD_TEST_DEF(0,sse4_1)
STANDARD_TEST_DEF(1,sse4_1)
STANDARD_TEST_DEF(2,sse4_1)
STANDARD_TEST_DEF(3,sse4_1)
STANDARD_TEST_DEF(4,sse4_1)
#endif

int main(void) {
    int r = 0;
    samples_unaligned = (tflac_s32*)malloc(15 + (sizeof(tflac_s32) * 16));
    if(samples_unaligned == NULL) abort();
    residuals_unaligned = (tflac_s32*)malloc(15 + (sizeof(tflac_s32) * 16));
    if(residuals_unaligned == NULL) abort();

    printf("sizeof(tflac_uint): %u\n",(tflac_u32)sizeof(tflac_uint));

    samples = align_ptr(samples_unaligned);
    residuals = align_ptr(residuals_unaligned);

    r |= test_order1_wide_std_zero();
    r |= test_order2_wide_std_zero();
    r |= test_order3_wide_std_zero();
    r |= test_order4_wide_std_zero();

    r |= test_order1_wide_std_max();
    r |= test_order2_wide_std_max();
    r |= test_order3_wide_std_max();
    r |= test_order4_wide_std_max();

    r |= STANDARD_TEST(0,std)();
    r |= STANDARD_TEST(1,std)();
    r |= STANDARD_TEST(2,std)();
    r |= STANDARD_TEST(3,std)();
    r |= STANDARD_TEST(4,std)();

    r |= STANDARD_TEST(1,wide_std)();
    r |= STANDARD_TEST(2,wide_std)();
    r |= STANDARD_TEST(3,wide_std)();
    r |= STANDARD_TEST(4,wide_std)();

#ifdef TFLAC_ENABLE_SSE2
    r |= STANDARD_TEST(0,sse2)();
    r |= STANDARD_TEST(1,sse2)();
    r |= STANDARD_TEST(2,sse2)();
    r |= STANDARD_TEST(3,sse2)();
    r |= STANDARD_TEST(4,sse2)();
#endif


#ifdef TFLAC_ENABLE_SSSE3
    r |= STANDARD_TEST(0,ssse3)();
    r |= STANDARD_TEST(1,ssse3)();
    r |= STANDARD_TEST(2,ssse3)();
    r |= STANDARD_TEST(3,ssse3)();
    r |= STANDARD_TEST(4,ssse3)();
#endif

#ifdef TFLAC_ENABLE_SSE4_1
    r |= STANDARD_TEST(0,sse4_1)();
    r |= STANDARD_TEST(1,sse4_1)();
    r |= STANDARD_TEST(2,sse4_1)();
    r |= STANDARD_TEST(3,sse4_1)();
    r |= STANDARD_TEST(4,sse4_1)();
#endif

    free(samples_unaligned);
    free(residuals_unaligned);
    return r;
}
