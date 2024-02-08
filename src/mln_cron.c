
/*
 * Copyright (C) Niklaus F.Schen.
 */

#include "mln_cron.h"
#include "mln_tools.h"
#include "mln_func.h"

static long mln_cron_parse_minute(mln_string_t *smin, long min);
static long mln_cron_parse_hour(mln_string_t *shour, long hour, int greater);
static long mln_cron_parse_day(mln_string_t *sday, long year, long month, long day, int greater);
static long mln_cron_parse_month(mln_string_t *smon, long mon, int greater);
static long mln_cron_parse_week(mln_string_t *sweek, long week, int greater);

MLN_FUNC(, time_t, mln_cron_parse, (mln_string_t *exp, time_t base), (exp, base), {
    struct utctime u;
    mln_string_t *arr;
    time_t tmp;
    long week;

    mln_time2utc(base, &u);
    /*printf("%lu-%lu-%lu %lu:%lu:%lu %lu\n", u.year, u.month, u.day, u.hour, u.minute, u.second, u.week);*/
    if ((arr = mln_string_slice(exp, " \t")) == NULL) {
        return 0;
    }

    if ((u.minute = mln_cron_parse_minute(&arr[0], u.minute)) < 0) {
        mln_string_slice_free(arr);
        return 0;
    }
    mln_utc_adjust(&u);
    tmp = mln_utc2time(&u);
    if ((u.hour = mln_cron_parse_hour(&arr[1], u.hour, tmp > base)) < 0) {
        mln_string_slice_free(arr);
        return 0;
    }
    mln_utc_adjust(&u);
    tmp = mln_utc2time(&u);
    if ((u.day = mln_cron_parse_day(&arr[2], u.year, u.month, u.day, tmp > base)) < 0) {
        mln_string_slice_free(arr);
        return 0;
    }
    mln_utc_adjust(&u);
    tmp = mln_utc2time(&u);
    if ((u.month = mln_cron_parse_month(&arr[3], u.month, tmp > base)) < 0) {
        mln_string_slice_free(arr);
        return 0;
    }
    mln_utc_adjust(&u);
    tmp = mln_utc2time(&u);
    if ((week = mln_cron_parse_week(&arr[4], u.week, tmp > base)) < 0) {
        mln_string_slice_free(arr);
        return 0;
    }
    u.day += (week - u.week);
    mln_utc_adjust(&u);

    mln_string_slice_free(arr);
    /*printf("%lu-%lu-%lu %lu:%lu:%lu %lu\n", u.year, u.month, u.day, u.hour, u.minute, u.second, u.week);*/
    return mln_utc2time(&u);
})

MLN_FUNC(static, long, mln_cron_parse_minute, (mln_string_t *smin, long min), (smin, min), {
    if (!smin->len) return -1;
    if (smin->len == 1 && smin->data[0] == '*')
        return min + 1;

    long period = 0, save = 0, tmp;
    mln_u8ptr_t head = smin->data, p = smin->data, end = smin->data + smin->len - 1;

    for (; end >= p; --end) {
        if (*end == '/') {
            period = atol((char *)(end+1));
            if (period < 1 || period >= 60) return -1;
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
                if (tmp < 1 || tmp >= 60) {
                    return -1;
                }
                if (tmp < min) tmp += 60;
                if (!save) {
                    save = min == tmp? min + period: tmp;
                } else {
                    if ((tmp == min && period < save - min) || (tmp - min < save - min))
                        save = min == tmp? min + period: tmp;
                }
		head = ++p;
                break;
            case '*':
                save = min + (period? period: 1);
                if (*(++p) == ',') head = ++p;
                else head = p;
                break;
            default:
                if (!mln_isdigit(*p++)) return -1;
                break;
        }
    }
    if (p > head) {
        if (p > head + 2) return -1;
        tmp = atol((char *)head);
        if (tmp < 1 || tmp >= 60) return -1;
        if (tmp < min) tmp += 60;
        if (!save) {
            save = min == tmp? min + period: tmp;
        } else {
            if ((tmp == min && period < save - min) || (tmp - min < save - min))
                save = min == tmp? min + period: tmp;
        }
    }
    return save;
})

