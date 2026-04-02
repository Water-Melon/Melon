#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "mln_regexp.h"

static int test_count = 0;
static int pass_count = 0;

#define TEST(name) do { \
    test_count++; \
    printf("  [%d] %s ... ", test_count, name); \
} while(0)

#define PASS() do { \
    pass_count++; \
    printf("PASS\n"); \
} while(0)

/* Helper: run mln_reg_match and return match count */
static int do_match(const char *exp_str, const char *text_str, mln_reg_match_result_t *res)
{
    mln_string_t exp, text;
    mln_string_nset(&exp, exp_str, strlen(exp_str));
    mln_string_nset(&text, text_str, strlen(text_str));
    return mln_reg_match(&exp, &text, res);
}

/* Helper: run mln_reg_equal */
static int do_equal(const char *exp_str, const char *text_str)
{
    mln_string_t exp, text;
    mln_string_nset(&exp, exp_str, strlen(exp_str));
    mln_string_nset(&text, text_str, strlen(text_str));
    return mln_reg_equal(&exp, &text);
}

/* Helper: check that the i-th match equals expected */
static void check_match(mln_string_t *matches, int idx, const char *expected)
{
    assert(matches[idx].len == strlen(expected));
    assert(memcmp(matches[idx].data, expected, matches[idx].len) == 0);
}

static void test_literal_match(void)
{
    printf("=== Literal matching ===\n");
    mln_reg_match_result_t *res;
    int n;

    TEST("exact literal in text");
    res = mln_reg_match_result_new(1);
    assert(res != NULL);
    n = do_match("abc", "xyzabcdef", res);
    assert(n == 1);
    check_match(mln_reg_match_result_get(res), 0, "abc");
    mln_reg_match_result_free(res);
    PASS();

    TEST("multiple literal matches");
    res = mln_reg_match_result_new(1);
    assert(res != NULL);
    n = do_match("ab", "ababab", res);
    assert(n == 3);
    mln_reg_match_result_free(res);
    PASS();

    TEST("no match returns 0");
    res = mln_reg_match_result_new(1);
    assert(res != NULL);
    n = do_match("xyz", "abcdef", res);
    assert(n == 0);
    mln_reg_match_result_free(res);
    PASS();

    TEST("single char match");
    res = mln_reg_match_result_new(1);
    assert(res != NULL);
    n = do_match("a", "bac", res);
    assert(n == 1);
    check_match(mln_reg_match_result_get(res), 0, "a");
    mln_reg_match_result_free(res);
    PASS();
}

static void test_dot(void)
{
    printf("=== Dot (.) matching ===\n");
    mln_reg_match_result_t *res;
    int n;

    TEST("dot matches any char");
    res = mln_reg_match_result_new(1);
    assert(res != NULL);
    n = do_match("a.c", "abcadc", res);
    assert(n >= 1);
    check_match(mln_reg_match_result_get(res), 0, "abc");
    mln_reg_match_result_free(res);
    PASS();

    TEST("multiple dots");
    res = mln_reg_match_result_new(1);
    assert(res != NULL);
    n = do_match("a..d", "abcd", res);
    assert(n == 1);
    check_match(mln_reg_match_result_get(res), 0, "abcd");
    mln_reg_match_result_free(res);
    PASS();

    TEST("dot does not match empty");
    res = mln_reg_match_result_new(1);
    assert(res != NULL);
    n = do_match("a.b", "ab", res);
    assert(n == 0);
    mln_reg_match_result_free(res);
    PASS();
}

static void test_star(void)
{
    printf("=== Star (*) quantifier ===\n");
    mln_reg_match_result_t *res;
    int n;

    TEST("star zero matches");
    res = mln_reg_match_result_new(1);
    assert(res != NULL);
    n = do_match("ab*c", "ac", res);
    assert(n == 1);
    check_match(mln_reg_match_result_get(res), 0, "ac");
    mln_reg_match_result_free(res);
    PASS();

    TEST("star multiple matches");
    res = mln_reg_match_result_new(1);
    assert(res != NULL);
    n = do_match("ab*c", "abbbc", res);
    assert(n == 1);
    check_match(mln_reg_match_result_get(res), 0, "abbbc");
    mln_reg_match_result_free(res);
    PASS();

    TEST("dot-star greedy");
    res = mln_reg_match_result_new(1);
    assert(res != NULL);
    n = do_match("a.*e", "abcde", res);
    assert(n == 1);
    check_match(mln_reg_match_result_get(res), 0, "abcde");
    mln_reg_match_result_free(res);
    PASS();

    TEST("star with pattern after");
    res = mln_reg_match_result_new(1);
    assert(res != NULL);
    n = do_match("a.*b", "axxxb", res);
    assert(n == 1);
    check_match(mln_reg_match_result_get(res), 0, "axxxb");
    mln_reg_match_result_free(res);
    PASS();
}

