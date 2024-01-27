#define TFLAC_IMPLEMENTATION
#include "tflac.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

/* times various residual-calculating methods */

/* max blocksize */
#define BLOCKSIZE 65535
#define TESTRUNS 1000

#if defined(_WIN32) || defined(_WIN64)
#define USE_QPC
#include <windows.h>
#else
#define CLOCK_ID CLOCK_REALTIME
#endif


static tflac_s32* samples_unaligned = NULL;
static tflac_s32* residuals_unaligned = NULL;

static tflac_s32* samples = NULL;
static tflac_s32* residuals = NULL;
#ifdef USE_QPC
static LARGE_INTEGER frequency;
#endif
static tflac_u64 result;

static void test_reset(void) {
    tflac_u32 i = 0;
    for(i=0;i<BLOCKSIZE;i++) {
        samples[i] = ((tflac_s16)(rand() % INT16_MAX)) * ( (rand() % 100 > 50 ? -1 : 1));
    }
}

static void* align_ptr(void* ptr) {
    tflac_u8 *d;
    tflac_uptr p1, p2;

    p1 = (tflac_uptr)ptr;
    p2 = (p1 + 15) & ~(tflac_uptr)0x0F;
    d = (tflac_u8*)ptr;

    return &d[p2 - p1];
}

#ifndef USE_QPC
static struct timespec timespec_diff(struct timespec start, struct timespec end) {
    struct timespec temp;
    if ((end.tv_nsec-start.tv_nsec)<0) {
        temp.tv_sec = end.tv_sec-start.tv_sec-1;
        temp.tv_nsec = 1000000000+end.tv_nsec-start.tv_nsec;
    } else {
        temp.tv_sec = end.tv_sec-start.tv_sec;
        temp.tv_nsec = end.tv_nsec-start.tv_nsec;
    }
    return temp;
}
#endif

static tflac_u32 time_cfr(void(*cfr)(tflac_u32, const tflac_s32* TFLAC_RESTRICT, tflac_s32* TFLAC_RESTRICT, tflac_u64* TFLAC_RESTRICT)) {
    tflac_u32 i = 0;
#ifdef USE_QPC
    LARGE_INTEGER time_start;
    LARGE_INTEGER time_end;
    LARGE_INTEGER time_total;
#else
    struct timespec time_start;
    struct timespec time_end;
    struct timespec time_total;
    struct timespec time_diff;
#endif
    tflac_u32 time_avg;

#ifdef USE_QPC
    time_total.QuadPart = 0;
#else
    time_total.tv_sec = 0;
    time_total.tv_nsec = 0;
#endif

    for(i=0;i<TESTRUNS;i++) {
        test_reset();
#ifdef USE_QPC
        QueryPerformanceCounter(&time_start);
#else
        clock_gettime(CLOCK_ID, &time_start);
#endif
        cfr(BLOCKSIZE,samples,residuals,&result);
#ifdef USE_QPC
        QueryPerformanceCounter(&time_end);
        time_total.QuadPart += time_end.QuadPart - time_start.QuadPart;
#else
        clock_gettime(CLOCK_ID, &time_end);
        time_diff = timespec_diff(time_start,time_end);
        time_total.tv_sec += time_diff.tv_sec;
        time_total.tv_nsec += time_diff.tv_nsec;
        if(time_total.tv_nsec > 1000000000) {
            time_total.tv_nsec -= 1000000000;
            time_total.tv_sec++;
        }
#endif
    }

    /* in both cases we convert the time to microseconds */
#ifdef USE_QPC
    time_total.QuadPart *= 1000000;
    time_total.QuadPart /= TESTRUNS;
    time_total.QuadPart /= frequency.QuadPart;
    time_avg = (tflac_u32)time_total.QuadPart;
#else
    time_avg = (tflac_u32)(time_total.tv_sec * 1000000); /* scale s up to us */
    time_avg += (tflac_u32)time_total.tv_nsec / 1000; /* scale ns down to us */
    time_avg /= TESTRUNS;
#endif

    return time_avg;

}


