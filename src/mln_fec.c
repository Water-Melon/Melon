
/*
 * Copyright (C) Niklaus F.Schen.
 * RFC 5109
 */
#include "mln_fec.h"
#include "mln_func.h"
#include <errno.h>

/*string_vector*/
MLN_FUNC(static inline, mln_string_t **, mln_string_vector_new, (mln_size_t n), (n), {
    return (mln_string_t **)calloc(n+1, sizeof(mln_string_t *));
})

MLN_FUNC_VOID(static inline, void, mln_string_vector_free, (mln_string_t **vec), (vec), {
    if (vec == NULL) return;
    mln_string_t **p = vec;
    for (; *p != NULL; ++p) {
        mln_string_free(*p);
    }
    free(vec);
})


/*
 * operations
 */
MLN_FUNC(static inline, mln_string_t *, mln_fec_xor, \
         (mln_string_t *data1, mln_string_t *data2), (data1, data2), \
{
    mln_string_t *ret;
    mln_u8ptr_t p1, p2, end;
    if (data1->len > data2->len) {
        if ((ret = mln_string_dup(data1)) == NULL) {
            errno = ENOMEM;
            return NULL;
        }
        p1 = ret->data;
        p2 = data2->data;
        end = data2->data + data2->len;
        for (; p2 < end; ++p1, ++p2) {
            *p1 ^= *p2;
        }
    } else {
        if ((ret = mln_string_dup(data2)) == NULL) {
            errno = ENOMEM;
            return NULL;
        }
        p1 = data1->data;
        p2 = ret->data;
        end = data1->data + data1->len;
        for (; p1 < end; ++p1, ++p2) {
            *p2 ^= *p1;
        }
    }
    return ret;
})

/*mln_fec_result_t*/
MLN_FUNC(static, mln_fec_result_t *, mln_fec_result_new, \
         (mln_string_t **packets, mln_size_t nr_packets), \
         (packets, nr_packets), \
{
    mln_fec_result_t *fr;
    if ((fr = (mln_fec_result_t *)malloc(sizeof(mln_fec_result_t))) == NULL)
        return NULL;
    fr->packets = packets;
    fr->nr_packets = nr_packets;
    return fr;
})

MLN_FUNC_VOID(, void, mln_fec_result_free, (mln_fec_result_t *fr), (fr), {
    if (fr == NULL) return;
    if (fr->packets != NULL) {
        mln_string_vector_free(fr->packets);
    }
    free(fr);
})

/*mln_fec_t*/
MLN_FUNC(, mln_fec_t *, mln_fec_new, (void), (), {
    mln_fec_t *fec;

    if ((fec = (mln_fec_t *)malloc(sizeof(mln_fec_t))) == NULL)
        return NULL;
    fec->seq_no = 0;
    fec->pt = 0;
    return fec;
})

MLN_FUNC_VOID(, void, mln_fec_free, (mln_fec_t *fec), (fec), {
    if (fec == NULL) return;
    free(fec);
})

/*generation*/
MLN_FUNC(static, int, mln_fec_generate_fecpacket_fecheader, \
         (mln_fec_t *fec, mln_size_t n, mln_u8ptr_t *packets, \
          mln_u16_t *packlen, mln_u8ptr_t buf, mln_size_t *len), \
         (fec, n, packets, packlen, buf, len), \
{
    mln_u16_t tmp16;
    mln_string_t *ret, tmp, *t;
    mln_u8_t header[10] = {0}, sn[2];
    mln_u8ptr_t *end = packets + n, p;

    sn[0] = packets[0][2];
    sn[1] = packets[0][3];
    mln_string_nset(&tmp, header, sizeof(header));
    if ((ret = mln_string_dup(&tmp)) == NULL) {
        errno = ENOMEM;
        return -1;
    }

    for (; packets < end; ++packets, ++packlen) {
        t = ret;
        tmp16 = *packlen - M_FEC_RTP_FIXEDLEN;
        memcpy(header, *packets, 8);
        p = header + 8;
        mln_bigendian_encode(tmp16, p, 2);
        mln_string_nset(&tmp, header, sizeof(header));
        ret = mln_fec_xor(&tmp, t);
        mln_string_free(t);
        if (ret == NULL) {
            errno = ENOMEM;
            return -1;
        }
    }

    buf[0] = ret->data[0] & 0x3f;
    if (n > 16) buf[0] |= 0x40;
    buf[1] = ret->data[1];
    buf[2] = sn[0];
    buf[3] = sn[1];
    buf[4] = ret->data[4];
    buf[5] = ret->data[5];
    buf[6] = ret->data[6];
    buf[7] = ret->data[7];
    buf[8] = ret->data[8];
    buf[9] = ret->data[9];
    *len += 10;
    mln_string_free(ret);
    return 0;
})

