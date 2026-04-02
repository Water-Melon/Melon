
/*
 * Copyright (C) Niklaus F.Schen.
 */

#include "mln_cron.h"
#include "mln_tools.h"
#include "mln_func.h"
#include <string.h>

static long mln_cron_parse_minute(mln_string_t *smin, long min);
static long mln_cron_parse_hour(mln_string_t *shour, long hour, int greater);
static long mln_cron_parse_day(mln_string_t *sday, long year, long month, long day, int greater);
static long mln_cron_parse_month(mln_string_t *smon, long mon, int greater);
static long mln_cron_parse_week(mln_string_t *sweek, long week, int greater);
static int mln_cron_replace_names(mln_string_t *field, const char **names, int n_names);

/* ---- O(1) inline time helpers to avoid O(n) year loops in mln_tools ---- */

static const long mln_cron_mon_days[2][12] = {
    {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
    {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}
};

static inline int mln_cron_is_leap(long y)
{
    return ((y % 4 == 0) && (y % 100 != 0)) || (y % 400 == 0);
}

/* O(1) time_t -> UTC using Howard Hinnant's civil calendar algorithm */
static inline void mln_cron_time2utc(time_t tm, struct utctime *uc)
{
    long days = (long)(tm / 86400);
    long subsec = (long)(tm % 86400);
    long z, era, doe, yoe, y, doy, mp, d, m;

    z = days + 719468;
    era = (z >= 0 ? z : z - 146096) / 146097;
    doe = z - era * 146097;
    yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    y = yoe + era * 400;
    doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    mp = (5 * doy + 2) / 153;
    d = doy - (153 * mp + 2) / 5 + 1;
    m = mp + (mp < 10 ? 3 : -9);
    y += (m <= 2);

    uc->year = y;
    uc->month = m;
    uc->day = d;
    uc->hour = subsec / 3600;
    uc->minute = (subsec % 3600) / 60;
    uc->second = subsec % 60;

    {
        long ma = m < 3 ? m + 12 : m;
        long ya = m < 3 ? y - 1 : y;
        uc->week = (d + 1 + 2 * ma + 3 * (ma + 1) / 5 + ya + (ya >> 2) - ya / 100 + ya / 400) % 7;
    }
}

/* O(1) UTC -> time_t using cumulative day table */
static inline time_t mln_cron_utc2time(struct utctime *uc)
{
    static const int cum[] = {0, 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
    long y = uc->year - 1;
    long days = 365L * (uc->year - 1970)
                + (y / 4 - 492)      /* 1969/4 = 492 */
                - (y / 100 - 19)     /* 1969/100 = 19 */
                + (y / 400 - 4);     /* 1969/400 = 4 */
    days += cum[uc->month] + uc->day - 1;
    if (uc->month > 2 && mln_cron_is_leap(uc->year)) days++;
    return (time_t)(days * 86400 + uc->hour * 3600 + uc->minute * 60 + uc->second);
}

/* O(1) UTC struct comparison: is a strictly later than b? */
static inline int mln_cron_utc_gt(struct utctime *a, struct utctime *b)
{
    if (a->year != b->year) return a->year > b->year;
    if (a->month != b->month) return a->month > b->month;
    if (a->day != b->day) return a->day > b->day;
    if (a->hour != b->hour) return a->hour > b->hour;
    if (a->minute != b->minute) return a->minute > b->minute;
    return a->second > b->second;
}

/* Inline utc_adjust: normalize overflowed fields (second -> year) */
static inline void mln_cron_utc_adjust(struct utctime *uc)
{
    long adj = 0, month, year;

    if (uc->second >= 60) { adj = uc->second / 60; uc->second %= 60; }
    if (adj) { uc->minute += adj; adj = 0; }
    if (uc->minute >= 60) { adj = uc->minute / 60; uc->minute %= 60; }
    if (adj) { uc->hour += adj; adj = 0; }
    if (uc->hour >= 24) { adj = uc->hour / 24; uc->hour %= 24; }
    if (adj) { uc->day += adj; adj = 0; }
    year = uc->year;
    month = uc->month;
    while (uc->day > mln_cron_mon_days[mln_cron_is_leap(year)][month - 1]) {
        uc->day -= mln_cron_mon_days[mln_cron_is_leap(year)][month - 1];
        if (++month > 12) { month = 1; ++year; }
        adj = 1;
    }
    if (adj) { uc->month = month; uc->year = year; adj = 0; }
    if (uc->month - 1 >= 12) {
        adj = (uc->month - 1) / 12;
        uc->month = (uc->month - 1) % 12 + 1;
    }
    if (adj) uc->year += adj;
    {
        long ma = uc->month < 3 ? uc->month + 12 : uc->month;
        long ya = uc->month < 3 ? uc->year - 1 : uc->year;
        uc->week = (uc->day + 1 + 2 * ma + 3 * (ma + 1) / 5 + ya + (ya >> 2) - ya / 100 + ya / 400) % 7;
    }
}

/*
 * Try a candidate value and update *save to the nearest future match.
 * is_minute: minute field has no 'greater' flag and always advances.
 */
static inline void mln_cron_try(long *save, long val, long cur, long period, long wrap, int is_minute, int greater)
{
    if (val < cur) val += wrap;
    if (!*save) {
        if (is_minute)
            *save = (cur == val) ? cur + period : val;
        else
            *save = (cur == val && !greater) ? cur + period : val;
    } else {
        if (is_minute) {
            if ((val == cur && period < *save - cur) || (val - cur < *save - cur))
                *save = (cur == val) ? cur + period : val;
        } else {
            if (val == cur) {
                if (greater) *save = val;
                else if (period < *save - cur) *save = cur + period;
            } else if (val - cur < *save - cur) {
                *save = val;
            }
        }
    }
}

/*
 * Generic cron field parser.
 * Supports: * (wildcard), N (value), N,N (list), N-N (range), /N (step).
 * Range and list can be combined: 1-5,10,20-25
 * Wrap-around ranges are supported: 22-3 for hours means 22,23,0,1,2,3.
 *
 * val_min: minimum valid value (inclusive)
 * val_max: maximum valid value (exclusive, e.g. 60 for minutes)
 * wrap: wrap-around amount when val < cur
 * max_digits: max digits per number token (2 for most fields, 1 for week)
 * is_minute: true for minute field (no greater flag, * means cur+1)
 * greater: true if a previous field already advanced the time
 */
static long mln_cron_parse_field(mln_string_t *s, long cur,
                                  long val_min, long val_max, long wrap,
                                  int max_digits, int is_minute, int greater)
{
    if (!s->len) return -1;
    if (s->len == 1 && s->data[0] == '*')
        return is_minute ? cur + 1 : cur;

    long period = 0, save = 0, tmp, range_start = -1;
    mln_u8ptr_t head = s->data, p = s->data, end = s->data + s->len;

    /* Extract step from trailing /N (scan backward without UB) */
    {
        mln_u8ptr_t scan = s->data + s->len;
        while (scan > s->data) {
            --scan;
            if (*scan == '/') {
                period = atol((char *)(scan + 1));
                if (period < 1 || period >= val_max) return -1;
                *scan = 0;
                end = scan;
                break;
            }
        }
    }

#define CRON_NORM(v) do { if (val_max == 7 && (v) == 7) (v) = 0; } while(0)
#define CRON_CHK(v) ((v) >= val_min && (v) < val_max)
#define CRON_RANGE(rs, re) do { \
    long _crs = (rs), _cre = (re); \
    if (_crs <= _cre) { \
        for (tmp = _crs; tmp <= _cre; ++tmp) \
            mln_cron_try(&save, tmp, cur, period, wrap, is_minute, greater); \
    } else { \
        for (tmp = _crs; tmp < val_max; ++tmp) \
            mln_cron_try(&save, tmp, cur, period, wrap, is_minute, greater); \
        for (tmp = val_min; tmp <= _cre; ++tmp) \
            mln_cron_try(&save, tmp, cur, period, wrap, is_minute, greater); \
    } \
} while(0)

    while (p < end) {
        switch (*p) {
            case '-':
                if (p == head || p > head + max_digits) return -1;
                *p = 0;
                range_start = atol((char *)head);
                CRON_NORM(range_start);
                if (!CRON_CHK(range_start)) return -1;
                head = ++p;
                break;
            case ',':
                if (p == head || p > head + max_digits) return -1;
                *p = 0;
                if (range_start >= 0) {
                    tmp = atol((char *)head);
                    CRON_NORM(tmp);
                    if (!CRON_CHK(tmp)) return -1;
                    CRON_RANGE(range_start, tmp);
                    range_start = -1;
                } else {
                    tmp = atol((char *)head);
                    CRON_NORM(tmp);
                    if (!CRON_CHK(tmp)) return -1;
                    mln_cron_try(&save, tmp, cur, period, wrap, is_minute, greater);
                }
                head = ++p;
                break;
            case '*':
                save = is_minute
                    ? cur + (period ? period : 1)
                    : (greater ? cur : cur + (period ? period : 1));
                if (*(++p) == ',') head = ++p;
                else head = p;
                range_start = -1;
                break;
            default:
                if (!mln_isdigit(*p)) return -1;
                ++p;
                break;
        }
    }

    if (p > head) {
        if (p > head + max_digits) return -1;
        if (range_start >= 0) {
            tmp = atol((char *)head);
            CRON_NORM(tmp);
            if (!CRON_CHK(tmp)) return -1;
            CRON_RANGE(range_start, tmp);
        } else {
            tmp = atol((char *)head);
            CRON_NORM(tmp);
            if (!CRON_CHK(tmp)) return -1;
            mln_cron_try(&save, tmp, cur, period, wrap, is_minute, greater);
        }
    } else if (range_start >= 0) {
        return -1; /* unterminated range e.g. "5-" */
    }

#undef CRON_NORM
#undef CRON_CHK
#undef CRON_RANGE
    return save;
}

static const char *mln_cron_month_names[] = {
    NULL, "JAN", "FEB", "MAR", "APR", "MAY", "JUN",
    "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"
};

static const char *mln_cron_week_names[] = {
    "SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"
};

/*
 * Replace 3-letter name tokens (case-insensitive) with their numeric indices.
 * Works in-place since numbers are always shorter than or equal to names.
 */
MLN_FUNC(static, int, mln_cron_replace_names, \
         (mln_string_t *field, const char **names, int n_names), \
         (field, names, n_names), \
{
    mln_u8ptr_t p = field->data;
    mln_u8ptr_t end = field->data + field->len;
    mln_u8ptr_t wp = field->data;
    int i, found;
    char u0, u1, u2;

    while (p < end) {
        if (end - p >= 3 &&
            ((*p >= 'A' && *p <= 'Z') || (*p >= 'a' && *p <= 'z')) &&
            ((p[1] >= 'A' && p[1] <= 'Z') || (p[1] >= 'a' && p[1] <= 'z')) &&
            ((p[2] >= 'A' && p[2] <= 'Z') || (p[2] >= 'a' && p[2] <= 'z')))
        {
            u0 = *p & ~0x20;
            u1 = p[1] & ~0x20;
            u2 = p[2] & ~0x20;
            found = -1;
            for (i = 0; i < n_names; i++) {
                if (names[i] && u0 == names[i][0] && u1 == names[i][1] && u2 == names[i][2]) {
                    found = i;
                    break;
                }
            }
            if (found >= 0) {
                if (found >= 10) {
                    *wp++ = '0' + found / 10;
                    *wp++ = '0' + found % 10;
                } else {
                    *wp++ = '0' + found;
                }
                p += 3;
                continue;
            }
        }
        *wp++ = *p++;
    }
    *wp = 0;
    field->len = wp - field->data;
    return 0;
})

MLN_FUNC(, time_t, mln_cron_parse, (mln_string_t *exp, time_t base), (exp, base), {
    struct utctime u;
    mln_string_t fields[5];
    long week;
    mln_u8_t buf[256];
    mln_u8ptr_t p, fend, fstart;
    int fi;

    size_t blen = exp->len;
    if (blen == 0 || blen >= sizeof(buf)) return 0;

    /* Copy to stack buffer to avoid heap allocation from mln_string_slice */
    memcpy(buf, exp->data, blen);
    buf[blen] = 0;

    /* Handle predefined macros (@yearly, @monthly, @weekly, @daily, @hourly) */
    if (buf[0] == '@') {
        const char *macro = NULL;
        mln_u8ptr_t q;
        for (q = buf + 1; *q && *q != ' ' && *q != '\t'; q++)
            if (*q >= 'A' && *q <= 'Z') *q |= 0x20;
        *q = 0;
        if (!strcmp((char *)buf, "@yearly") || !strcmp((char *)buf, "@annually"))
            macro = "0 0 1 1 *";
        else if (!strcmp((char *)buf, "@monthly"))
            macro = "0 0 1 * *";
        else if (!strcmp((char *)buf, "@weekly"))
            macro = "0 0 * * 0";
        else if (!strcmp((char *)buf, "@daily") || !strcmp((char *)buf, "@midnight"))
            macro = "0 0 * * *";
        else if (!strcmp((char *)buf, "@hourly"))
            macro = "0 * * * *";
        else
            return 0;
        blen = strlen(macro);
        memcpy(buf, macro, blen);
        buf[blen] = 0;
    }

    /* Split into 5 fields by whitespace, NUL-terminate each */
    p = buf;
    fend = buf + blen;
    fi = 0;
    while (fi < 5 && p < fend) {
        while (p < fend && (*p == ' ' || *p == '\t')) p++;
        if (p >= fend) break;
        fstart = p;
        while (p < fend && *p != ' ' && *p != '\t') p++;
        fields[fi].data = fstart;
        fields[fi].len = p - fstart;
        fi++;
    }
    if (fi != 5) return 0;
    for (fi = 0; fi < 5; fi++)
        fields[fi].data[fields[fi].len] = 0;

    /* Replace month names (JAN-DEC) and weekday names (SUN-SAT) with numbers */
    mln_cron_replace_names(&fields[3], mln_cron_month_names, 13);
    mln_cron_replace_names(&fields[4], mln_cron_week_names, 7);

    mln_cron_time2utc(base, &u);
    {
        struct utctime orig = u;

        if ((u.minute = mln_cron_parse_minute(&fields[0], u.minute)) < 0)
            return 0;
        mln_cron_utc_adjust(&u);
        if ((u.hour = mln_cron_parse_hour(&fields[1], u.hour, mln_cron_utc_gt(&u, &orig))) < 0)
            return 0;
        mln_cron_utc_adjust(&u);
        if ((u.day = mln_cron_parse_day(&fields[2], u.year, u.month, u.day, mln_cron_utc_gt(&u, &orig))) < 0)
            return 0;
        mln_cron_utc_adjust(&u);
        if ((u.month = mln_cron_parse_month(&fields[3], u.month, mln_cron_utc_gt(&u, &orig))) < 0)
            return 0;
        mln_cron_utc_adjust(&u);
        if ((week = mln_cron_parse_week(&fields[4], u.week, mln_cron_utc_gt(&u, &orig))) < 0)
            return 0;
        u.day += (week - u.week);
        mln_cron_utc_adjust(&u);
    }

    return mln_cron_utc2time(&u);
})

MLN_FUNC(static, long, mln_cron_parse_minute, (mln_string_t *smin, long min), (smin, min), {
    return mln_cron_parse_field(smin, min, 0, 60, 60, 2, 1, 0);
})

MLN_FUNC(static, long, mln_cron_parse_hour, \
         (mln_string_t *shour, long hour, int greater), \
         (shour, hour, greater), \
{
    return mln_cron_parse_field(shour, hour, 0, 24, 24, 2, 0, greater);
})

MLN_FUNC(static, long, mln_cron_parse_day, \
         (mln_string_t *sday, long year, long month, long day, int greater), \
         (sday, year, month, day, greater), \
{
    return mln_cron_parse_field(sday, day, 1, 32, mln_cron_mon_days[mln_cron_is_leap(year)][month - 1], 2, 0, greater);
})

MLN_FUNC(static, long, mln_cron_parse_month, \
         (mln_string_t *smon, long mon, int greater), \
         (smon, mon, greater), \
{
    return mln_cron_parse_field(smon, mon, 1, 13, 12, 2, 0, greater);
})

MLN_FUNC(static, long, mln_cron_parse_week, \
         (mln_string_t *sweek, long week, int greater), \
         (sweek, week, greater), \
{
    return mln_cron_parse_field(sweek, week, 0, 7, 7, 1, 0, greater);
})