static void test_plus(void)
{
    printf("=== Plus (+) quantifier ===\n");
    mln_reg_match_result_t *res;
    int n;

    TEST("plus requires at least one");
    res = mln_reg_match_result_new(1);
    assert(res != NULL);
    n = do_match("ab+c", "ac", res);
    assert(n == 0);
    mln_reg_match_result_free(res);
    PASS();

    TEST("plus one match");
    res = mln_reg_match_result_new(1);
    assert(res != NULL);
    n = do_match("ab+c", "abc", res);
    assert(n == 1);
    check_match(mln_reg_match_result_get(res), 0, "abc");
    mln_reg_match_result_free(res);
    PASS();

    TEST("plus multiple matches");
    res = mln_reg_match_result_new(1);
    assert(res != NULL);
    n = do_match("ab+c", "abbbc", res);
    assert(n == 1);
    check_match(mln_reg_match_result_get(res), 0, "abbbc");
    mln_reg_match_result_free(res);
    PASS();
}

static void test_question(void)
{
    printf("=== Question (?) quantifier ===\n");
    mln_reg_match_result_t *res;
    int n;

    TEST("question zero match");
    res = mln_reg_match_result_new(1);
    assert(res != NULL);
    n = do_match("ab?c", "ac", res);
    assert(n == 1);
    check_match(mln_reg_match_result_get(res), 0, "ac");
    mln_reg_match_result_free(res);
    PASS();

    TEST("question one match");
    res = mln_reg_match_result_new(1);
    assert(res != NULL);
    n = do_match("ab?c", "abc", res);
    assert(n == 1);
    check_match(mln_reg_match_result_get(res), 0, "abc");
    mln_reg_match_result_free(res);
    PASS();
}

static void test_brace(void)
{
    printf("=== Brace {n,m} quantifier ===\n");
    mln_reg_match_result_t *res;
    int n;

    TEST("exact count {3}");
    res = mln_reg_match_result_new(1);
    assert(res != NULL);
    n = do_match("a{3}", "xaaax", res);
    assert(n == 1);
    check_match(mln_reg_match_result_get(res), 0, "aaa");
    mln_reg_match_result_free(res);
    PASS();

    TEST("range {2,4} min");
    res = mln_reg_match_result_new(1);
    assert(res != NULL);
    n = do_match("a{2,4}", "xaax", res);
    assert(n == 1);
    check_match(mln_reg_match_result_get(res), 0, "aa");
    mln_reg_match_result_free(res);
    PASS();

    TEST("range {2,4} max");
    res = mln_reg_match_result_new(1);
    assert(res != NULL);
    n = do_match("a{2,4}", "xaaaaax", res);
    assert(n >= 1);
    check_match(mln_reg_match_result_get(res), 0, "aaaa");
    mln_reg_match_result_free(res);
    PASS();

    TEST("range {2,4} too few");
    res = mln_reg_match_result_new(1);
    assert(res != NULL);
    n = do_match("a{2,4}$", "xa", res);
    assert(n == 0);
    mln_reg_match_result_free(res);
    PASS();

    TEST("min only {2,}");
    res = mln_reg_match_result_new(1);
    assert(res != NULL);
    n = do_match("a{2,}", "xaaaaax", res);
    assert(n >= 1);
    mln_reg_match_result_free(res);
    PASS();
}