MLN_FUNC(static, long, mln_cron_parse_hour, \
         (mln_string_t *shour, long hour, int greater), \
         (shour, hour, greater), \
{
    if (!shour->len) return -1;
    if (shour->len == 1 && shour->data[0] == '*')
        return hour;

    long period = 0, save = 0, tmp;
    mln_u8ptr_t head = shour->data, p = shour->data, end = shour->data + shour->len;

    for (--end; end >= p; --end) {
        if (*end == '/') {
            period = atol((char *)(end+1));
            if (period < 1 || period >= 24) return -1;
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
                if (tmp < 1 || tmp >= 24) {
                    return -1;
                }
                if (tmp < hour) tmp += 24;
                if (!save) {
                    save = hour==tmp && !greater? hour + period: tmp;
                } else {
                    if (tmp == hour) {
                        if (greater) save = tmp;
                        else if (period < save - hour) save = hour + period;
                    } else if (tmp - hour < save - hour) {
                        save = tmp;
                    }
                }
		head = ++p;
                break;
            case '*':
                save = greater? hour: (hour + (period? period: 1));
                if (*(++p) == ',') head = ++p;
                else head = p;
                break;
            default:
                if (!mln_isdigit(*p++)) return -1;
                break;
        }
    }
    if (p > head) {
        if (p > head + 2) return -1;
        tmp = atol((char *)head);
        if (tmp < 1 || tmp >= 24) return -1;
        if (tmp < hour) tmp += 24;
        if (!save) {
            save = hour==tmp && !greater? hour + period: tmp;
        } else {
            if (tmp == hour) {
                if (greater) save = tmp;
                else if (period < save - hour) save = hour + period;
            } else if (tmp - hour < save - hour) {
                save = tmp;
            }
        }
    }
    return save;
})

MLN_FUNC(static, long, mln_cron_parse_day, \
         (mln_string_t *sday, long year, long month, long day, int greater), \
         (sday, year, month, day, greater), \
{
    if (!sday->len) return -1;
    if (sday->len == 1 && sday->data[0] == '*')
        return day;

    long period = 0, save = 0, tmp;
    mln_u8ptr_t head = sday->data, p = sday->data, end = sday->data + sday->len - 1;

    for (; end >= p; --end) {
        if (*end == '/') {
            period = atol((char *)(end+1));
            if (period < 1 || period >= 32) return -1;
            *end = 0;
            break;
        }
    }
    if (end < p) end = sday->data + sday->len;

    while (p < end) {
        switch (*p) {
            case ',':
                if (p == head || p > head + 2) return -1;
                *p = 0;
                tmp = atol((char *)head);
                if (tmp < 1 || tmp >= 32) {
                    return -1;
                }
                if (tmp < day) tmp += mln_month_days(year, month);
                if (!save) {
                    save = day==tmp && !greater? day + period: tmp;
                } else {
                    if (tmp == day) {
                        if (greater) save = tmp;
                        else if (period < save - day) save = day + period;
                    } else if (tmp - day < save - day) {
                        save = tmp;
                    }
                }
		head = ++p;
                break;
            case '*':
                save = greater? day: (day + (period? period: 1));
                if (*(++p) == ',') head = ++p;
                else head = p;
                break;
            default:
                if (!mln_isdigit(*p++)) return -1;
                break;
        }
    }
    if (p > head) {
        if (p > head + 2) return -1;
        tmp = atol((char *)head);
        if (tmp < 1 || tmp >= 32) return -1;
        if (tmp < day) tmp += mln_month_days(year, month);
        if (!save) {
            save = day==tmp && !greater? day + period: tmp;
        } else {
            if (tmp == day) {
                if (greater) save = tmp;
                else if (period < save - day) save = day + period;
            } else if (tmp - day < save - day) {
                save = tmp;
            }
        }
    }
    return save;
})

