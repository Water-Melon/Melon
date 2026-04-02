#include "mln_cron.h"
#include "mln_tools.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

static int n_pass = 0, n_fail = 0;

/*
 * Helper: parse cron expression with given base time and verify the result
 * matches the expected UTC components.
 */
static void check_cron(const char *label, const char *expr, time_t base,
                        int exp_year, int exp_month, int exp_day,
                        int exp_hour, int exp_minute)
{
    char buf[256];
    int len = snprintf(buf, sizeof(buf), "%s", expr);
    mln_string_t s;
    mln_string_nset(&s, buf, len);

    time_t result = mln_cron_parse(&s, base);

    struct utctime u;
    mln_time2utc(result, &u);

    if (u.year == exp_year && u.month == exp_month && u.day == exp_day &&
        u.hour == exp_hour && u.minute == exp_minute) {
        n_pass++;
    } else {
        n_fail++;
        fprintf(stderr, "FAIL [%s] \"%s\" base=%lu\n", label, expr, (unsigned long)base);
        fprintf(stderr, "  expected: %d-%02d-%02d %02d:%02d\n",
                exp_year, exp_month, exp_day, exp_hour, exp_minute);
        fprintf(stderr, "  got:      %ld-%02ld-%02ld %02ld:%02ld (time_t=%lu)\n",
                u.year, u.month, u.day, u.hour, u.minute, (unsigned long)result);
    }
}

/* Helper: verify that a cron expression returns 0 (error). */
static void check_cron_error(const char *label, const char *expr, time_t base)
{
    char buf[256];
    int len = snprintf(buf, sizeof(buf), "%s", expr);
    mln_string_t s;
    mln_string_nset(&s, buf, len);

    time_t result = mln_cron_parse(&s, base);
    if (result == 0) {
        n_pass++;
    } else {
        n_fail++;
        fprintf(stderr, "FAIL [%s] \"%s\" expected error (0), got %lu\n",
                label, expr, (unsigned long)result);
    }
}