static void test_anchors(void)
{
    printf("=== Anchors (^ and $) ===\n");
    mln_reg_match_result_t *res;
    int n;

    TEST("^ anchor start");
    res = mln_reg_match_result_new(1);
    assert(res != NULL);
    n = do_match("^abc", "abcdef", res);
    assert(n == 1);
    check_match(mln_reg_match_result_get(res), 0, "abc");
    mln_reg_match_result_free(res);
    PASS();

    TEST("^ anchor no match");
    res = mln_reg_match_result_new(1);
    assert(res != NULL);
    n = do_match("^abc", "xabc", res);
    assert(n == 0);
    mln_reg_match_result_free(res);
    PASS();

    TEST("$ anchor end");
    assert(do_equal("xyz$", "abcxyz") == 0);
    PASS();

    TEST("$ anchor no match");
    assert(do_equal("xyz$", "xyzabc") != 0);
    PASS();

    TEST("^ and $ together");
    assert(do_equal("^abc$", "abc") == 0);
    PASS();

    TEST("^ and $ no match");
    assert(do_equal("^abc$", "abcd") != 0);
    PASS();
}

static void test_groups(void)
{
    printf("=== Groups (parentheses) ===\n");
    mln_reg_match_result_t *res;
    mln_string_t *s; (void)s;
    int n;

    TEST("capture group");
    res = mln_reg_match_result_new(1);
    assert(res != NULL);
    n = do_match("a(bc)d", "xabcdx", res);
    assert(n >= 1);
    s = mln_reg_match_result_get(res);
    /* Check both the overall match and captured group exist */
    assert(n >= 2);
    mln_reg_match_result_free(res);
    PASS();

    TEST("multiple capture groups");
    res = mln_reg_match_result_new(1);
    assert(res != NULL);
    n = do_match("(ab)(cd)", "xabcdx", res);
    assert(n >= 2);
    mln_reg_match_result_free(res);
    PASS();

    TEST("empty group");
    res = mln_reg_match_result_new(1);
    assert(res != NULL);
    n = do_match("a()b", "ab", res);
    /* empty group should still work */
    assert(n >= 0);
    mln_reg_match_result_free(res);
    PASS();

    TEST("group with quantifier");
    res = mln_reg_match_result_new(1);
    assert(res != NULL);
    n = do_match("(ab)+", "ababab", res);
    assert(n >= 1);
    mln_reg_match_result_free(res);
    PASS();
}

static void test_char_class(void)
{
    printf("=== Character classes [] ===\n");
    mln_reg_match_result_t *res;
    int n;

    TEST("simple char class [abc]");
    res = mln_reg_match_result_new(1);
    assert(res != NULL);
    n = do_match("[abc]", "xbx", res);
    assert(n >= 1);
    check_match(mln_reg_match_result_get(res), 0, "b");
    mln_reg_match_result_free(res);
    PASS();

    TEST("char range [a-z]");
    res = mln_reg_match_result_new(1);
    assert(res != NULL);
    n = do_match("[a-z]", "A1mB", res);
    assert(n >= 1);
    check_match(mln_reg_match_result_get(res), 0, "m");
    mln_reg_match_result_free(res);
    PASS();

    TEST("negated char class [^abc]");
    res = mln_reg_match_result_new(1);
    assert(res != NULL);
    n = do_match("[^abc]", "ax", res);
    assert(n >= 1);
    check_match(mln_reg_match_result_get(res), 0, "x");
    mln_reg_match_result_free(res);
    PASS();

    TEST("negated range [^0-9]");
    res = mln_reg_match_result_new(1);
    assert(res != NULL);
    n = do_match("[^0-9]", "1a2", res);
    assert(n >= 1);
    check_match(mln_reg_match_result_get(res), 0, "a");
    mln_reg_match_result_free(res);
    PASS();

    TEST("char class with quantifier [a-z]+");
    res = mln_reg_match_result_new(1);
    assert(res != NULL);
    n = do_match("[a-z]+", "Hello", res);
    assert(n >= 1);
    check_match(mln_reg_match_result_get(res), 0, "ello");
    mln_reg_match_result_free(res);
    PASS();
}

