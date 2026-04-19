#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include "mln_fec.h"

static int test_nr = 0;

#define PASS() do { printf("  #%02d PASS\n", ++test_nr); } while (0)

/* Helper: build an RTP packet in buf. Returns total packet length. */
static uint16_t build_rtp(uint8_t *buf, uint16_t seq, uint8_t pt,
                           uint32_t ts, uint32_t ssrc,
                           const uint8_t *payload, uint16_t payload_len)
{
    buf[0] = 0x80;
    buf[1] = pt & 0x7f;
    buf[2] = (uint8_t)(seq >> 8);
    buf[3] = (uint8_t)(seq & 0xff);
    buf[4] = (uint8_t)(ts >> 24);
    buf[5] = (uint8_t)(ts >> 16);
    buf[6] = (uint8_t)(ts >> 8);
    buf[7] = (uint8_t)(ts & 0xff);
    buf[8] = (uint8_t)(ssrc >> 24);
    buf[9] = (uint8_t)(ssrc >> 16);
    buf[10] = (uint8_t)(ssrc >> 8);
    buf[11] = (uint8_t)(ssrc & 0xff);
    if (payload != NULL && payload_len > 0)
        memcpy(buf + 12, payload, payload_len);
    return (uint16_t)(12 + payload_len);
}

static void test_new_free(void)
{
    mln_fec_t *fec = mln_fec_new();
    assert(fec != NULL);
    mln_fec_free(fec);
    mln_fec_free(NULL);
    PASS();
}

static void test_set_pt(void)
{
    mln_fec_t *fec = mln_fec_new();
    assert(fec != NULL);
    mln_fec_set_pt(fec, 127);
    mln_fec_free(fec);
    PASS();
}

static void test_result_free_null(void)
{
    mln_fec_result_free(NULL);
    PASS();
}

static void test_encode_basic(void)
{
    mln_fec_t *fec = mln_fec_new();
    assert(fec != NULL);
    mln_fec_set_pt(fec, 127);

    uint8_t pkt_bufs[4][256];
    uint8_t *packets[4];
    uint16_t packlen[4];
    uint8_t payload[100];
    int i;

    for (i = 0; i < 4; ++i) {
        memset(payload, 'A' + i, sizeof(payload));
        packlen[i] = build_rtp(pkt_bufs[i], (uint16_t)(100 + i), 96,
                               (uint32_t)(1000 + i * 160), 0x12345678,
                               payload, sizeof(payload));
        packets[i] = pkt_bufs[i];
    }

    mln_fec_result_t *enc = mln_fec_encode(fec, packets, packlen, 4, 4);
    assert(enc != NULL);
    assert(mln_fec_get_result_num(enc) == 1);

    size_t fec_len;
    uint8_t *fec_pkt = mln_fec_get_result(enc, 0, fec_len);
    assert(fec_pkt != NULL);
    assert(fec_len > 0);

    /* Out of bounds access returns NULL */
    size_t dummy;
    assert(mln_fec_get_result(enc, 1, dummy) == NULL);

    mln_fec_result_free(enc);
    mln_fec_free(fec);
    PASS();
}

static void test_encode_multiple_groups(void)
{
    mln_fec_t *fec = mln_fec_new();
    assert(fec != NULL);
    mln_fec_set_pt(fec, 127);

    uint8_t pkt_bufs[8][256];
    uint8_t *packets[8];
    uint16_t packlen[8];
    uint8_t payload[80];
    int i;

    for (i = 0; i < 8; ++i) {
        memset(payload, 'a' + i, sizeof(payload));
        packlen[i] = build_rtp(pkt_bufs[i], (uint16_t)(200 + i), 96,
                               (uint32_t)(2000 + i * 160), 0xAABBCCDD,
                               payload, sizeof(payload));
        packets[i] = pkt_bufs[i];
    }

    mln_fec_result_t *enc = mln_fec_encode(fec, packets, packlen, 8, 4);
    assert(enc != NULL);
    assert(mln_fec_get_result_num(enc) == 2);

    size_t len0, len1;
    assert(mln_fec_get_result(enc, 0, len0) != NULL);
    assert(mln_fec_get_result(enc, 1, len1) != NULL);

    mln_fec_result_free(enc);
    mln_fec_free(fec);
    PASS();
}