MLN_FUNC(static, int, mln_fec_generate_fecpacket_fecbody, \
         (mln_fec_t *fec, mln_size_t n, mln_u8ptr_t *packets, \
          mln_u16_t *packlen, mln_u8ptr_t buf, mln_size_t *len), \
         (fec, n, packets, packlen, buf, len), \
{
    mln_u8_t c = 0;
    mln_u64_t mask = 0;
    mln_u8ptr_t *end, *p, ptr;
    mln_string_t *t, *ret, tmp;
    mln_u16_t tmp_sn, sn_base, *pl;

    ptr = *packets + 2;
    mln_bigendian_decode(sn_base, ptr, 2);
    ptr = packets[n-1]+2;
    mln_bigendian_decode(tmp_sn, ptr, 2);

    /*calc ulp level 0 body*/
    mln_string_nset(&tmp, &c, 1);
    if ((ret = mln_string_dup(&tmp)) == NULL) {
        errno = ENOMEM;
        return -1;
    }
    end = packets + n;
    for (p = packets, pl = packlen; p < end; ++p, ++pl) {
        t = ret;
        mln_string_nset(&tmp, *p, *pl);
        ret = mln_fec_xor(&tmp, t);
        mln_string_free(t);
        if (ret == NULL) {
            errno = ENOMEM;
            return -1;
        }
    }

    /*fill ulp level 0 header*/
    mln_bigendian_encode(ret->len, buf, 2);
    *len += 2;
    if (n > 16 || tmp_sn-sn_base > 16) {
        for (p = packets, end = packets+n; p < end; ++p) {
            ptr = *p + 2;
            mln_bigendian_decode(tmp_sn, ptr, 2);
            mask |= ((mln_u64_t)1 << (47 - (tmp_sn - sn_base)));
        }
        mln_bigendian_encode(mask, buf, 6);
        *len += 6;
    } else {
        for (p = packets, end = packets+n; p < end; ++p) {
            ptr = *p + 2;
            mln_bigendian_decode(tmp_sn, ptr, 2);
            mask |= ((mln_u64_t)1 << (15 - (tmp_sn - sn_base)));
        }
        mln_bigendian_encode(mask, buf, 2);
        *len += 2;
    }

    /*fill ulp level 0 body*/
    memcpy(buf, ret->data+M_FEC_RTP_FIXEDLEN, ret->len-M_FEC_RTP_FIXEDLEN); 
    *len += ret->len-M_FEC_RTP_FIXEDLEN;
    mln_string_free(ret);

    return 0;
})

MLN_FUNC(static, mln_string_t *, mln_fec_generate_fecpacket, \
         (mln_fec_t *fec, mln_size_t n, mln_u8ptr_t *packets, mln_u16_t *packlen), \
         (fec, n, packets, packlen), \
{
    mln_string_t tmp, *ret;
    mln_size_t len = 0, mod;
    mln_u8_t buf[1472] = {0}, *p;

    if (mln_fec_generate_fecpacket_fecheader(fec, \
                                             n, \
                                             packets, \
                                             packlen, \
                                             buf+M_FEC_RTP_FIXEDLEN, \
                                             &len) < 0)
    {
        return NULL;
    }
    if (mln_fec_generate_fecpacket_fecbody(fec, \
                                           n, \
                                           packets, \
                                           packlen, \
                                           buf+M_FEC_RTP_FIXEDLEN+M_FEC_FECHEADER_LEN, \
                                           &len) < 0)
    {
        return NULL;
    }
    len += M_FEC_RTP_FIXEDLEN;
    buf[0] |= 0x80;
    if ((mod = (len % 4))) {
        buf[len + mod - 1] = mod;
        buf[0] |= 0x20;
        len += mod;
    }
    buf[1] = fec->pt;
    p = buf + 2;
    mln_bigendian_encode(fec->seq_no, p, 2);
    ++fec->seq_no;
    memcpy(p, &(packets[n-1])[4], 8);/*TS and SSRC*/
    mln_string_nset(&tmp, buf, len);
    if ((ret = mln_string_dup(&tmp)) == NULL) {
        errno = ENOMEM;
        return NULL;
    }
    return ret;
})

