
/*
 * Copyright (C) Niklaus F.Schen.
 *
 * Comprehensive tools module test:
 *   - full API coverage for mln_time2utc, mln_utc2time, mln_utc_adjust,
 *     mln_month_days, mln_s2time
 *   - edge cases: epoch zero, leap years, century boundaries, year 2000/2100
 *   - round-trip correctness: utc2time(time2utc(t)) == t
 *   - stability: randomized fuzz testing over wide time range
 *   - performance: benchmark against naive loop baseline; guard >= 2x speedup
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "mln_tools.h"
#include "mln_string.h"

#ifdef ASSERT
#undef ASSERT
#endif

static int passed, failed;

#define ASSERT(cond, msg) do { \
    if (cond) { ++passed; } \
    else { ++failed; fprintf(stderr, "FAIL [%s:%d] %s\n", __FILE__, __LINE__, msg); } \
} while (0)

static double now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
}

/* ---- naive O(n) baseline for benchmarking ---- */
static int naive_is_leap(long year)
{
    return ((year % 4 == 0) && (year % 100 != 0)) || (year % 400 == 0);
}

static long naive_mon_days[2][12] = {
    {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
    {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}
};

static void naive_time2utc(time_t tm, struct utctime *uc)
{
    long days = (long)(tm / 86400);
    long subsec = (long)(tm % 86400);
    long cnt = 0;
    uc->year = uc->month = 0;
    while ((naive_is_leap(1970 + uc->year) ? (cnt + 366) : (cnt + 365)) <= days) {
        if (naive_is_leap(1970 + uc->year)) cnt += 366;
        else cnt += 365;
        ++(uc->year);
    }
    uc->year += 1970;
    int is_leap = naive_is_leap(uc->year);
    long subdays = days - cnt;
    cnt = 0;
    while (cnt + naive_mon_days[is_leap][uc->month] <= subdays) {
        cnt += naive_mon_days[is_leap][uc->month];
        ++(uc->month);
    }
    ++(uc->month);
    uc->day = subdays - cnt + 1;
    uc->hour = subsec / 3600;
    uc->minute = (subsec % 3600) / 60;
    uc->second = (subsec % 3600) % 60;
}

static time_t naive_utc2time(struct utctime *uc)
{
    time_t ret = 0;
    long year = uc->year - 1, month = uc->month - 2;
    int is_leap = naive_is_leap(uc->year);
    for (; year >= 1970; --year)
        ret += (naive_is_leap(year) ? 366 : 365);
    for (; month >= 0; --month)
        ret += naive_mon_days[is_leap][month];
    ret += (uc->day - 1);
    ret *= 86400;
    ret += (uc->hour * 3600 + uc->minute * 60 + uc->second);
    return ret;
}

/* ===========================================================
 *  1. mln_time2utc
 * =========================================================== */
static void test_time2utc(void)
{
    struct utctime uc;
    char buf[64];

    /* epoch zero: 1970-01-01 00:00:00 Thursday */
    mln_time2utc(0, &uc);
    ASSERT(uc.year == 1970, "time2utc(0): year == 1970");
    ASSERT(uc.month == 1, "time2utc(0): month == 1");
    ASSERT(uc.day == 1, "time2utc(0): day == 1");
    ASSERT(uc.hour == 0, "time2utc(0): hour == 0");
    ASSERT(uc.minute == 0, "time2utc(0): minute == 0");
    ASSERT(uc.second == 0, "time2utc(0): second == 0");

    /* 2000-01-01 00:00:00 (946684800) Saturday */
    mln_time2utc(946684800, &uc);
    ASSERT(uc.year == 2000, "time2utc(Y2K): year == 2000");
    ASSERT(uc.month == 1, "time2utc(Y2K): month == 1");
    ASSERT(uc.day == 1, "time2utc(Y2K): day == 1");

    /* 2000-02-29 (leap day) */
    mln_time2utc(951782400, &uc);
    ASSERT(uc.year == 2000, "time2utc(2000-02-29): year == 2000");
    ASSERT(uc.month == 2, "time2utc(2000-02-29): month == 2");
    ASSERT(uc.day == 29, "time2utc(2000-02-29): day == 29");

    /* 2000-03-01 (day after leap day) */
    mln_time2utc(951868800, &uc);
    ASSERT(uc.year == 2000, "time2utc(2000-03-01): year == 2000");
    ASSERT(uc.month == 3, "time2utc(2000-03-01): month == 3");
    ASSERT(uc.day == 1, "time2utc(2000-03-01): day == 1");

    /* 1970-12-31 23:59:59 */
    mln_time2utc(31535999, &uc);
    ASSERT(uc.year == 1970, "time2utc(end-1970): year == 1970");
    ASSERT(uc.month == 12, "time2utc(end-1970): month == 12");
    ASSERT(uc.day == 31, "time2utc(end-1970): day == 31");
    ASSERT(uc.hour == 23, "time2utc(end-1970): hour == 23");
    ASSERT(uc.minute == 59, "time2utc(end-1970): minute == 59");
    ASSERT(uc.second == 59, "time2utc(end-1970): second == 59");

    /* 1900 is not a leap year (century rule) - test via 2100 which is also not leap */
    /* 2100-03-01 00:00:00 = 4107542400 */
    mln_time2utc(4107542400L, &uc);
    ASSERT(uc.year == 2100, "time2utc(2100-03-01): year == 2100");
    ASSERT(uc.month == 3, "time2utc(2100-03-01): month == 3");
    ASSERT(uc.day == 1, "time2utc(2100-03-01): day == 1");

    /* 2100-02-28 (no leap day in 2100) */
    mln_time2utc(4107456000L, &uc);
    ASSERT(uc.year == 2100, "time2utc(2100-02-28): year == 2100");
    ASSERT(uc.month == 2, "time2utc(2100-02-28): month == 2");
    ASSERT(uc.day == 28, "time2utc(2100-02-28): day == 28");

    /* 2024-02-29 (leap year) = 1709164800 */
    mln_time2utc(1709164800L, &uc);
    ASSERT(uc.year == 2024, "time2utc(2024-02-29): year == 2024");
    ASSERT(uc.month == 2, "time2utc(2024-02-29): month == 2");
    ASSERT(uc.day == 29, "time2utc(2024-02-29): day == 29");

    /* cross-check with naive for a far-future date: 2200-06-15 12:30:45 */
    {
        struct utctime uc_naive;
        /* Build from naive: 2200-06-15 12:30:45 */
        uc_naive.year = 2200; uc_naive.month = 6; uc_naive.day = 15;
        uc_naive.hour = 12; uc_naive.minute = 30; uc_naive.second = 45;
        time_t t = naive_utc2time(&uc_naive);
        mln_time2utc(t, &uc);
        ASSERT(uc.year == 2200, "time2utc(2200-06-15): year");
        ASSERT(uc.month == 6, "time2utc(2200-06-15): month");
        ASSERT(uc.day == 15, "time2utc(2200-06-15): day");
        ASSERT(uc.hour == 12, "time2utc(2200-06-15): hour");
        ASSERT(uc.minute == 30, "time2utc(2200-06-15): minute");
        ASSERT(uc.second == 45, "time2utc(2200-06-15): second");
    }

    /* verify week day for known date: 2026-04-19 is Sunday = 0 */
    mln_time2utc(1776556800L, &uc);
    sprintf(buf, "time2utc(2026-04-19): week == 0 (Sunday), got %ld", uc.week);
    ASSERT(uc.week == 0, buf);
}

/* ===========================================================
 *  2. mln_utc2time
 * =========================================================== */
static void test_utc2time(void)
{
    struct utctime uc;
    time_t t;

    /* epoch */
    memset(&uc, 0, sizeof(uc));
    uc.year = 1970; uc.month = 1; uc.day = 1;
    t = mln_utc2time(&uc);
    ASSERT(t == 0, "utc2time(1970-01-01): t == 0");

    /* Y2K */
    uc.year = 2000; uc.month = 1; uc.day = 1;
    uc.hour = 0; uc.minute = 0; uc.second = 0;
    t = mln_utc2time(&uc);
    ASSERT(t == 946684800, "utc2time(2000-01-01): t == 946684800");

    /* leap day 2000-02-29 */
    uc.year = 2000; uc.month = 2; uc.day = 29;
    uc.hour = 0; uc.minute = 0; uc.second = 0;
    t = mln_utc2time(&uc);
    ASSERT(t == 951782400, "utc2time(2000-02-29): t == 951782400");

    /* 2100-03-01 */
    uc.year = 2100; uc.month = 3; uc.day = 1;
    uc.hour = 0; uc.minute = 0; uc.second = 0;
    t = mln_utc2time(&uc);
    ASSERT(t == 4107542400L, "utc2time(2100-03-01): t == 4107542400");

    /* with time component */
    uc.year = 1970; uc.month = 1; uc.day = 2;
    uc.hour = 3; uc.minute = 25; uc.second = 45;
    t = mln_utc2time(&uc);
    ASSERT(t == 86400 + 3 * 3600 + 25 * 60 + 45, "utc2time(1970-01-02 03:25:45)");
}

/* ===========================================================
 *  3. Round-trip: utc2time(time2utc(t)) == t
 * =========================================================== */
static void test_roundtrip(void)
{
    struct utctime uc;
    time_t t, rt;
    char buf[128];

    /* systematic test: every year boundary from 1970 to 2200 */
    for (long y = 1970; y <= 2200; ++y) {
        uc.year = y; uc.month = 1; uc.day = 1;
        uc.hour = 0; uc.minute = 0; uc.second = 0;
        t = mln_utc2time(&uc);
        mln_time2utc(t, &uc);
        rt = mln_utc2time(&uc);
        if (t != rt || uc.year != y || uc.month != 1 || uc.day != 1) {
            sprintf(buf, "roundtrip year %ld failed", y);
            ASSERT(0, buf);
            return;
        }
    }
    ASSERT(1, "roundtrip: all year boundaries 1970-2200 pass");

    /* random timestamps */
    srand(42);
    for (int i = 0; i < 100000; ++i) {
        /* random time_t in [0, 2^31) covers 1970-2038 */
        t = (time_t)(((unsigned long)rand() << 1) ^ (unsigned long)rand()) & 0x7FFFFFFFL;
        mln_time2utc(t, &uc);
        rt = mln_utc2time(&uc);
        if (t != rt) {
            sprintf(buf, "roundtrip rand failed: t=%ld rt=%ld", (long)t, (long)rt);
            ASSERT(0, buf);
            return;
        }
    }
    ASSERT(1, "roundtrip: 100000 random timestamps pass");

    /* large timestamps: 2100+ range */
    for (long i = 0; i < 10000; ++i) {
        t = 4000000000L + (time_t)(rand() % 200000000);
        mln_time2utc(t, &uc);
        rt = mln_utc2time(&uc);
        if (t != rt) {
            sprintf(buf, "roundtrip far-future failed: t=%ld rt=%ld", (long)t, (long)rt);
            ASSERT(0, buf);
            return;
        }
    }
    ASSERT(1, "roundtrip: 10000 far-future timestamps pass");
}

/* ===========================================================
 *  4. mln_utc_adjust
 * =========================================================== */
static void test_utc_adjust(void)
{
    struct utctime uc;

    /* overflow seconds */
    uc.year = 2020; uc.month = 1; uc.day = 1;
    uc.hour = 0; uc.minute = 0; uc.second = 3661;
    mln_utc_adjust(&uc);
    ASSERT(uc.hour == 1, "adjust(3661s): hour == 1");
    ASSERT(uc.minute == 1, "adjust(3661s): minute == 1");
    ASSERT(uc.second == 1, "adjust(3661s): second == 1");

    /* overflow days across month */
    uc.year = 2020; uc.month = 1; uc.day = 32;
    uc.hour = 0; uc.minute = 0; uc.second = 0;
    mln_utc_adjust(&uc);
    ASSERT(uc.year == 2020, "adjust(Jan 32): year == 2020");
    ASSERT(uc.month == 2, "adjust(Jan 32): month == 2");
    ASSERT(uc.day == 1, "adjust(Jan 32): day == 1");

    /* overflow across year */
    uc.year = 2020; uc.month = 12; uc.day = 32;
    uc.hour = 0; uc.minute = 0; uc.second = 0;
    mln_utc_adjust(&uc);
    ASSERT(uc.year == 2021, "adjust(Dec 32): year == 2021");
    ASSERT(uc.month == 1, "adjust(Dec 32): month == 1");
    ASSERT(uc.day == 1, "adjust(Dec 32): day == 1");

    /* large overflow: 400 days */
    uc.year = 2020; uc.month = 1; uc.day = 401;
    uc.hour = 0; uc.minute = 0; uc.second = 0;
    mln_utc_adjust(&uc);
    ASSERT(uc.year == 2021, "adjust(Jan 401): year == 2021");
    ASSERT(uc.month == 2, "adjust(Jan 401): month == 2");
    ASSERT(uc.day == 4, "adjust(Jan 401): day == 4");

    /* verify week day is set after adjust */
    uc.year = 2020; uc.month = 1; uc.day = 1;
    uc.hour = 0; uc.minute = 0; uc.second = 0;
    uc.week = -1;
    mln_utc_adjust(&uc);
    /* 2020-01-01 is Wednesday = 3 */
    ASSERT(uc.week == 3, "adjust: week set correctly for 2020-01-01 (Wed)");

    /* leap year Feb overflow: Feb 30 in a leap year */
    uc.year = 2024; uc.month = 2; uc.day = 30;
    uc.hour = 0; uc.minute = 0; uc.second = 0;
    mln_utc_adjust(&uc);
    ASSERT(uc.year == 2024, "adjust(2024 Feb 30): year == 2024");
    ASSERT(uc.month == 3, "adjust(2024 Feb 30): month == 3");
    ASSERT(uc.day == 1, "adjust(2024 Feb 30): day == 1");

    /* non-leap year Feb overflow: Feb 29 in 2023 */
    uc.year = 2023; uc.month = 2; uc.day = 29;
    uc.hour = 0; uc.minute = 0; uc.second = 0;
    mln_utc_adjust(&uc);
    ASSERT(uc.year == 2023, "adjust(2023 Feb 29): year == 2023");
    ASSERT(uc.month == 3, "adjust(2023 Feb 29): month == 3");
    ASSERT(uc.day == 1, "adjust(2023 Feb 29): day == 1");
}

/* ===========================================================
 *  5. mln_month_days
 * =========================================================== */
static void test_month_days(void)
{
    /* non-leap year */
    long expected_normal[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    for (int m = 1; m <= 12; ++m) {
        char buf[64];
        sprintf(buf, "month_days(2023, %d) == %ld", m, expected_normal[m-1]);
        ASSERT(mln_month_days(2023, m) == expected_normal[m-1], buf);
    }

    /* leap year */
    ASSERT(mln_month_days(2000, 2) == 29, "month_days(2000, 2) == 29 (leap, div 400)");
    ASSERT(mln_month_days(2024, 2) == 29, "month_days(2024, 2) == 29 (leap)");
    ASSERT(mln_month_days(1900, 2) == 28, "month_days(1900, 2) == 28 (century, not leap)");
    ASSERT(mln_month_days(2100, 2) == 28, "month_days(2100, 2) == 28 (century, not leap)");
}

/* ===========================================================
 *  6. mln_s2time
 * =========================================================== */
static void test_s2time(void)
{
    time_t t;
    int ret;

    /* UTC format: YYMMDDhhmmssZ (13 chars) */
    {
        mln_string_t s = mln_string("700101000000Z");
        ret = mln_s2time(&t, &s, M_TOOLS_TIME_UTC);
        ASSERT(ret == 0, "s2time(UTC 700101000000Z): ret == 0");
        ASSERT(t == 0, "s2time(UTC 700101000000Z): t == 0");
    }

    /* UTC: year >= 50 means 19xx */
    {
        mln_string_t s = mln_string("991231235959Z");
        ret = mln_s2time(&t, &s, M_TOOLS_TIME_UTC);
        ASSERT(ret == 0, "s2time(UTC 991231235959Z): ret == 0");
        /* 1999-12-31 23:59:59 = 946684799 */
        ASSERT(t == 946684799, "s2time(UTC 991231235959Z): t == 946684799");
    }

    /* UTC: year < 50 means 20xx */
    {
        mln_string_t s = mln_string("000101000000Z");
        ret = mln_s2time(&t, &s, M_TOOLS_TIME_UTC);
        ASSERT(ret == 0, "s2time(UTC 000101000000Z): ret == 0");
        ASSERT(t == 946684800, "s2time(UTC 000101000000Z): t == 946684800");
    }

    /* Generalized time: YYYYMMDDhhmmssZ (15 chars) */
    {
        mln_string_t s = mln_string("20000229000000Z");
        ret = mln_s2time(&t, &s, M_TOOLS_TIME_GENERALIZEDTIME);
        ASSERT(ret == 0, "s2time(GT 20000229): ret == 0");
        ASSERT(t == 951782400, "s2time(GT 20000229): t == 951782400");
    }

    /* lowercase z */
    {
        mln_string_t s = mln_string("20240229120000z");
        ret = mln_s2time(&t, &s, M_TOOLS_TIME_GENERALIZEDTIME);
        ASSERT(ret == 0, "s2time(GT lowercase z): ret == 0");
    }

    /* invalid: wrong length */
    {
        mln_string_t s = mln_string("2000022900000Z");
        ret = mln_s2time(&t, &s, M_TOOLS_TIME_GENERALIZEDTIME);
        ASSERT(ret == -1, "s2time(GT wrong length): ret == -1");
    }

    /* invalid: no Z suffix */
    {
        mln_string_t s = mln_string("20000229000000X");
        ret = mln_s2time(&t, &s, M_TOOLS_TIME_GENERALIZEDTIME);
        ASSERT(ret == -1, "s2time(GT no Z): ret == -1");
    }

    /* invalid: non-digit */
    {
        mln_string_t s = mln_string("2000022900A000Z");
        ret = mln_s2time(&t, &s, M_TOOLS_TIME_GENERALIZEDTIME);
        ASSERT(ret == -1, "s2time(GT non-digit): ret == -1");
    }

    /* invalid: month > 12 */
    {
        mln_string_t s = mln_string("20001301000000Z");
        ret = mln_s2time(&t, &s, M_TOOLS_TIME_GENERALIZEDTIME);
        ASSERT(ret == -1, "s2time(GT month 13): ret == -1");
    }

    /* invalid: Feb 29 in non-leap year */
    {
        mln_string_t s = mln_string("19000229000000Z");
        ret = mln_s2time(&t, &s, M_TOOLS_TIME_GENERALIZEDTIME);
        ASSERT(ret == -1, "s2time(GT 1900-02-29 non-leap): ret == -1");
    }

    /* invalid: year < 1970 */
    {
        mln_string_t s = mln_string("19690101000000Z");
        ret = mln_s2time(&t, &s, M_TOOLS_TIME_GENERALIZEDTIME);
        ASSERT(ret == -1, "s2time(GT year 1969): ret == -1");
    }

    /* invalid type */
    {
        mln_string_t s = mln_string("20000101000000Z");
        ret = mln_s2time(&t, &s, 99);
        ASSERT(ret == -1, "s2time(invalid type): ret == -1");
    }

    /* cross-check s2time vs utc2time */
    {
        mln_string_t s = mln_string("20240229153045Z");
        struct utctime uc;
        uc.year = 2024; uc.month = 2; uc.day = 29;
        uc.hour = 15; uc.minute = 30; uc.second = 45;
        time_t t1, t2;
        ret = mln_s2time(&t1, &s, M_TOOLS_TIME_GENERALIZEDTIME);
        t2 = mln_utc2time(&uc);
        ASSERT(ret == 0, "s2time cross-check: ret == 0");
        ASSERT(t1 == t2, "s2time cross-check: s2time == utc2time");
    }

    /* far future: 2099-12-31 23:59:59 */
    {
        mln_string_t s = mln_string("20991231235959Z");
        ret = mln_s2time(&t, &s, M_TOOLS_TIME_GENERALIZEDTIME);
        ASSERT(ret == 0, "s2time(GT 2099): ret == 0");
        struct utctime uc;
        mln_time2utc(t, &uc);
        ASSERT(uc.year == 2099 && uc.month == 12 && uc.day == 31,
               "s2time(GT 2099): roundtrip matches");
    }
}

/* ===========================================================
 *  7. Performance benchmark
 * =========================================================== */
static void test_performance(void)
{
    struct utctime uc;
    time_t t;
    double t0, t1;
    int iterations = 2000000;
    volatile time_t sink = 0;

    printf("\n--- Performance Benchmark (%d iterations) ---\n", iterations);

    /* Benchmark mln_time2utc vs naive (far-future dates for max loop cost) */
    {
        /* naive baseline */
        t0 = now_ms();
        for (int i = 0; i < iterations; ++i) {
            /* timestamps around year 2080 to stress the year loop */
            time_t ts = 3500000000L + (i % 100000);
            naive_time2utc(ts, &uc);
            sink += uc.year;
        }
        t1 = now_ms();
        double naive_ms = t1 - t0;

        /* optimized */
        t0 = now_ms();
        for (int i = 0; i < iterations; ++i) {
            time_t ts = 3500000000L + (i % 100000);
            mln_time2utc(ts, &uc);
            sink += uc.year;
        }
        t1 = now_ms();
        double opt_ms = t1 - t0;

        double speedup = naive_ms / opt_ms;
        printf("  time2utc:  naive=%.1fms  opt=%.1fms  speedup=%.1fx\n",
               naive_ms, opt_ms, speedup);
        ASSERT(speedup >= 2.0, "time2utc: >= 2x speedup over naive");
    }

    /* Benchmark mln_utc2time vs naive */
    {
        struct utctime uc_bench;
        uc_bench.hour = 12; uc_bench.minute = 30; uc_bench.second = 45;

        /* naive baseline */
        t0 = now_ms();
        for (int i = 0; i < iterations; ++i) {
            uc_bench.year = 2050 + (i % 100);
            uc_bench.month = (i % 12) + 1;
            uc_bench.day = (i % 28) + 1;
            sink += naive_utc2time(&uc_bench);
        }
        t1 = now_ms();
        double naive_ms = t1 - t0;

        /* optimized */
        t0 = now_ms();
        for (int i = 0; i < iterations; ++i) {
            uc_bench.year = 2050 + (i % 100);
            uc_bench.month = (i % 12) + 1;
            uc_bench.day = (i % 28) + 1;
            sink += mln_utc2time(&uc_bench);
        }
        t1 = now_ms();
        double opt_ms = t1 - t0;

        double speedup = naive_ms / opt_ms;
        printf("  utc2time:  naive=%.1fms  opt=%.1fms  speedup=%.1fx\n",
               naive_ms, opt_ms, speedup);
        ASSERT(speedup >= 2.0, "utc2time: >= 2x speedup over naive");
    }

    /* Benchmark mln_s2time (parsing + conversion) */
    {
        mln_string_t s = mln_string("20800615123045Z");

        t0 = now_ms();
        for (int i = 0; i < iterations; ++i) {
            mln_s2time(&t, &s, M_TOOLS_TIME_GENERALIZEDTIME);
            sink += t;
        }
        t1 = now_ms();
        printf("  s2time:    %.1fms for %d calls (%.0f ns/call)\n",
               t1 - t0, iterations, (t1 - t0) * 1e6 / iterations);
    }

    (void)sink;
}

/* ===========================================================
 *  8. Stability: fuzz + stress
 * =========================================================== */
static void test_stability(void)
{
    struct utctime uc1, uc2;
    char buf[128];

    /* cross-check optimized vs naive for 500K random timestamps */
    srand(12345);
    for (int i = 0; i < 500000; ++i) {
        time_t t = (time_t)(((unsigned long)rand() << 1) ^ (unsigned long)rand()) & 0x7FFFFFFFL;
        mln_time2utc(t, &uc1);
        naive_time2utc(t, &uc2);
        if (uc1.year != uc2.year || uc1.month != uc2.month || uc1.day != uc2.day ||
            uc1.hour != uc2.hour || uc1.minute != uc2.minute || uc1.second != uc2.second) {
            sprintf(buf, "stability: mismatch at t=%ld opt=(%ld-%ld-%ld) naive=(%ld-%ld-%ld)",
                    (long)t, uc1.year, uc1.month, uc1.day,
                    uc2.year, uc2.month, uc2.day);
            ASSERT(0, buf);
            return;
        }
    }
    ASSERT(1, "stability: 500K random timestamps match naive");

    /* far-future cross-check */
    for (long i = 0; i < 100000; ++i) {
        time_t t = 4000000000L + (time_t)(rand() % 300000000);
        mln_time2utc(t, &uc1);
        naive_time2utc(t, &uc2);
        if (uc1.year != uc2.year || uc1.month != uc2.month || uc1.day != uc2.day) {
            sprintf(buf, "stability far-future: mismatch at t=%ld", (long)t);
            ASSERT(0, buf);
            return;
        }
    }
    ASSERT(1, "stability: 100K far-future timestamps match naive");

    /* utc2time cross-check */
    srand(99);
    for (int i = 0; i < 200000; ++i) {
        uc1.year = 1970 + (rand() % 200);
        uc1.month = (rand() % 12) + 1;
        uc1.day = (rand() % 28) + 1;
        uc1.hour = rand() % 24;
        uc1.minute = rand() % 60;
        uc1.second = rand() % 60;
        time_t t1 = mln_utc2time(&uc1);
        time_t t2 = naive_utc2time(&uc1);
        if (t1 != t2) {
            sprintf(buf, "stability utc2time: mismatch at %ld-%ld-%ld t1=%ld t2=%ld",
                    uc1.year, uc1.month, uc1.day, (long)t1, (long)t2);
            ASSERT(0, buf);
            return;
        }
    }
    ASSERT(1, "stability: 200K utc2time matches naive");
}

/* =========================================================== */
int main(void)
{
    passed = failed = 0;

    test_time2utc();
    test_utc2time();
    test_roundtrip();
    test_utc_adjust();
    test_month_days();
    test_s2time();
    test_stability();
    test_performance();

    printf("\n========================================\n");
    printf("  tools test: %d passed, %d failed\n", passed, failed);
    printf("========================================\n");

    return failed ? 1 : 0;
}