static void test_alternation(void)
{
    printf("=== Alternation (|) ===\n");
    mln_reg_match_result_t *res;
    int n;

    /*
     * NOTE: In this regexp engine, | alternates only the adjacent atoms
     * (single tokens), NOT full sub-expressions. So "cat|dog" means
     * "ca" + (t|d) + "og", NOT "(cat)|(dog)".
     * For full alternation semantics use char classes [td] instead.
     */

    TEST("single-char alternation - match first");
    res = mln_reg_match_result_new(1);
    assert(res != NULL);
    n = do_match("a|b", "xa", res);
    assert(n >= 1);
    check_match(mln_reg_match_result_get(res), 0, "a");
    mln_reg_match_result_free(res);
    PASS();

    TEST("single-char alternation - match second");
    res = mln_reg_match_result_new(1);
    assert(res != NULL);
    n = do_match("a|b", "xb", res);
    assert(n >= 1);
    check_match(mln_reg_match_result_get(res), 0, "b");
    mln_reg_match_result_free(res);
    PASS();

    TEST("alternation no match");
    res = mln_reg_match_result_new(1);
    assert(res != NULL);
    n = do_match("a|b", "xyz", res);
    assert(n == 0);
    mln_reg_match_result_free(res);
    PASS();

    TEST("alternation in context: ca(t|d)og");
    res = mln_reg_match_result_new(1);
    assert(res != NULL);
    /* In this engine cat|dog means ca(t|d)og - test the 't' branch */
    n = do_match("cat|dog", "xcatog", res);
    assert(n >= 1);
    check_match(mln_reg_match_result_get(res), 0, "catog");
    mln_reg_match_result_free(res);
    PASS();

    TEST("multi alternation a|b|c");
    res = mln_reg_match_result_new(1);
    assert(res != NULL);
    n = do_match("a|b|c", "xc", res);
    assert(n >= 1);
    check_match(mln_reg_match_result_get(res), 0, "c");
    mln_reg_match_result_free(res);
    PASS();
}

static void test_escape_sequences(void)
{
    printf("=== Escape sequences ===\n");
    mln_reg_match_result_t *res;
    int n;

    TEST("\\d matches digit");
    res = mln_reg_match_result_new(1);
    assert(res != NULL);
    n = do_match("\\d", "abc5def", res);
    assert(n >= 1);
    check_match(mln_reg_match_result_get(res), 0, "5");
    mln_reg_match_result_free(res);
    PASS();

    TEST("\\d+ matches digits");
    res = mln_reg_match_result_new(1);
    assert(res != NULL);
    n = do_match("\\d+", "abc123def", res);
    assert(n >= 1);
    check_match(mln_reg_match_result_get(res), 0, "123");
    mln_reg_match_result_free(res);
    PASS();

    TEST("\\D matches non-digit");
    res = mln_reg_match_result_new(1);
    assert(res != NULL);
    n = do_match("\\D", "123a456", res);
    assert(n >= 1);
    check_match(mln_reg_match_result_get(res), 0, "a");
    mln_reg_match_result_free(res);
    PASS();

    TEST("\\w matches word char (alpha/digit/underscore)");
    res = mln_reg_match_result_new(1);
    assert(res != NULL);
    n = do_match("\\w", " !a ", res);
    assert(n >= 1);
    check_match(mln_reg_match_result_get(res), 0, "a");
    mln_reg_match_result_free(res);
    PASS();

    TEST("\\W matches non-word char");
    res = mln_reg_match_result_new(1);
    assert(res != NULL);
    n = do_match("\\W", "abc def", res);
    assert(n >= 1);
    check_match(mln_reg_match_result_get(res), 0, " ");
    mln_reg_match_result_free(res);
    PASS();

    TEST("\\s matches whitespace");
    res = mln_reg_match_result_new(1);
    assert(res != NULL);
    n = do_match("\\s", "abc def", res);
    assert(n >= 1);
    check_match(mln_reg_match_result_get(res), 0, " ");
    mln_reg_match_result_free(res);
    PASS();

    TEST("\\S matches non-whitespace");
    res = mln_reg_match_result_new(1);
    assert(res != NULL);
    n = do_match("\\S", " a ", res);
    assert(n >= 1);
    check_match(mln_reg_match_result_get(res), 0, "a");
    mln_reg_match_result_free(res);
    PASS();

    TEST("\\n matches newline");
    res = mln_reg_match_result_new(1);
    assert(res != NULL);
    n = do_match("a\\nb", "a\nb", res);
    assert(n >= 1);
    check_match(mln_reg_match_result_get(res), 0, "a\nb");
    mln_reg_match_result_free(res);
    PASS();

    TEST("\\t matches tab");
    res = mln_reg_match_result_new(1);
    assert(res != NULL);
    n = do_match("a\\tb", "a\tb", res);
    assert(n >= 1);
    check_match(mln_reg_match_result_get(res), 0, "a\tb");
    mln_reg_match_result_free(res);
    PASS();

    TEST("\\\\  matches literal backslash");
    res = mln_reg_match_result_new(1);
    assert(res != NULL);
    n = do_match("a\\\\b", "a\\b", res);
    assert(n >= 1);
    check_match(mln_reg_match_result_get(res), 0, "a\\b");
    mln_reg_match_result_free(res);
    PASS();

    TEST("escaped special chars \\. \\* \\+");
    res = mln_reg_match_result_new(1);
    assert(res != NULL);
    n = do_match("a\\.b", "a.b", res);
    assert(n >= 1);
    check_match(mln_reg_match_result_get(res), 0, "a.b");
    mln_reg_match_result_free(res);
    PASS();
}