MLN_FUNC(, mln_fec_result_t *, mln_fec_encode, \
         (mln_fec_t *fec, uint8_t *packets[], uint16_t packlen[], size_t n, uint16_t group_size), \
         (fec, packets, packlen, n, group_size), \
{
    uint16_t *pl, *plend;
    mln_u8ptr_t *p, *pend;
    mln_string_t **vec, **next;
    mln_fec_result_t *result;

    if (fec == NULL || \
        packets == NULL || \
        packlen == NULL || \
        !n || \
        group_size > 48 || \
        !group_size)
    {
        errno = EINVAL;
        return NULL;
    }
    for (pl = packlen, plend = packlen+n; pl < plend; ++pl) {
        if (*pl < M_FEC_RTP_FIXEDLEN || *pl > M_FEC_G_MAXLEN) {
            errno = EINVAL;
            return NULL;
        }
    }

    if ((vec = mln_string_vector_new(n/group_size)) == NULL) {
        errno = ENOMEM;
        return NULL;
    }
    next = vec;

    p = (mln_u8ptr_t *)packets;
    pend = (mln_u8ptr_t *)packets+n;
    pl = packlen;
    plend = packlen+n;
    for (; p < pend; p+=group_size, pl+=group_size, ++next) {
        if ((*next = mln_fec_generate_fecpacket(fec, group_size, p, pl)) == NULL) {
            mln_string_vector_free(vec);
            errno = ENOMEM;
            return NULL;
        }
    }
    if ((result = mln_fec_result_new(vec, n/group_size)) == NULL) {
        mln_string_vector_free(vec);
        errno = ENOMEM;
        return NULL;
    }
    return result;
})

/*recovery*/
MLN_FUNC_VOID(static inline, void, mln_fec_recover_header_info_get, \
              (mln_string_t *fec_packet, mln_u16_t *sn_base, mln_u32_t *ssrc, \
               mln_u64_t *mask, mln_u16_t *is_long), \
              (fec_packet, sn_base, ssrc, mask, is_long), \
{
    mln_u8ptr_t p = fec_packet->data + 8;
    if (ssrc != NULL) {
        mln_bigendian_decode(*ssrc, p, 4);
    } else {
        p += 4;
    }
    if (is_long != NULL) {
        if (*p & 0x40) *is_long = 1;
        else *is_long = 0;
    }
    p += 2;
    if (sn_base != NULL) {
        mln_bigendian_decode(*sn_base, p, 2);
    } else {
        p += 2;
    }
    p += 8;
    if (mask != NULL) {
        if (*is_long) {
            mln_bigendian_decode(*mask, p, 6);
        } else {
            mln_bigendian_decode(*mask, p, 2);
        }
    }
})