static void test_seq_no_increment(void)
{
    mln_fec_t *fec = mln_fec_new();
    assert(fec != NULL);
    mln_fec_set_pt(fec, 127);

    uint8_t pkt_bufs[8][256];
    uint8_t *packets[8];
    uint16_t packlen[8];
    uint8_t payload[50];
    int i;

    for (i = 0; i < 8; ++i) {
        memset(payload, (uint8_t)i, sizeof(payload));
        packlen[i] = build_rtp(pkt_bufs[i], (uint16_t)(300 + i), 96,
                               (uint32_t)(3000 + i * 160), 0x11223344,
                               payload, sizeof(payload));
        packets[i] = pkt_bufs[i];
    }

    mln_fec_result_t *enc = mln_fec_encode(fec, packets, packlen, 8, 4);
    assert(enc != NULL);
    assert(mln_fec_get_result_num(enc) == 2);

    /* FEC packets should have sequential seq_no starting from 0 */
    size_t l0, l1;
    uint8_t *p0 = mln_fec_get_result(enc, 0, l0);
    uint8_t *p1 = mln_fec_get_result(enc, 1, l1);
    uint16_t sn0 = (uint16_t)((p0[2] << 8) | p0[3]);
    uint16_t sn1 = (uint16_t)((p1[2] << 8) | p1[3]);
    assert(sn1 == sn0 + 1);

    mln_fec_result_free(enc);
    mln_fec_free(fec);
    PASS();
}

/* Helper: encode 4 packets, lose one at given index, decode and verify recovery */
static void roundtrip_test(int loss_index)
{
    mln_fec_t *enc_fec = mln_fec_new();
    mln_fec_t *dec_fec = mln_fec_new();
    assert(enc_fec != NULL && dec_fec != NULL);
    mln_fec_set_pt(enc_fec, 127);
    mln_fec_set_pt(dec_fec, 127);

    uint8_t pkt_bufs[4][256];
    uint8_t *packets[4];
    uint16_t packlen[4];
    uint8_t payload[100];
    int i;

    for (i = 0; i < 4; ++i) {
        memset(payload, 'A' + i, sizeof(payload));
        packlen[i] = build_rtp(pkt_bufs[i], (uint16_t)(100 + i), 96,
                               (uint32_t)(1000 + i * 160), 0x12345678,
                               payload, sizeof(payload));
        packets[i] = pkt_bufs[i];
    }

    /* Encode */
    mln_fec_result_t *enc = mln_fec_encode(enc_fec, packets, packlen, 4, 4);
    assert(enc != NULL);

    size_t fec_len;
    uint8_t *fec_pkt = mln_fec_get_result(enc, 0, fec_len);
    assert(fec_pkt != NULL);

    /* Copy FEC packet */
    uint8_t fec_copy[1500];
    memcpy(fec_copy, fec_pkt, fec_len);

    /* Build decode input: 3 remaining packets + FEC */
    uint8_t *dec_packets[4];
    uint16_t dec_packlen[4];
    int j = 0;
    for (i = 0; i < 4; ++i) {
        if (i == loss_index) continue;
        dec_packets[j] = packets[i];
        dec_packlen[j] = packlen[i];
        ++j;
    }
    dec_packets[j] = fec_copy;
    dec_packlen[j] = (uint16_t)fec_len;

    /* Decode */
    mln_fec_result_t *dec = mln_fec_decode(dec_fec, dec_packets, dec_packlen, 4);
    assert(dec != NULL);
    assert(mln_fec_get_result_num(dec) == 1);

    size_t rec_len;
    uint8_t *recovered = mln_fec_get_result(dec, 0, rec_len);
    assert(recovered != NULL);
    assert(rec_len == packlen[loss_index]);

    /* Verify payload matches */
    assert(memcmp(recovered + 12, pkt_bufs[loss_index] + 12,
                  packlen[loss_index] - 12) == 0);

    /* Verify recovered seq_no */
    uint16_t rec_seq = (uint16_t)((recovered[2] << 8) | recovered[3]);
    assert(rec_seq == (uint16_t)(100 + loss_index));

    mln_fec_result_free(enc);
    mln_fec_result_free(dec);
    mln_fec_free(enc_fec);
    mln_fec_free(dec_fec);
}

