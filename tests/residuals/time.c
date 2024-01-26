#define TFLAC_IMPLEMENTATION
#include "tflac.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <stdint.h>

/* times various residual-calculating methods */

/* max blocksize */
#define BLOCKSIZE 65535
#define TESTRUNS 1000


static tflac_s32* samples_unaligned = NULL;
static tflac_s32* residuals_unaligned = NULL;

static tflac_s32* samples = NULL;
static tflac_s32* residuals = NULL;
static struct timespec* times = NULL;
static tflac_u64 result;

static void test_reset(void) {
    tflac_u32 i = 0;
    for(i=0;i<BLOCKSIZE;i++) {
        samples[i] = ((int16_t)(rand() % INT16_MAX)) * ( (rand() % 100 > 50 ? -1 : 1));
    }
}

static void* align_ptr(void* ptr) {
    tflac_u8 *d;
    tflac_u32 p1, p2;

    p1 = (tflac_u32)ptr;
    p2 = (p1 + 15) & ~(tflac_u32)0x0F;
    d = (tflac_u8*)ptr;

    return &d[p2 - p1];
}

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

static tflac_u32 time_cfr(void(*cfr)(tflac_u32, const tflac_s32*, tflac_s32*, tflac_u64*)) {
    tflac_u32 i = 0;
    struct timespec time_start;
    struct timespec time_end;
    struct timespec time_total;
    tflac_u32 time_avg;

    for(i=0;i<TESTRUNS;i++) {
        test_reset();
        clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &time_start);
        cfr(BLOCKSIZE,samples,residuals,&result);
        clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &time_end);
        times[i] = timespec_diff(time_start,time_end);
    }

    time_total.tv_sec = 0;
    time_total.tv_nsec = 0;

    for(i=0;i<TESTRUNS;i++) {
        time_total.tv_sec += times[i].tv_sec;
        time_total.tv_nsec += times[i].tv_nsec;
        if(time_total.tv_nsec > 1000000000) {
            time_total.tv_nsec -= 1000000000;
            time_total.tv_sec++;
        }
    }
    time_avg = (tflac_u32)(time_total.tv_sec * 1000000000); /* scale up to ns */
    time_avg += (tflac_u32)time_total.tv_nsec;
    time_avg /= TESTRUNS;

    return time_avg;

}


int main(void) {
    int r = 0;
    tflac_u32 std_times[5];
    tflac_u32 wide_times[5];
    tflac_u32 sse2_times[5];
    samples_unaligned = (tflac_s32*)malloc(15 + (sizeof(tflac_s32) * BLOCKSIZE));
    if(samples_unaligned == NULL) abort();
    residuals_unaligned = (tflac_s32*)malloc(15 + (sizeof(tflac_s32) * BLOCKSIZE));
    if(residuals_unaligned == NULL) abort();
    times = (struct timespec*)malloc(sizeof(struct timespec) * TESTRUNS);
    if(times == NULL) abort();

    samples = align_ptr(samples_unaligned);
    residuals = align_ptr(residuals_unaligned);

#if 0
    wide_times[0] = time_cfr(tflac_cfr_order0_wide_std);
    wide_times[1] = time_cfr(tflac_cfr_order1_wide_std);
    wide_times[2] = time_cfr(tflac_cfr_order2_wide_std);
    wide_times[3] = time_cfr(tflac_cfr_order3_wide_std);
    wide_times[4] = time_cfr(tflac_cfr_order4_wide_std);

    sse2_times[0] = time_cfr(tflac_cfr_order0_sse2);
    sse2_times[1] = time_cfr(tflac_cfr_order1_sse2);
    sse2_times[2] = time_cfr(tflac_cfr_order2_sse2);
    sse2_times[3] = time_cfr(tflac_cfr_order3_sse2);
    sse2_times[4] = time_cfr(tflac_cfr_order4_sse2);
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

    wide_times[0] = time_cfr(tflac_cfr_order0_wide_std);
    wide_times[1] = time_cfr(tflac_cfr_order1_wide_std);
    wide_times[2] = time_cfr(tflac_cfr_order2_wide_std);
    wide_times[3] = time_cfr(tflac_cfr_order3_wide_std);
    wide_times[4] = time_cfr(tflac_cfr_order4_wide_std);

    printf("|%6s|%13u|%13u|%13u|%14u|%14u|\n",
      "wstd", wide_times[0], wide_times[1], wide_times[2], wide_times[3], wide_times[4]);

    sse2_times[0] = time_cfr(tflac_cfr_order0_sse2);
    sse2_times[1] = time_cfr(tflac_cfr_order1_sse2);
    sse2_times[2] = time_cfr(tflac_cfr_order2_sse2);
    sse2_times[3] = time_cfr(tflac_cfr_order3_sse2);
    sse2_times[4] = time_cfr(tflac_cfr_order4_sse2);

    printf("|%6s|%13u|%13u|%13u|%14u|%14u|\n",
      "sse2", sse2_times[0], sse2_times[1], sse2_times[2], sse2_times[3], sse2_times[4]);
    printf("|______________________________________________________________________________|\n");


    free(samples_unaligned);
    free(residuals_unaligned);
    free(times);
    return r;
}