MLN_FUNC(static, int, mln_fec_decode_header, \
         (mln_fec_t *fec, mln_string_t *fec_packet, mln_u8ptr_t buf, mln_size_t *blen, \
          mln_u8ptr_t *packets, mln_u16_t *packlen, mln_size_t n, mln_u16_t *body_len), \
         (fec, fec_packet, buf, blen, packets, packlen, n, body_len), \
{
    mln_u8ptr_t ptr;
    mln_u64_t mask = 0;
    mln_u32_t ssrc = 0;
    mln_string_t tmp, *t, *res;
    mln_u8ptr_t *p, *pend;
    mln_u16_t *pl, seq_no, is_long = 0;
    mln_u16_t sn_base = 0;
    mln_u8_t bit_string[10] = {0};

    mln_fec_recover_header_info_get(fec_packet, &sn_base, &ssrc, &mask, &is_long);
    p = packets;
    pend = packets + n;
    pl = packlen;
    mln_string_nset(&tmp, bit_string, sizeof(bit_string));
    if ((res = mln_string_dup(&tmp)) == NULL) {
        errno = ENOMEM;
        return -1;
    }
    for (; p < pend; ++p, ++pl) {
        if (((*p)[1] & 0x7f) == fec->pt)
            continue;
        ptr = (*p) + 2;
        mln_bigendian_decode(seq_no, ptr, 2);
        if (seq_no < sn_base)
            continue;
        if (is_long) {
            if (!(mask & ((mln_u64_t)1 << (47 - (seq_no - sn_base)))))
                continue;
            mask &= (~((mln_u64_t)1 << (47 - (seq_no - sn_base))));
        } else {
            if (!(mask & ((mln_u64_t)1 << (15 - (seq_no - sn_base)))))
                continue;
            mask &= (~((mln_u64_t)1 << (15 - (seq_no - sn_base))));
        }

        t = res;
        memcpy(bit_string, *p, 8);
        ptr = bit_string + 8;
        *body_len = *pl - M_FEC_RTP_FIXEDLEN;
        mln_bigendian_encode(*body_len, ptr, 2);
        mln_string_nset(&tmp, bit_string, sizeof(bit_string));
        res = mln_fec_xor(t, &tmp);
        mln_string_free(t);
        if (res == NULL) {
            errno = ENOMEM;
            return -1;
        }
    }
    if (!mask) {
        mln_string_free(res);
        *blen = 0;
        return 0;
    }
    for (seq_no = 0; mask; mask >>= 1, ++seq_no) {
        if (mask & 0x1) {
            if (is_long) seq_no = 47 - seq_no;
            else seq_no = 15 - seq_no;
            seq_no += sn_base;
            mask >>= 1;
            break;
        }
    }
    if (mask) {
        mln_string_free(res);
        errno = EFAULT;
        return -1;
    }

    memcpy(bit_string, fec_packet->data+M_FEC_RTP_FIXEDLEN, sizeof(bit_string));
    mln_string_nset(&tmp, bit_string, sizeof(bit_string));
    t = res;
    res = mln_fec_xor(t, &tmp);
    mln_string_free(t);
    if (res == NULL) {
        errno = ENOMEM;
        return -1;
    }
    ptr = res->data;
    *buf++ = ((*ptr++) & 0x3f) | 0x80;
    *buf++ = *ptr++;
    mln_bigendian_encode(seq_no, buf, 2);
    ptr += 2;
    *buf++ = *ptr++;
    *buf++ = *ptr++;
    *buf++ = *ptr++;
    *buf++ = *ptr++;
    mln_bigendian_encode(ssrc, buf, 4);
    *blen = M_FEC_RTP_FIXEDLEN;
    mln_bigendian_decode(*body_len, ptr, 2);
    mln_string_free(res);
    return 0;
})