static void test_roundtrip_first_loss(void)
{
    roundtrip_test(0);
    PASS();
}

static void test_roundtrip_middle_loss(void)
{
    roundtrip_test(1);
    PASS();
}

static void test_roundtrip_third_loss(void)
{
    roundtrip_test(2);
    PASS();
}

static void test_roundtrip_last_loss(void)
{
    roundtrip_test(3);
    PASS();
}

static void test_no_loss(void)
{
    mln_fec_t *enc_fec = mln_fec_new();
    mln_fec_t *dec_fec = mln_fec_new();
    assert(enc_fec != NULL && dec_fec != NULL);
    mln_fec_set_pt(enc_fec, 127);
    mln_fec_set_pt(dec_fec, 127);

    uint8_t pkt_bufs[4][256];
    uint8_t *packets[4];
    uint16_t packlen[4];
    uint8_t payload[60];
    int i;

    for (i = 0; i < 4; ++i) {
        memset(payload, (uint8_t)i, sizeof(payload));
        packlen[i] = build_rtp(pkt_bufs[i], (uint16_t)(100 + i), 96,
                               1000, 0x12345678, payload, sizeof(payload));
        packets[i] = pkt_bufs[i];
    }

    mln_fec_result_t *enc = mln_fec_encode(enc_fec, packets, packlen, 4, 4);
    assert(enc != NULL);

    size_t fec_len;
    uint8_t *fec_pkt = mln_fec_get_result(enc, 0, fec_len);
    uint8_t fec_copy[1500];
    memcpy(fec_copy, fec_pkt, fec_len);

    /* All 4 packets + FEC = no loss */
    uint8_t *dec_packets[5];
    uint16_t dec_packlen[5];
    for (i = 0; i < 4; ++i) {
        dec_packets[i] = packets[i];
        dec_packlen[i] = packlen[i];
    }
    dec_packets[4] = fec_copy;
    dec_packlen[4] = (uint16_t)fec_len;

    mln_fec_result_t *dec = mln_fec_decode(dec_fec, dec_packets, dec_packlen, 5);
    assert(dec != NULL);
    assert(mln_fec_get_result_num(dec) == 0);

    mln_fec_result_free(enc);
    mln_fec_result_free(dec);
    mln_fec_free(enc_fec);
    mln_fec_free(dec_fec);
    PASS();
}

static void test_no_fec_packet(void)
{
    mln_fec_t *fec = mln_fec_new();
    assert(fec != NULL);
    mln_fec_set_pt(fec, 127);

    uint8_t pkt_bufs[3][256];
    uint8_t *packets[3];
    uint16_t packlen[3];
    uint8_t payload[40];
    int i;

    for (i = 0; i < 3; ++i) {
        memset(payload, (uint8_t)i, sizeof(payload));
        packlen[i] = build_rtp(pkt_bufs[i], (uint16_t)(100 + i), 96,
                               1000, 0x12345678, payload, sizeof(payload));
        packets[i] = pkt_bufs[i];
    }

    /* Decode without FEC packet */
    mln_fec_result_t *dec = mln_fec_decode(fec, packets, packlen, 3);
    assert(dec != NULL);
    assert(mln_fec_get_result_num(dec) == 0);

    mln_fec_result_free(dec);
    mln_fec_free(fec);
    PASS();
}