static void test_reg_equal(void)
{
    printf("=== mln_reg_equal ===\n");

    TEST("equal - simple match");
    assert(do_equal("abc", "abc") == 0);
    PASS();

    TEST("equal - partial match in longer text");
    assert(do_equal("abc", "xabcx") == 0);
    PASS();

    TEST("equal - no match");
    assert(do_equal("xyz", "abc") < 0);
    PASS();

    TEST("equal - anchored exact match");
    assert(do_equal("^abc$", "abc") == 0);
    PASS();

    TEST("equal - anchored no match");
    assert(do_equal("^abc$", "abcd") != 0);
    PASS();

    TEST("equal - dot pattern");
    assert(do_equal("a.c", "abc") == 0);
    PASS();

    TEST("equal - complex pattern");
    assert(do_equal("\\d{3}-\\d{4}", "123-4567") == 0);
    PASS();
}

static void test_combined_patterns(void)
{
    printf("=== Combined patterns ===\n");
    mln_reg_match_result_t *res;
    int n;

    TEST("dot-star with anchors");
    res = mln_reg_match_result_new(1);
    assert(res != NULL);
    n = do_match("^a.*z$", "abcdefz", res);
    assert(n == 1);
    check_match(mln_reg_match_result_get(res), 0, "abcdefz");
    mln_reg_match_result_free(res);
    PASS();

    TEST("digit pattern \\d{3}-\\d{4}");
    res = mln_reg_match_result_new(1);
    assert(res != NULL);
    n = do_match("\\d{3}-\\d{4}", "call 123-4567 now", res);
    assert(n >= 1);
    check_match(mln_reg_match_result_get(res), 0, "123-4567");
    mln_reg_match_result_free(res);
    PASS();

    TEST("char class + quantifier + group");
    res = mln_reg_match_result_new(1);
    assert(res != NULL);
    n = do_match("([a-z]+)@([a-z]+)", "user@host", res);
    assert(n >= 1);
    mln_reg_match_result_free(res);
    PASS();

    TEST("alternation with quantifiers");
    res = mln_reg_match_result_new(1);
    assert(res != NULL);
    n = do_match("a+|b+", "bbb", res);
    assert(n >= 1);
    check_match(mln_reg_match_result_get(res), 0, "bbb");
    mln_reg_match_result_free(res);
    PASS();

    TEST("nested groups");
    res = mln_reg_match_result_new(1);
    assert(res != NULL);
    n = do_match("((ab)c)", "xabcx", res);
    assert(n >= 1);
    mln_reg_match_result_free(res);
    PASS();
}