int main(void)
{
    /*
     * Use a fixed base time for deterministic tests.
     * 2025-04-02 08:30:00 UTC  (Wednesday, week=3)
     *
     * We compute this from a known epoch value.
     * Let's use mln_utc2time to build it.
     */
    struct utctime base_u;
    memset(&base_u, 0, sizeof(base_u));
    base_u.year = 2025;
    base_u.month = 4;
    base_u.day = 2;
    base_u.hour = 8;
    base_u.minute = 30;
    base_u.second = 0;
    base_u.week = 3; /* Wednesday */
    time_t base = mln_utc2time(&base_u);

    printf("Base time: %lu (2025-04-02 08:30:00 UTC, Wed)\n", (unsigned long)base);

    /* ====== 1. Wildcard ====== */
    /* "* * * * *" => next minute: 08:31 same day */
    check_cron("wildcard all", "* * * * *", base,
               2025, 4, 2, 8, 31);

    /* ====== 2. Specific values ====== */
    /* "45 * * * *" => 08:45 same day (45 > 30, same hour) */
    check_cron("specific minute future", "45 * * * *", base,
               2025, 4, 2, 8, 45);

    /* "15 * * * *" => 09:15 same day (15 < 30, so next hour) */
    check_cron("specific minute past", "15 * * * *", base,
               2025, 4, 2, 9, 15);

    /* "0 12 * * *" => 12:00 same day */
    check_cron("specific hour", "0 12 * * *", base,
               2025, 4, 2, 12, 0);

    /* "0 3 * * *" => 03:00 next day (3 < 8) */
    check_cron("specific hour past", "0 3 * * *", base,
               2025, 4, 3, 3, 0);

    /* "0 0 15 * *" => 00:00 on the 15th of this month */
    check_cron("specific day", "0 0 15 * *", base,
               2025, 4, 15, 0, 0);

    /* "0 0 1 * *" => 00:00 on the 1st of next month (1 < 2) */
    check_cron("specific day past", "0 0 1 * *", base,
               2025, 5, 1, 0, 0);

    /* "0 0 1 1 *" => 00:00 Jan 1 next year (month 1 < 4) */
    check_cron("specific month past", "0 0 1 1 *", base,
               2026, 1, 1, 0, 0);

    /* "0 0 1 12 *" => 00:00 Dec 1 this year */
    check_cron("specific month future", "0 0 1 12 *", base,
               2025, 12, 1, 0, 0);

    /* ====== 3. Comma-separated lists ====== */
    /* "0,15,30,45 * * * *" => 30 matches current minute exactly => 08:30 */
    check_cron("comma minute", "0,15,30,45 * * * *", base,
               2025, 4, 2, 8, 30);

    /* "5,10 * * * *" => both < 30, so next hour: 09:05 */
    check_cron("comma minute all past", "5,10 * * * *", base,
               2025, 4, 2, 9, 5);

    /* "* 6,12,18 * * *" => hour 12 is nearest future (12 > 8), minute 31 */
    check_cron("comma hour", "* 6,12,18 * * *", base,
               2025, 4, 2, 12, 31);

    /* ====== 4. Ranges (NEW) ====== */
    /* "1-10 * * * *" => range 1-10, all < 30, next hour: 09:01 */
    check_cron("range minute all past", "1-10 * * * *", base,
               2025, 4, 2, 9, 1);

    /* "25-45 * * * *" => range 25-45, 30 is in range and matches current => 08:30 */
    check_cron("range minute spanning", "25-45 * * * *", base,
               2025, 4, 2, 8, 30);

    /* "40-50 * * * *" => range 40-50, all > 30, nearest is 40 at 08:40 */
    check_cron("range minute future", "40-50 * * * *", base,
               2025, 4, 2, 8, 40);

    /* "* 10-14 * * *" => hours 10-14, nearest > 8 is 10, minute 31 */
    check_cron("range hour", "* 10-14 * * *", base,
               2025, 4, 2, 10, 31);

    /* "* * 5-10 * *" => days 5-10, nearest > 2 is 5 */
    check_cron("range day", "* * 5-10 * *", base,
               2025, 4, 5, 8, 31);

    /* "* * * 6-9 *" => months 6-9, nearest > 4 is 6 */
    check_cron("range month", "* * * 6-9 *", base,
               2025, 6, 2, 8, 31);

    /* "* * * * 5-6" => week 5(Fri)-6(Sat), nearest > 3(Wed) is 5(Fri) = day+2 */
    check_cron("range week", "* * * * 5-6", base,
               2025, 4, 4, 8, 31);

    /* ====== 5. Mixed ranges and values ====== */
    /* "1-5,40-50 * * * *" => 1-5 all < 30, 40-50 all > 30, nearest is 40 */
    check_cron("mixed range+value minute", "1-5,40-50 * * * *", base,
               2025, 4, 2, 8, 40);

    /* "1-5,10,45 * * * *" => 1-5 and 10 all < 30, 45 > 30, nearest is 45 */
    check_cron("mixed list+range minute", "1-5,10,45 * * * *", base,
               2025, 4, 2, 8, 45);

    /* ====== 6. Step/period ====== */
    /* star/5 => wildcard with step 5: cur(30) + 5 = 35 */
    check_cron("step wildcard minute", "*/5 * * * *", base,
               2025, 4, 2, 8, 35);

    /* star/15 => cur(30) + 15 = 45 */
    check_cron("step wildcard minute 15", "*/15 * * * *", base,
               2025, 4, 2, 8, 45);

    /* hour star/6 => minute advances to 31 (greater=1), so hour stays at 8 */
    check_cron("step wildcard hour", "* */6 * * *", base,
               2025, 4, 2, 8, 31);

    /* ====== 7. Range with step ====== */
    /* "10-50/5 * * * *" => range 10-50 with global period 5.
     * Values: 10..50. Nearest > 30 is 31, but 31==cur+period? No: period=5.
     * Actually 31 is in range and 31>30, so save=31. Then 32>30 but 32-30>31-30.
     * Wait: nearest to 30 in {10..50} where val>30 is 31. */
    check_cron("range with step", "10-50/5 * * * *", base,
               2025, 4, 2, 8, 31);

    /* ====== 8. Wrap-around range ====== */
    /* "50-10 * * * *" => values 50..59,0..10. All < 30 or > 30.
     * > 30: {50,51,...,59}. Nearest is 50. */
    check_cron("wrap range minute", "50-10 * * * *", base,
               2025, 4, 2, 8, 50);

    /* "* 22-3 * * *" => hours {22,23,0,1,2,3}. Nearest > 8 is 22. */
    check_cron("wrap range hour", "* 22-3 * * *", base,
               2025, 4, 2, 22, 31);

    /* "* * * * 5-1" => week {5,6,0,1}. cur=3(Wed). Nearest > 3 is 5(Fri) = day+2. */
    check_cron("wrap range week", "* * * * 5-1", base,
               2025, 4, 4, 8, 31);

    /* ====== 9. Minute value 0 ====== */
    /* "0 * * * *" => minute 0, which is < 30, so next hour 09:00 */
    check_cron("minute zero", "0 * * * *", base,
               2025, 4, 2, 9, 0);

    /* "0 0 * * *" => 00:00 next day */
    check_cron("midnight", "0 0 * * *", base,
               2025, 4, 3, 0, 0);

    /* ====== 10. Week day 7 (alias for 0=Sunday) ====== */
    /* "* * * * 7" => Sunday. cur=Wed(3). Sunday(0) < 3, wrap: 0+7=7, day+(7-3)=day+4 */
    check_cron("week 7 alias", "* * * * 7", base,
               2025, 4, 6, 8, 31);

    /* "* * * * 0" => Sunday same as above */
    check_cron("week 0 sunday", "* * * * 0", base,
               2025, 4, 6, 8, 31);

    /* ====== 11. Realistic expressions ====== */
    /* "0 9 * * 1" => Monday 9:00. cur=Wed. Mon(1)<Wed(3), next Mon = day+(1+7-3)=day+5 = Apr 7 */
    check_cron("monday 9am", "0 9 * * 1", base,
               2025, 4, 7, 9, 0);

    /* "30 8 * * 3" => Wednesday 8:30. cur is Wed 8:30.
     * minute: 30 == 30, period=0, save = 30+0 = 30.
     * hour: 8, tmp==base so not greater. 8==8, not greater, period=0 => save=8+0=8.
     * week: 3==3, not greater => save=3+0=3. day stays.
     * Result should be the same base time. */
    check_cron("exact match", "30 8 * * 3", base,
               2025, 4, 2, 8, 30);

    /* ====== 12. Month names (JAN-DEC) ====== */
    /* "0 0 1 JAN *" same as "0 0 1 1 *" => Jan 1 next year */
    check_cron("month name JAN", "0 0 1 JAN *", base,
               2026, 1, 1, 0, 0);

    /* "0 0 1 dec *" lowercase => Dec 1 this year */
    check_cron("month name dec lower", "0 0 1 dec *", base,
               2025, 12, 1, 0, 0);

    /* "* * * JUN-SEP *" same as "* * * 6-9 *" => month 6 nearest > 4 */
    check_cron("month name range", "* * * JUN-SEP *", base,
               2025, 6, 2, 8, 31);

    /* "0 0 1 JAN,JUN *" => JAN(1) < 4, JUN(6) > 4, nearest is June */
    check_cron("month name comma", "0 0 1 JAN,JUN *", base,
               2025, 6, 1, 0, 0);

    /* ====== 13. Weekday names (SUN-SAT) ====== */
    /* "0 9 * * MON" same as "0 9 * * 1" => Monday 9:00 */
    check_cron("week name MON", "0 9 * * MON", base,
               2025, 4, 7, 9, 0);

    /* "* * * * sun" lowercase => Sunday */
    check_cron("week name sun lower", "* * * * sun", base,
               2025, 4, 6, 8, 31);

    /* "* * * * MON-FRI" => Wed(3) in range, minute advances, stays same day */
    check_cron("week name range", "* * * * MON-FRI", base,
               2025, 4, 2, 8, 31);

    /* ====== 14. Predefined macros ====== */
    /* "@yearly" same as "0 0 1 1 *" => Jan 1 next year */
    check_cron("macro yearly", "@yearly", base,
               2026, 1, 1, 0, 0);

    /* "@monthly" same as "0 0 1 * *" => May 1 (1 < 2, next month) */
    check_cron("macro monthly", "@monthly", base,
               2025, 5, 1, 0, 0);

    /* "@weekly" same as "0 0 * * 0" => Sunday 00:00 */
    check_cron("macro weekly", "@weekly", base,
               2025, 4, 6, 0, 0);

    /* "@daily" same as "0 0 * * *" => midnight next day */
    check_cron("macro daily", "@daily", base,
               2025, 4, 3, 0, 0);

    /* "@hourly" same as "0 * * * *" => next hour :00 */
    check_cron("macro hourly", "@hourly", base,
               2025, 4, 2, 9, 0);

    /* "@annually" alias for @yearly */
    check_cron("macro annually", "@annually", base,
               2026, 1, 1, 0, 0);

    /* "@DAILY" uppercase macro */
    check_cron("macro DAILY upper", "@DAILY", base,
               2025, 4, 3, 0, 0);

    /* ====== 15. Error cases ====== */
    check_cron_error("empty", "", base);
    check_cron_error("too few fields", "* * *", base);
    check_cron_error("invalid minute", "60 * * * *", base);
    check_cron_error("invalid hour", "* 25 * * *", base);
    check_cron_error("invalid day", "* * 32 * *", base);
    check_cron_error("invalid month", "* * * 13 *", base);
    check_cron_error("invalid week", "* * * * 8", base);
    check_cron_error("bad step minute", "*/0 * * * *", base);
    check_cron_error("bad step too large", "*/60 * * * *", base);
    check_cron_error("unterminated range", "5- * * * *", base);
    check_cron_error("bad char", "abc * * * *", base);
    check_cron_error("bad macro", "@foobar", base);

    /* ====== Summary ====== */
    printf("\n====== Cron Test Summary ======\n");
    printf("Passed: %d\n", n_pass);
    printf("Failed: %d\n", n_fail);

    return n_fail ? 1 : 0;
}