static void test_group_size_1(void)
{
    mln_fec_t *enc_fec = mln_fec_new();
    mln_fec_t *dec_fec = mln_fec_new();
    assert(enc_fec != NULL && dec_fec != NULL);
    mln_fec_set_pt(enc_fec, 127);
    mln_fec_set_pt(dec_fec, 127);

    uint8_t pkt_buf[256];
    uint8_t *packets[1];
    uint16_t packlen[1];
    uint8_t payload[50];

    memset(payload, 0xAB, sizeof(payload));
    packlen[0] = build_rtp(pkt_buf, 500, 96, 5000, 0xDEADBEEF,
                           payload, sizeof(payload));
    packets[0] = pkt_buf;

    mln_fec_result_t *enc = mln_fec_encode(enc_fec, packets, packlen, 1, 1);
    assert(enc != NULL);
    assert(mln_fec_get_result_num(enc) == 1);

    size_t fec_len;
    uint8_t *fec_pkt = mln_fec_get_result(enc, 0, fec_len);
    uint8_t fec_copy[1500];
    memcpy(fec_copy, fec_pkt, fec_len);

    /* Lose the only packet, keep FEC */
    uint8_t *dec_packets[1];
    uint16_t dec_packlen[1];
    dec_packets[0] = fec_copy;
    dec_packlen[0] = (uint16_t)fec_len;

    mln_fec_result_t *dec = mln_fec_decode(dec_fec, dec_packets, dec_packlen, 1);
    assert(dec != NULL);
    assert(mln_fec_get_result_num(dec) == 1);

    size_t rec_len;
    uint8_t *recovered = mln_fec_get_result(dec, 0, rec_len);
    assert(recovered != NULL);
    assert(rec_len == packlen[0]);
    assert(memcmp(recovered + 12, pkt_buf + 12, packlen[0] - 12) == 0);

    mln_fec_result_free(enc);
    mln_fec_result_free(dec);
    mln_fec_free(enc_fec);
    mln_fec_free(dec_fec);
    PASS();
}

static void test_group_size_2(void)
{
    mln_fec_t *enc_fec = mln_fec_new();
    mln_fec_t *dec_fec = mln_fec_new();
    assert(enc_fec != NULL && dec_fec != NULL);
    mln_fec_set_pt(enc_fec, 127);
    mln_fec_set_pt(dec_fec, 127);

    uint8_t pkt_bufs[2][256];
    uint8_t *packets[2];
    uint16_t packlen[2];
    uint8_t payload[70];
    int i;

    for (i = 0; i < 2; ++i) {
        memset(payload, 0x10 + i, sizeof(payload));
        packlen[i] = build_rtp(pkt_bufs[i], (uint16_t)(600 + i), 96,
                               6000, 0x99887766, payload, sizeof(payload));
        packets[i] = pkt_bufs[i];
    }

    mln_fec_result_t *enc = mln_fec_encode(enc_fec, packets, packlen, 2, 2);
    assert(enc != NULL);
    assert(mln_fec_get_result_num(enc) == 1);

    /* Lose first packet */
    size_t fec_len;
    uint8_t *fec_pkt = mln_fec_get_result(enc, 0, fec_len);
    uint8_t fec_copy[1500];
    memcpy(fec_copy, fec_pkt, fec_len);

    uint8_t *dec_packets[2];
    uint16_t dec_packlen[2];
    dec_packets[0] = packets[1];
    dec_packlen[0] = packlen[1];
    dec_packets[1] = fec_copy;
    dec_packlen[1] = (uint16_t)fec_len;

    mln_fec_result_t *dec = mln_fec_decode(dec_fec, dec_packets, dec_packlen, 2);
    assert(dec != NULL);
    assert(mln_fec_get_result_num(dec) == 1);

    size_t rec_len;
    uint8_t *recovered = mln_fec_get_result(dec, 0, rec_len);
    assert(recovered != NULL);
    assert(rec_len == packlen[0]);
    assert(memcmp(recovered + 12, pkt_bufs[0] + 12, packlen[0] - 12) == 0);

    mln_fec_result_free(enc);
    mln_fec_result_free(dec);
    mln_fec_free(enc_fec);
    mln_fec_free(dec_fec);
    PASS();
}