MLN_FUNC(static, long, mln_cron_parse_month, \
         (mln_string_t *smon, long mon, int greater), \
         (smon, mon, greater), \
{
    if (!smon->len) return -1;
    if (smon->len == 1 && smon->data[0] == '*')
        return mon;

    long period = 0, save = 0, tmp;
    mln_u8ptr_t head = smon->data, p = smon->data, end = smon->data + smon->len;

    for (--end; end >= p; --end) {
        if (*end == '/') {
            period = atol((char *)(end+1));
            if (period < 1 || period >= 13) return -1;
            *end = 0;
            break;
        }
    }
    if (end < p) end = smon->data + smon->len;

    while (p < end) {
        switch (*p) {
            case ',':
                if (p == head || p > head + 2) return -1;
                *p = 0;
                tmp = atol((char *)head);
                if (tmp < 1 || tmp >= 13) {
                    return -1;
                }
                if (tmp < mon) tmp += 12;
                if (!save) {
                    save = mon==tmp && !greater? mon + period: tmp;
                } else {
                    if (tmp == mon) {
                        if (greater) save = tmp;
                        else if (period < save - mon) save = mon + period;
                    } else if (tmp - mon < save - mon) {
                        save = tmp;
                    }
                }
		head = ++p;
                break;
            case '*':
                save = greater? mon: (mon + (period? period: 1));
                if (*(++p) == ',') head = ++p;
                else head = p;
                break;
            default:
                if (!mln_isdigit(*p++)) return -1;
                break;
        }
    }
    if (p > head) {
        if (p > head + 2) return -1;
        tmp = atol((char *)head);
        if (tmp < 1 || tmp >= 13) return -1;
        if (tmp < mon) tmp += 12;
        if (!save) {
            save = mon==tmp && !greater? mon + period: tmp;
        } else {
            if (tmp == mon) {
                if (greater) save = tmp;
                else if (period < save - mon) save = mon + period;
            } else if (tmp - mon < save - mon) {
                save = tmp;
            }
        }
    }
    return save;
})

MLN_FUNC(static, long, mln_cron_parse_week, \
         (mln_string_t *sweek, long week, int greater), \
         (sweek, week, greater), \
{
    if (!sweek->len) return -1;
    if (sweek->len == 1 && sweek->data[0] == '*')
        return week;

    long period = 0, save = 0, tmp;
    mln_u8ptr_t head = sweek->data, p = sweek->data, end = sweek->data + sweek->len;

    for (--end; end >= p; --end) {
        if (*end == '/') {
            period = atol((char *)(end+1));
            if (period < 0 || period >= 7) return -1;
            *end = 0;
            break;
        }
    }
    if (end < p) end = sweek->data + sweek->len;

    while (p < end) {
        switch (*p) {
            case ',':
                if (p == head || p > head + 1) return -1;
                *p = 0;
                tmp = atol((char *)head);
                if (tmp < 0 || tmp >= 7) {
                    return -1;
                }
                if (tmp < week) tmp += 7;
                if (!save) {
                    save = week==tmp && !greater? week + period: tmp;
                } else {
                    if (tmp == week) {
                        if (greater) save = tmp;
                        else if (period < save - week) save = week + period;
                    } else if (tmp - week < save - week) {
                        save = tmp;
                    }
                }
		head = ++p;
                break;
            case '*':
                save = greater? week: (week + (period? period: 1));
                if (*(++p) == ',') head = ++p;
                else head = p;
                break;
            default:
                if (!mln_isdigit(*p++)) return -1;
                break;
        }
    }
    if (p > head) {
        if (p > head + 1) return -1;
        tmp = atol((char *)head);
        if (tmp < 0 || tmp >= 7) return -1;
        if (tmp < week) tmp += 7;
        if (!save) {
            save = week==tmp && !greater? week + period: tmp;
        } else {
            if (tmp == week) {
                if (greater) save = tmp;
                else if (period < save - week) save = week + period;
            } else if (tmp - week < save - week) {
                save = tmp;
            }
        }
    }
    return save;
})