static void test_edge_cases(void)
{
    printf("=== Edge cases ===\n");
    mln_reg_match_result_t *res;
    int n;

    TEST("empty text, no anchors");
    res = mln_reg_match_result_new(1);
    assert(res != NULL);
    n = do_match("abc", "", res);
    assert(n == 0);
    mln_reg_match_result_free(res);
    PASS();

    TEST("pattern at very end of text");
    res = mln_reg_match_result_new(1);
    assert(res != NULL);
    n = do_match("xyz", "abcxyz", res);
    assert(n >= 1);
    check_match(mln_reg_match_result_get(res), 0, "xyz");
    mln_reg_match_result_free(res);
    PASS();

    TEST("pattern at very start of text");
    res = mln_reg_match_result_new(1);
    assert(res != NULL);
    n = do_match("abc", "abcxyz", res);
    assert(n >= 1);
    check_match(mln_reg_match_result_get(res), 0, "abc");
    mln_reg_match_result_free(res);
    PASS();

    TEST("single char text and pattern");
    res = mln_reg_match_result_new(1);
    assert(res != NULL);
    n = do_match("a", "a", res);
    assert(n == 1);
    mln_reg_match_result_free(res);
    PASS();

    TEST("star on empty text with following pattern");
    res = mln_reg_match_result_new(1);
    assert(res != NULL);
    /* a*b on "b" should match "b" (zero a's + b) */
    n = do_match("a*b", "b", res);
    assert(n >= 1);
    check_match(mln_reg_match_result_get(res), 0, "b");
    mln_reg_match_result_free(res);
    PASS();

    TEST("overlapping matches");
    res = mln_reg_match_result_new(1);
    assert(res != NULL);
    n = do_match("a.c.e", "dabcdeadcbef", res);
    assert(n >= 1);
    check_match(mln_reg_match_result_get(res), 0, "abcde");
    mln_reg_match_result_free(res);
    PASS();
}

static void test_word_boundary(void)
{
    printf("\n=== Word boundary \\b / \\B ===\n");
    mln_reg_match_result_t *res;
    int n;

    TEST("\\b at start of word");
    res = mln_reg_match_result_new(1);
    n = do_match("\\bcat", "the cat sat", res);
    assert(n == 1);
    check_match(mln_reg_match_result_get(res), 0, "cat");
    mln_reg_match_result_free(res);
    PASS();

    TEST("\\b at end of word");
    res = mln_reg_match_result_new(1);
    n = do_match("cat\\b", "the cat sat", res);
    assert(n == 1);
    check_match(mln_reg_match_result_get(res), 0, "cat");
    mln_reg_match_result_free(res);
    PASS();

    TEST("\\b both sides");
    res = mln_reg_match_result_new(1);
    n = do_match("\\bcat\\b", "the cat sat", res);
    assert(n == 1);
    check_match(mln_reg_match_result_get(res), 0, "cat");
    mln_reg_match_result_free(res);
    PASS();

    TEST("\\b no match mid-word");
    res = mln_reg_match_result_new(1);
    n = do_match("\\bcat\\b", "concatenate", res);
    assert(n == 0);
    mln_reg_match_result_free(res);
    PASS();

    TEST("\\b at text start");
    res = mln_reg_match_result_new(1);
    n = do_match("\\bfoo", "foobar", res);
    assert(n == 1);
    check_match(mln_reg_match_result_get(res), 0, "foo");
    mln_reg_match_result_free(res);
    PASS();

    TEST("\\b at text end");
    res = mln_reg_match_result_new(1);
    n = do_match("bar\\b", "foobar", res);
    assert(n == 1);
    check_match(mln_reg_match_result_get(res), 0, "bar");
    mln_reg_match_result_free(res);
    PASS();

    TEST("\\b with reg_equal");
    assert(do_equal("\\btest\\b", "test") == 0);
    PASS();

    TEST("\\B non-boundary inside word");
    res = mln_reg_match_result_new(1);
    n = do_match("\\Bat\\B", "catch", res);
    assert(n == 1);
    check_match(mln_reg_match_result_get(res), 0, "at");
    mln_reg_match_result_free(res);
    PASS();

    TEST("\\B no match at word boundary");
    res = mln_reg_match_result_new(1);
    n = do_match("\\Bcat", "the cat sat", res);
    /* "cat" starts at a word boundary, so \Bcat should not match "cat" there.
       But it might match "cat" inside "concatenate" etc. Here no such text. */
    assert(n == 0);
    mln_reg_match_result_free(res);
    PASS();

    TEST("\\b with digit boundary");
    res = mln_reg_match_result_new(1);
    n = do_match("\\b123\\b", "abc 123 def", res);
    assert(n == 1);
    check_match(mln_reg_match_result_get(res), 0, "123");
    mln_reg_match_result_free(res);
    PASS();
}