static void test_different_payload_sizes(void)
{
    mln_fec_t *enc_fec = mln_fec_new();
    mln_fec_t *dec_fec = mln_fec_new();
    assert(enc_fec != NULL && dec_fec != NULL);
    mln_fec_set_pt(enc_fec, 127);
    mln_fec_set_pt(dec_fec, 127);

    /* Packets with different payload sizes */
    uint8_t pkt_bufs[4][1442];
    uint8_t *packets[4];
    uint16_t packlen[4];
    uint16_t payload_sizes[4] = {20, 50, 100, 30};
    int i;

    for (i = 0; i < 4; ++i) {
        uint8_t payload[1430];
        memset(payload, 'A' + i, payload_sizes[i]);
        packlen[i] = build_rtp(pkt_bufs[i], (uint16_t)(700 + i), 96,
                               7000, 0x55667788, payload, payload_sizes[i]);
        packets[i] = pkt_bufs[i];
    }

    mln_fec_result_t *enc = mln_fec_encode(enc_fec, packets, packlen, 4, 4);
    assert(enc != NULL);

    size_t fec_len;
    uint8_t *fec_pkt = mln_fec_get_result(enc, 0, fec_len);
    uint8_t fec_copy[1500];
    memcpy(fec_copy, fec_pkt, fec_len);

    /* Lose the largest packet (index 2, payload=100) */
    uint8_t *dec_packets[4];
    uint16_t dec_packlen[4];
    int j = 0;
    for (i = 0; i < 4; ++i) {
        if (i == 2) continue;
        dec_packets[j] = packets[i];
        dec_packlen[j] = packlen[i];
        ++j;
    }
    dec_packets[j] = fec_copy;
    dec_packlen[j] = (uint16_t)fec_len;

    mln_fec_result_t *dec = mln_fec_decode(dec_fec, dec_packets, dec_packlen, 4);
    assert(dec != NULL);
    assert(mln_fec_get_result_num(dec) == 1);

    size_t rec_len;
    uint8_t *recovered = mln_fec_get_result(dec, 0, rec_len);
    assert(recovered != NULL);
    assert(rec_len == packlen[2]);
    assert(memcmp(recovered + 12, pkt_bufs[2] + 12, payload_sizes[2]) == 0);

    mln_fec_result_free(enc);
    mln_fec_result_free(dec);
    mln_fec_free(enc_fec);
    mln_fec_free(dec_fec);
    PASS();
}

static void test_min_packet_size(void)
{
    mln_fec_t *enc_fec = mln_fec_new();
    mln_fec_t *dec_fec = mln_fec_new();
    assert(enc_fec != NULL && dec_fec != NULL);
    mln_fec_set_pt(enc_fec, 127);
    mln_fec_set_pt(dec_fec, 127);

    /* Minimum packet: RTP header only, no payload */
    uint8_t pkt_bufs[2][14];
    uint8_t *packets[2];
    uint16_t packlen[2];
    int i;

    for (i = 0; i < 2; ++i) {
        packlen[i] = build_rtp(pkt_bufs[i], (uint16_t)(800 + i), 96,
                               8000, 0x11111111, NULL, 0);
        packets[i] = pkt_bufs[i];
    }

    mln_fec_result_t *enc = mln_fec_encode(enc_fec, packets, packlen, 2, 2);
    assert(enc != NULL);

    size_t fec_len;
    uint8_t *fec_pkt = mln_fec_get_result(enc, 0, fec_len);
    uint8_t fec_copy[1500];
    memcpy(fec_copy, fec_pkt, fec_len);

    uint8_t *dec_packets[2];
    uint16_t dec_packlen[2];
    dec_packets[0] = packets[1];
    dec_packlen[0] = packlen[1];
    dec_packets[1] = fec_copy;
    dec_packlen[1] = (uint16_t)fec_len;

    mln_fec_result_t *dec = mln_fec_decode(dec_fec, dec_packets, dec_packlen, 2);
    assert(dec != NULL);

    mln_fec_result_free(enc);
    mln_fec_result_free(dec);
    mln_fec_free(enc_fec);
    mln_fec_free(dec_fec);
    PASS();
}

