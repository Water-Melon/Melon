
/*
 * Copyright (C) Niklaus F.Schen.
 */

#include <ctype.h>
#include "mln_cron.h"
#include "mln_tools.h"

static long mln_cron_parse_minute(mln_string_t *smin, long min);
static long mln_cron_parse_hour(mln_string_t *shour, long hour);
static long mln_cron_parse_day(mln_string_t *sday, long day);
static long mln_cron_parse_month(mln_string_t *smon, long mon);
static long mln_cron_parse_week(mln_string_t *sweek, long week);

time_t mln_cron_parse(mln_string_t *exp, time_t base)
{
    struct utctime u;
    mln_string_t *arr;

    mln_time2utc(base, &u);
    if ((arr = mln_string_slice(exp, " \t")) == NULL) {
        return 0;
    }

    if ((u.minute = mln_cron_parse_minute(&arr[0], u.minute)) < 0) {
        mln_string_slice_free(arr);
        return 0;
    }
    mln_utc_adjust(&u);
    if ((u.hour = mln_cron_parse_hour(&arr[1], u.hour)) < 0) {
        mln_string_slice_free(arr);
        return 0;
    }
    mln_utc_adjust(&u);
    if ((u.day = mln_cron_parse_day(&arr[2], u.day)) < 0) {
        mln_string_slice_free(arr);
        return 0;
    }
    mln_utc_adjust(&u);
    if ((u.month = mln_cron_parse_month(&arr[3], u.month)) < 0) {
        mln_string_slice_free(arr);
        return 0;
    }
    mln_utc_adjust(&u);
    if ((u.week = mln_cron_parse_week(&arr[4], u.week)) < 0) {
        mln_string_slice_free(arr);
        return 0;
    }
    mln_utc_adjust(&u);

    mln_string_slice_free(arr);
    return mln_utc2time(&u);
}

static long mln_cron_parse_minute(mln_string_t *smin, long min)
{
    if (smin->data == NULL) return -1;
    if (smin->len == 1 && smin->data[0] == '*')
        return min + 1;

    long period = 0, save = 0, tmp;
    mln_u8ptr_t head = smin->data, p = smin->data, end = smin->data + smin->len - 1;

    for (; end >= p; --end) {
        if (*end == '/') {
            period = atol((char *)(end+1));
            if (period < 0 || period >= 60) return -1;
            *end = 0;
            break;
        }
    }
    if (end < p) end = smin->data + smin->len;

    while (p < end) {
        switch (*p) {
            case ',':
                if (p == head || p > head + 2) return -1;
                *p = 0;
                tmp = atol((char *)head);
                if (tmp < 0 || tmp >= 60) {
                    return -1;
                }
                if (tmp < min) tmp += 60;
                if (!save) {
                    save = min == tmp? min + period: tmp;
                } else {
                    if ((tmp == min && min + period < save - min) || (tmp - min < save - min))
                        save = min == tmp? min + period: tmp;
                }
		head = ++p;
                break;
            case '*':
                save = min + (period? period: 1);
                if (*(++p) == ',') head = ++p;
                break;
            default:
                if (!isdigit(*p++)) return -1;
                break;
        }
    }
    if (p > head) {
        tmp = atol((char *)head);
        if (tmp < 0 || tmp >= 60) return -1;
        if (tmp < min) tmp += 60;
        if (!save) {
            save = min == tmp? min + period: tmp;
        } else {
            if ((tmp == min && min + period < save - min) || (tmp - min < save - min))
                save = min == tmp? min + period: tmp;
        }
    }
    return save;
}

static long mln_cron_parse_hour(mln_string_t *shour, long hour)
{
    if (shour->data == NULL) return -1;
    if (shour->len == 1 && shour->data[0] == '*')
        return hour;

    long period = 0, save = 0, tmp;
    mln_u8ptr_t head = shour->data, p = shour->data, end = shour->data + shour->len;

    for (--end; end >= p; --end) {
        if (*end == '/') {
            period = atol((char *)(end+1));
            if (period < 0 || period >= 24) return -1;
            *end = 0;
            break;
        }
    }
    if (end < p) end = shour->data + shour->len;

    while (p < end) {
        switch (*p) {
            case ',':
                if (p == head || p > head + 2) return -1;
                *p = 0;
                tmp = atol((char *)head);
                if (tmp < 0 || tmp >= 24) {
                    return -1;
                }
                if (tmp < hour) tmp += 24;
                if (!save) {
                    save = hour == tmp? hour + period: tmp;
                } else {
                    if ((tmp == hour && hour + period < save - hour) || (tmp - hour < save - hour))
                        save = hour == tmp? hour + period: tmp;
                }
		head = ++p;
                break;
            case '*':
                save = hour + (period? period: 1);
                if (*(++p) == ',') head = ++p;
                break;
            default:
                if (!isdigit(*p++)) return -1;
                ++p;
                break;
        }
    }
    if (p > head) {
        tmp = atol((char *)head);
        if (tmp < 0 || tmp >= 24) return -1;
        if (tmp < hour) tmp += 24;
        if (!save) {
            save = hour == tmp? hour + period: tmp;
        } else {
            if ((tmp == hour && hour + period < save - hour) || (tmp - hour < save - hour))
                save = hour == tmp? hour + period: tmp;
        }
    }
    return save;
}

static long mln_cron_parse_day(mln_string_t *sday, long day)
{
    if (sday->data == NULL) return -1;
    if (sday->len == 1 && sday->data[0] == '*')
        return day;

    return day;
}

static long mln_cron_parse_month(mln_string_t *smon, long mon)
{
    if (smon->data == NULL) return -1;
    if (smon->len == 1 && smon->data[0] == '*')
        return mon;

    return mon;
}

static long mln_cron_parse_week(mln_string_t *sweek, long week)
{
    if (sweek->data == NULL) return -1;
    if (sweek->len == 1 && sweek->data[0] == '*')
        return week;

    return week;
}