int main(void) {
    int r = 0;
    tflac_u32 std_times[5];
    tflac_u32 wide_times[5];
#ifdef TFLAC_ENABLE_SSE2
    tflac_u32 sse2_times[5];
#endif
#ifdef TFLAC_ENABLE_SSSE3
    tflac_u32 ssse3_times[5];
#endif
#ifdef TFLAC_ENABLE_SSE4_1
    tflac_u32 sse4_1_times[5];
#endif
    samples_unaligned = (tflac_s32*)malloc(15 + (sizeof(tflac_s32) * BLOCKSIZE));
    if(samples_unaligned == NULL) abort();
    residuals_unaligned = (tflac_s32*)malloc(15 + (sizeof(tflac_s32) * BLOCKSIZE));
    if(residuals_unaligned == NULL) abort();

    samples = align_ptr(samples_unaligned);
    residuals = align_ptr(residuals_unaligned);

#ifdef USE_QPC
    QueryPerformanceFrequency(&frequency);
#endif

    printf(".______________________________________________________________________________.\n");
    printf("|%6s|%13s|%13s|%13s|%14s|%14s|\n","","order0","order1","order2","order3","order4");
    printf("|------|-------------|-------------|-------------|--------------|--------------|\n");

    std_times[0] = time_cfr(tflac_cfr_order0_std);
    std_times[1] = time_cfr(tflac_cfr_order1_std);
    std_times[2] = time_cfr(tflac_cfr_order2_std);
    std_times[3] = time_cfr(tflac_cfr_order3_std);
    std_times[4] = time_cfr(tflac_cfr_order4_std);

    printf("|%6s|%13u|%13u|%13u|%14u|%14u|\n",
      "std", std_times[0], std_times[1], std_times[2], std_times[3], std_times[4]);

    wide_times[0] = std_times[0];
    wide_times[1] = time_cfr(tflac_cfr_order1_wide_std);
    wide_times[2] = time_cfr(tflac_cfr_order2_wide_std);
    wide_times[3] = time_cfr(tflac_cfr_order3_wide_std);
    wide_times[4] = time_cfr(tflac_cfr_order4_wide_std);

    printf("|%6s|%13u|%13u|%13u|%14u|%14u|\n",
      "wstd", wide_times[0], wide_times[1], wide_times[2], wide_times[3], wide_times[4]);

#ifdef TFLAC_ENABLE_SSE2

    sse2_times[0] = time_cfr(tflac_cfr_order0_sse2);
    sse2_times[1] = time_cfr(tflac_cfr_order1_sse2);
    sse2_times[2] = time_cfr(tflac_cfr_order2_sse2);
    sse2_times[3] = time_cfr(tflac_cfr_order3_sse2);
    sse2_times[4] = time_cfr(tflac_cfr_order4_sse2);

    printf("|%6s|%13u|%13u|%13u|%14u|%14u|\n",
      "sse2", sse2_times[0], sse2_times[1], sse2_times[2], sse2_times[3], sse2_times[4]);

#endif

#ifdef TFLAC_ENABLE_SSSE3

    ssse3_times[0] = time_cfr(tflac_cfr_order0_ssse3);
    ssse3_times[1] = time_cfr(tflac_cfr_order1_ssse3);
    ssse3_times[2] = time_cfr(tflac_cfr_order2_ssse3);
    ssse3_times[3] = time_cfr(tflac_cfr_order3_ssse3);
    ssse3_times[4] = time_cfr(tflac_cfr_order4_ssse3);
    printf("|%6s|%13u|%13u|%13u|%14u|%14u|\n",
      "ssse3", ssse3_times[0], ssse3_times[1], ssse3_times[2], ssse3_times[3], ssse3_times[4]);

#endif

#ifdef TFLAC_ENABLE_SSE4_1

    sse4_1_times[0] = time_cfr(tflac_cfr_order0_sse4_1);
    sse4_1_times[1] = time_cfr(tflac_cfr_order1_sse4_1);
    sse4_1_times[2] = time_cfr(tflac_cfr_order2_sse4_1);
    sse4_1_times[3] = time_cfr(tflac_cfr_order3_sse4_1);
    sse4_1_times[4] = time_cfr(tflac_cfr_order4_sse4_1);
    printf("|%6s|%13u|%13u|%13u|%14u|%14u|\n",
      "sse4_1", sse4_1_times[0], sse4_1_times[1], sse4_1_times[2], sse4_1_times[3], sse4_1_times[4]);

#endif

    printf("|______________________________________________________________________________|\n");


    free(samples_unaligned);
    free(residuals_unaligned);
    return r;
}