MLN_FUNC(static, int, mln_fec_decode_body, \
         (mln_fec_t *fec, mln_string_t *fec_packet, mln_u8ptr_t buf, mln_size_t *blen, \
          mln_u8ptr_t *packets, mln_u16_t *packlen, mln_size_t n, mln_u16_t body_len), \
         (fec, fec_packet, buf, blen, packets, packlen, n, body_len), \
{
    mln_u64_t mask = 0;
    mln_u16_t seq_no, sn_base = 0, protect_len = 0;
    mln_u16_t *pl, is_long = 0;
    mln_u8ptr_t ptr, *p, *pend;
    mln_string_t *t, *res, tmp;
    mln_u8_t body[1472] = {0};

    mln_fec_recover_header_info_get(fec_packet, &sn_base, NULL, &mask, &is_long);
    ptr = fec_packet->data + 22;
    mln_bigendian_decode(protect_len, ptr, 2);
    ptr = is_long? ptr+6: ptr+2;
    if (ptr - fec_packet->data < fec_packet->len)
        memcpy(body+M_FEC_RTP_FIXEDLEN, ptr, protect_len-M_FEC_RTP_FIXEDLEN);
    mln_string_nset(&tmp, body, protect_len);
    if ((res = mln_string_dup(&tmp)) == NULL) {
        errno = ENOMEM;
        return -1;
    }
    p = (mln_u8ptr_t *)packets;
    pend = (mln_u8ptr_t *)packets + n;
    pl = packlen;
    for (; p < pend; ++p, ++pl) {
        if (((*p)[1] & 0x7f) == fec->pt) continue;
        ptr = (*p) + 2;
        mln_bigendian_decode(seq_no, ptr, 2);
        if (seq_no < sn_base) continue;
        if (is_long) {
            if (!(mask & ((mln_u64_t)1 << (47 - (seq_no - sn_base)))))
                continue;
            mask &= (~((mln_u64_t)1 << (47 - (seq_no - sn_base))));
        } else {
            if (!(mask & ((mln_u64_t)1 << (15 - (seq_no - sn_base)))))
                continue;
            mask &= (~((mln_u64_t)1 << (15 - (seq_no - sn_base))));
        }

        t = res;
        mln_string_nset(&tmp, *p, *pl>protect_len? protect_len: *pl);
        res = mln_fec_xor(t, &tmp);
        mln_string_free(t);
        if (res == NULL) {
            errno = ENOMEM;
            return -1;
        }
    }
    memcpy(buf, res->data+M_FEC_RTP_FIXEDLEN, body_len);
    mln_string_free(res);
    *blen += body_len;
    return 0;
})

MLN_FUNC(, mln_fec_result_t *, mln_fec_decode, \
         (mln_fec_t *fec, uint8_t *packets[], uint16_t *packlen, size_t n), \
         (fec, packets, packlen, n), \
{
    mln_fec_result_t *res;
    mln_u8ptr_t *p;
    uint16_t *pl, *plend, fec_cnt = 0, body_len = 0;
    mln_string_t fec_packet = mln_string(NULL), **vec, tmp;
    mln_u8_t buf[M_FEC_R_MAXLEN] = {0};
    mln_size_t blen = 0;

    if (fec == NULL || packets == NULL || packlen == NULL || !n) {
        errno = EINVAL;
        return NULL;
    }
    p = (mln_u8ptr_t *)packets;
    pl = packlen;
    plend = packlen + n;
    for (; pl < plend; ++pl, ++p) {
        if (*pl < M_FEC_RTP_FIXEDLEN || *pl > M_FEC_R_MAXLEN) {
            errno = EINVAL;
            return NULL;
        }
        if (((*p)[1] & 0x7f) == fec->pt) {
            if (*pl < 26 || (((*p)[12]&0x40) && *pl < 30)) {
                errno = EINVAL;
                return NULL;
            }
            mln_string_nset(&fec_packet, *p, *pl);
            ++fec_cnt;
            continue;
        }
    }
    if (fec_cnt > 1) {
        errno = EINVAL;
        return NULL;
    }
    if (!fec_cnt) {
no_recover:
        if ((res = mln_fec_result_new(NULL, 0)) == NULL) {
            errno = ENOMEM;
            return NULL;
        }
        return res;
    }

    if (mln_fec_decode_header(fec, \
                              &fec_packet, \
                              buf, \
                              &blen, \
                              (mln_u8ptr_t *)packets, \
                              packlen, \
                              n, \
                              &body_len) < 0)
    {
        return NULL;
    }
    if (!blen) {/*no need to recover*/
        goto no_recover;
    }
    if (mln_fec_decode_body(fec, \
                            &fec_packet, \
                            buf+blen, \
                            &blen, \
                            (mln_u8ptr_t *)packets, \
                            packlen, \
                            n, \
                            body_len) < 0)
    {
        return NULL;
    } 

    if ((vec = mln_string_vector_new(1)) == NULL) {
        errno = ENOMEM;
        return NULL;
    }
    mln_string_nset(&tmp, buf, blen);
    if ((*vec = mln_string_dup(&tmp)) == NULL) {
        mln_string_vector_free(vec);
        errno = ENOMEM;
        return NULL;
    }
    if ((res = mln_fec_result_new(vec, 1)) == NULL) {
        mln_string_vector_free(vec);
        errno = ENOMEM;
        return NULL;
    }
    return res;
})