static void test_max_packet_size(void)
{
    mln_fec_t *fec = mln_fec_new();
    assert(fec != NULL);
    mln_fec_set_pt(fec, 127);

    uint8_t *pkt_bufs[2];
    uint8_t *packets[2];
    uint16_t packlen[2];
    uint8_t payload[1430]; /* 1442 - 12 = 1430 max payload */
    int i;

    for (i = 0; i < 2; ++i) {
        pkt_bufs[i] = (uint8_t *)malloc(1442);
        assert(pkt_bufs[i] != NULL);
        memset(payload, 0xCC + i, sizeof(payload));
        packlen[i] = build_rtp(pkt_bufs[i], (uint16_t)(900 + i), 96,
                               9000, 0x22222222, payload, sizeof(payload));
        packets[i] = pkt_bufs[i];
    }

    mln_fec_result_t *enc = mln_fec_encode(fec, packets, packlen, 2, 2);
    assert(enc != NULL);
    assert(mln_fec_get_result_num(enc) == 1);

    mln_fec_result_free(enc);
    for (i = 0; i < 2; ++i) free(pkt_bufs[i]);
    mln_fec_free(fec);
    PASS();
}

static void test_encode_invalid_params(void)
{
    mln_fec_t *fec = mln_fec_new();
    assert(fec != NULL);

    uint8_t pkt[256];
    uint8_t *packets[1] = {pkt};
    uint16_t packlen[1] = {100};

    /* NULL fec */
    assert(mln_fec_encode(NULL, packets, packlen, 1, 1) == NULL);

    /* NULL packets */
    assert(mln_fec_encode(fec, NULL, packlen, 1, 1) == NULL);

    /* NULL packlen */
    assert(mln_fec_encode(fec, packets, NULL, 1, 1) == NULL);

    /* n = 0 */
    assert(mln_fec_encode(fec, packets, packlen, 0, 1) == NULL);

    /* group_size = 0 */
    assert(mln_fec_encode(fec, packets, packlen, 1, 0) == NULL);

    /* group_size > 48 */
    assert(mln_fec_encode(fec, packets, packlen, 1, 49) == NULL);

    /* packet too small */
    packlen[0] = 11;
    assert(mln_fec_encode(fec, packets, packlen, 1, 1) == NULL);

    /* packet too large */
    packlen[0] = 1443;
    assert(mln_fec_encode(fec, packets, packlen, 1, 1) == NULL);

    mln_fec_free(fec);
    PASS();
}

static void test_decode_invalid_params(void)
{
    mln_fec_t *fec = mln_fec_new();
    assert(fec != NULL);

    uint8_t pkt[256];
    uint8_t *packets[1] = {pkt};
    uint16_t packlen[1] = {100};

    /* NULL fec */
    assert(mln_fec_decode(NULL, packets, packlen, 1) == NULL);

    /* NULL packets */
    assert(mln_fec_decode(fec, NULL, packlen, 1) == NULL);

    /* NULL packlen */
    assert(mln_fec_decode(fec, packets, NULL, 1) == NULL);

    /* n = 0 */
    assert(mln_fec_decode(fec, packets, packlen, 0) == NULL);

    /* packet too small */
    packlen[0] = 11;
    assert(mln_fec_decode(fec, packets, packlen, 1) == NULL);

    /* packet too large */
    packlen[0] = 1473;
    assert(mln_fec_decode(fec, packets, packlen, 1) == NULL);

    mln_fec_free(fec);
    PASS();
}