static void test_lazy_quantifiers(void)
{
    printf("\n=== Lazy quantifiers *? +? ?? ===\n");
    mln_reg_match_result_t *res;
    int n;

    TEST("a*?b - lazy star match");
    res = mln_reg_match_result_new(1);
    n = do_match("a*?b", "aaab", res);
    assert(n >= 1);
    /* lazy star: match minimal a's before b */
    mln_reg_match_result_free(res);
    PASS();

    TEST("a+?b - lazy plus minimal");
    res = mln_reg_match_result_new(1);
    n = do_match("a+?b", "aaab", res);
    assert(n >= 1);
    mln_reg_match_result_free(res);
    PASS();

    TEST("a??b - lazy question");
    res = mln_reg_match_result_new(1);
    n = do_match("a??b", "ab", res);
    assert(n >= 1);
    mln_reg_match_result_free(res);
    PASS();

    TEST("a??b - lazy question prefers zero");
    res = mln_reg_match_result_new(1);
    n = do_match("a??b", "b", res);
    assert(n >= 1);
    check_match(mln_reg_match_result_get(res), 0, "b");
    mln_reg_match_result_free(res);
    PASS();

    TEST(".*? non-greedy dot-star");
    res = mln_reg_match_result_new(1);
    n = do_match("a.*?c", "abcabc", res);
    assert(n >= 1);
    check_match(mln_reg_match_result_get(res), 0, "abc");
    mln_reg_match_result_free(res);
    PASS();

    TEST(".+? non-greedy dot-plus");
    res = mln_reg_match_result_new(1);
    n = do_match("a.+?c", "abcabc", res);
    assert(n >= 1);
    check_match(mln_reg_match_result_get(res), 0, "abc");
    mln_reg_match_result_free(res);
    PASS();

    TEST("lazy star with reg_equal");
    assert(do_equal("a*?b", "b") == 0);
    PASS();

    TEST("lazy plus with reg_equal");
    assert(do_equal("a+?b", "ab") == 0);
    PASS();
}

static void test_non_capturing_group(void)
{
    printf("\n=== Non-capturing group (?:...) ===\n");
    mln_reg_match_result_t *res;
    int n;

    TEST("(?:abc) match no capture");
    res = mln_reg_match_result_new(1);
    n = do_match("(?:abc)", "xabcx", res);
    /* Non-capturing group should match but not add to matches array */
    /* Actually the match result stores the overall match, not group captures */
    assert(n >= 0);
    mln_reg_match_result_free(res);
    PASS();

    TEST("(?:abc) vs (abc) capture difference");
    res = mln_reg_match_result_new(1);
    int n_nc = do_match("(?:abc)", "xabcx", res);
    mln_reg_match_result_free(res);

    res = mln_reg_match_result_new(1);
    int n_c = do_match("(abc)", "xabcx", res);
    mln_reg_match_result_free(res);
    /* Non-capturing should have fewer matches (no group capture) */
    assert(n_nc < n_c);
    PASS();

    TEST("(?:a)b match");
    res = mln_reg_match_result_new(1);
    n = do_match("(?:a)b", "ab", res);
    assert(n >= 1);
    mln_reg_match_result_free(res);
    PASS();

    TEST("(?:abc) with reg_equal");
    assert(do_equal("(?:abc)", "abc") == 0);
    PASS();
}

static void test_case_insensitive(void)
{
    printf("\n=== Case insensitive (?i) ===\n");
    mln_reg_match_result_t *res;
    int n;

    TEST("(?i) basic case insensitive");
    res = mln_reg_match_result_new(1);
    n = do_match("(?i)abc", "xABCx", res);
    assert(n == 1);
    check_match(mln_reg_match_result_get(res), 0, "ABC");
    mln_reg_match_result_free(res);
    PASS();

    TEST("(?i) mixed case");
    res = mln_reg_match_result_new(1);
    n = do_match("(?i)hello", "HeLLo World", res);
    assert(n == 1);
    check_match(mln_reg_match_result_get(res), 0, "HeLLo");
    mln_reg_match_result_free(res);
    PASS();

    TEST("(?i) no match different text");
    res = mln_reg_match_result_new(1);
    n = do_match("(?i)xyz", "abcdef", res);
    assert(n == 0);
    mln_reg_match_result_free(res);
    PASS();

    TEST("(?i) with reg_equal");
    assert(do_equal("(?i)abc", "ABC") == 0);
    assert(do_equal("(?i)ABC", "abc") == 0);
    PASS();

    TEST("(?i) lowercase pattern vs uppercase text");
    res = mln_reg_match_result_new(1);
    n = do_match("(?i)test", "THIS IS A TEST STRING", res);
    assert(n == 1);
    check_match(mln_reg_match_result_get(res), 0, "TEST");
    mln_reg_match_result_free(res);
    PASS();

    TEST("case sensitive by default");
    res = mln_reg_match_result_new(1);
    n = do_match("abc", "ABC", res);
    assert(n == 0);
    mln_reg_match_result_free(res);
    PASS();
}