static void test_group_size_16(void)
{
    mln_fec_t *enc_fec = mln_fec_new();
    mln_fec_t *dec_fec = mln_fec_new();
    assert(enc_fec != NULL && dec_fec != NULL);
    mln_fec_set_pt(enc_fec, 127);
    mln_fec_set_pt(dec_fec, 127);

    uint8_t pkt_bufs[16][128];
    uint8_t *packets[16];
    uint16_t packlen[16];
    uint8_t payload[40];
    int i;

    for (i = 0; i < 16; ++i) {
        memset(payload, (uint8_t)(0x10 + i), sizeof(payload));
        packlen[i] = build_rtp(pkt_bufs[i], (uint16_t)(1000 + i), 96,
                               10000, 0x33333333, payload, sizeof(payload));
        packets[i] = pkt_bufs[i];
    }

    mln_fec_result_t *enc = mln_fec_encode(enc_fec, packets, packlen, 16, 16);
    assert(enc != NULL);
    assert(mln_fec_get_result_num(enc) == 1);

    size_t fec_len;
    uint8_t *fec_pkt = mln_fec_get_result(enc, 0, fec_len);
    uint8_t fec_copy[1500];
    memcpy(fec_copy, fec_pkt, fec_len);

    /* Lose packet at index 7 */
    uint8_t *dec_packets[16];
    uint16_t dec_packlen[16];
    int j = 0;
    for (i = 0; i < 16; ++i) {
        if (i == 7) continue;
        dec_packets[j] = packets[i];
        dec_packlen[j] = packlen[i];
        ++j;
    }
    dec_packets[j] = fec_copy;
    dec_packlen[j] = (uint16_t)fec_len;

    mln_fec_result_t *dec = mln_fec_decode(dec_fec, dec_packets, dec_packlen, 16);
    assert(dec != NULL);
    assert(mln_fec_get_result_num(dec) == 1);

    size_t rec_len;
    uint8_t *recovered = mln_fec_get_result(dec, 0, rec_len);
    assert(recovered != NULL);
    assert(rec_len == packlen[7]);
    assert(memcmp(recovered + 12, pkt_bufs[7] + 12, packlen[7] - 12) == 0);

    mln_fec_result_free(enc);
    mln_fec_result_free(dec);
    mln_fec_free(enc_fec);
    mln_fec_free(dec_fec);
    PASS();
}

static void test_performance(void)
{
    int i, iters = 10000;
    struct timespec t0, t1;
    double enc_sec, dec_sec;
    mln_fec_t *fec;

    uint8_t pkt_bufs[4][256];
    uint8_t *packets[4];
    uint16_t packlen[4];
    uint8_t payload[100];

    for (i = 0; i < 4; ++i) {
        memset(payload, 'A' + i, sizeof(payload));
        packlen[i] = build_rtp(pkt_bufs[i], (uint16_t)(100 + i), 96,
                               (uint32_t)(1000 + i * 160), 0x12345678,
                               payload, sizeof(payload));
        packets[i] = pkt_bufs[i];
    }

    /* Benchmark encode */
    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (i = 0; i < iters; ++i) {
        fec = mln_fec_new();
        mln_fec_set_pt(fec, 127);
        mln_fec_result_t *enc = mln_fec_encode(fec, packets, packlen, 4, 4);
        mln_fec_result_free(enc);
        mln_fec_free(fec);
    }
    clock_gettime(CLOCK_MONOTONIC, &t1);
    enc_sec = (t1.tv_sec - t0.tv_sec) + (t1.tv_nsec - t0.tv_nsec) / 1e9;

    /* Benchmark decode */
    fec = mln_fec_new();
    mln_fec_set_pt(fec, 127);
    mln_fec_result_t *enc = mln_fec_encode(fec, packets, packlen, 4, 4);
    size_t fec_len;
    uint8_t *fec_pkt = mln_fec_get_result(enc, 0, fec_len);
    uint8_t fec_copy[1500];
    memcpy(fec_copy, fec_pkt, fec_len);
    mln_fec_result_free(enc);
    mln_fec_free(fec);

    uint8_t *dec_packets[4];
    uint16_t dec_packlen[4];
    dec_packets[0] = packets[0];
    dec_packlen[0] = packlen[0];
    dec_packets[1] = packets[2];
    dec_packlen[1] = packlen[2];
    dec_packets[2] = packets[3];
    dec_packlen[2] = packlen[3];
    dec_packets[3] = fec_copy;
    dec_packlen[3] = (uint16_t)fec_len;

    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (i = 0; i < iters; ++i) {
        fec = mln_fec_new();
        mln_fec_set_pt(fec, 127);
        mln_fec_result_t *dec = mln_fec_decode(fec, dec_packets, dec_packlen, 4);
        mln_fec_result_free(dec);
        mln_fec_free(fec);
    }
    clock_gettime(CLOCK_MONOTONIC, &t1);
    dec_sec = (t1.tv_sec - t0.tv_sec) + (t1.tv_nsec - t0.tv_nsec) / 1e9;

    printf("  Encode %d iters: %.3f sec (%.0f ops/sec)\n",
           iters, enc_sec, iters / enc_sec);
    printf("  Decode %d iters: %.3f sec (%.0f ops/sec)\n",
           iters, dec_sec, iters / dec_sec);

    assert(enc_sec < 30.0);
    assert(dec_sec < 30.0);
    PASS();
}

static void test_stability(void)
{
    int i;
    mln_fec_t *enc_fec, *dec_fec;

    uint8_t pkt_bufs[4][256];
    uint8_t *packets[4];
    uint16_t packlen[4];
    uint8_t payload[80];
    uint8_t first_recovered[256];
    size_t first_rec_len = 0;

    for (i = 0; i < 4; ++i) {
        memset(payload, 'X' + i, sizeof(payload));
        packlen[i] = build_rtp(pkt_bufs[i], (uint16_t)(500 + i), 96,
                               5000, 0xAAAABBBB, payload, sizeof(payload));
        packets[i] = pkt_bufs[i];
    }

    for (i = 0; i < 1000; ++i) {
        enc_fec = mln_fec_new();
        dec_fec = mln_fec_new();
        mln_fec_set_pt(enc_fec, 127);
        mln_fec_set_pt(dec_fec, 127);

        mln_fec_result_t *enc = mln_fec_encode(enc_fec, packets, packlen, 4, 4);
        assert(enc != NULL);

        size_t fec_len;
        uint8_t *fec_pkt = mln_fec_get_result(enc, 0, fec_len);
        uint8_t fec_copy[1500];
        memcpy(fec_copy, fec_pkt, fec_len);

        uint8_t *dec_packets[4];
        uint16_t dec_packlen[4];
        dec_packets[0] = packets[0];
        dec_packlen[0] = packlen[0];
        dec_packets[1] = packets[2];
        dec_packlen[1] = packlen[2];
        dec_packets[2] = packets[3];
        dec_packlen[2] = packlen[3];
        dec_packets[3] = fec_copy;
        dec_packlen[3] = (uint16_t)fec_len;

        mln_fec_result_t *dec = mln_fec_decode(dec_fec, dec_packets, dec_packlen, 4);
        assert(dec != NULL);
        assert(mln_fec_get_result_num(dec) == 1);

        size_t rec_len;
        uint8_t *recovered = mln_fec_get_result(dec, 0, rec_len);
        assert(recovered != NULL);

        if (i == 0) {
            first_rec_len = rec_len;
            memcpy(first_recovered, recovered, rec_len);
        } else {
            assert(rec_len == first_rec_len);
            assert(memcmp(recovered, first_recovered, rec_len) == 0);
        }

        mln_fec_result_free(enc);
        mln_fec_result_free(dec);
        mln_fec_free(enc_fec);
        mln_fec_free(dec_fec);
    }
    PASS();
}

int main(int argc, char *argv[])
{
    (void)argc; (void)argv;
    printf("FEC test suite\n");

    test_new_free();
    test_set_pt();
    test_result_free_null();
    test_encode_basic();
    test_encode_multiple_groups();
    test_seq_no_increment();
    test_roundtrip_first_loss();
    test_roundtrip_middle_loss();
    test_roundtrip_third_loss();
    test_roundtrip_last_loss();
    test_no_loss();
    test_no_fec_packet();
    test_group_size_1();
    test_group_size_2();
    test_different_payload_sizes();
    test_min_packet_size();
    test_max_packet_size();
    test_encode_invalid_params();
    test_decode_invalid_params();
    test_group_size_16();
    test_performance();
    test_stability();

    printf("All %d tests passed.\n", test_nr);
    return 0;
}