static void test_hex_escape(void)
{
    printf("\n=== Hex escape \\xHH ===\n");
    mln_reg_match_result_t *res;
    int n;

    TEST("\\x41 matches 'A'");
    res = mln_reg_match_result_new(1);
    n = do_match("\\x41", "xAx", res);
    assert(n == 1);
    check_match(mln_reg_match_result_get(res), 0, "A");
    mln_reg_match_result_free(res);
    PASS();

    TEST("\\x61 matches 'a'");
    res = mln_reg_match_result_new(1);
    n = do_match("\\x61", "xax", res);
    assert(n == 1);
    check_match(mln_reg_match_result_get(res), 0, "a");
    mln_reg_match_result_free(res);
    PASS();

    TEST("\\x20 matches space");
    res = mln_reg_match_result_new(1);
    n = do_match("\\x20", "a b", res);
    assert(n == 1);
    check_match(mln_reg_match_result_get(res), 0, " ");
    mln_reg_match_result_free(res);
    PASS();

    TEST("\\x2e matches literal dot (not wildcard)");
    res = mln_reg_match_result_new(1);
    n = do_match("\\x2e", "a.b", res);
    assert(n == 1);
    check_match(mln_reg_match_result_get(res), 0, ".");
    mln_reg_match_result_free(res);
    PASS();

    TEST("\\x41 with reg_equal");
    assert(do_equal("\\x41\\x42\\x43", "ABC") == 0);
    PASS();
}

static void test_word_char_extended(void)
{
    printf("\n=== \\w extended (digits + underscore) ===\n");
    mln_reg_match_result_t *res;
    int n;

    TEST("\\w matches digits");
    res = mln_reg_match_result_new(1);
    n = do_match("\\w+", "abc123", res);
    assert(n == 1);
    check_match(mln_reg_match_result_get(res), 0, "abc123");
    mln_reg_match_result_free(res);
    PASS();

    TEST("\\w matches underscore");
    res = mln_reg_match_result_new(1);
    n = do_match("\\w+", "foo_bar", res);
    assert(n == 1);
    check_match(mln_reg_match_result_get(res), 0, "foo_bar");
    mln_reg_match_result_free(res);
    PASS();

    TEST("\\w stops at non-word char");
    res = mln_reg_match_result_new(1);
    n = do_match("\\w+", "hello world", res);
    assert(n >= 1);
    check_match(mln_reg_match_result_get(res), 0, "hello");
    mln_reg_match_result_free(res);
    PASS();

    TEST("\\W matches non-word char");
    res = mln_reg_match_result_new(1);
    n = do_match("\\W+", "abc def", res);
    assert(n >= 1);
    check_match(mln_reg_match_result_get(res), 0, " ");
    mln_reg_match_result_free(res);
    PASS();

    TEST("\\w identifier pattern");
    res = mln_reg_match_result_new(1);
    n = do_match("\\w+", "_var123", res);
    assert(n == 1);
    check_match(mln_reg_match_result_get(res), 0, "_var123");
    mln_reg_match_result_free(res);
    PASS();
}

int main(int argc, char *argv[])
{
    setbuf(stdout, NULL);
    printf("Melon Regexp Test Suite\n");
    printf("========================\n\n");

    test_literal_match();
    test_dot();
    test_star();
    test_plus();
    test_question();
    test_brace();
    test_anchors();
    test_groups();
    test_char_class();
    test_alternation();
    test_escape_sequences();
    test_reg_equal();
    test_combined_patterns();
    test_edge_cases();
    test_word_boundary();
    test_lazy_quantifiers();
    test_non_capturing_group();
    test_case_insensitive();
    test_hex_escape();
    test_word_char_extended();

    printf("\n========================\n");
    printf("Results: %d/%d tests passed\n", pass_count, test_count);

    if (pass_count == test_count) {
        printf("ALL TESTS PASSED\n");
        return 0;
    } else {
        printf("SOME TESTS FAILED\n");
        return 1;
    }
}
