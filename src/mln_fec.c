
/*
 * Copyright (C) Niklaus F.Schen.
 * RFC 5109
 */
#include "mln_fec.h"
#include <errno.h>

/*stringVector*/
static inline mln_string_t **mln_stringVector_new(mln_size_t n)
{
    return (mln_string_t **)calloc(n+1, sizeof(mln_string_t *));
}

static inline void mln_stringVector_free(mln_string_t **vec)
{
    if (vec == NULL) return;
    mln_string_t **p = vec;
    for (; *p != NULL; ++p) {
        mln_string_free(*p);
    }
    free(vec);
}


/*
 * operations
 */
static inline mln_string_t *
mln_fec_xor(mln_string_t *data1, mln_string_t *data2)
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
}

/*mln_fec_result_t*/
static mln_fec_result_t *
mln_fec_result_new(mln_string_t **packets, mln_size_t nrPackets)
{
    mln_fec_result_t *fr;
    if ((fr = (mln_fec_result_t *)malloc(sizeof(mln_fec_result_t))) == NULL)
        return NULL;
    fr->packets = packets;
    fr->nrPackets = nrPackets;
    return fr;
}

void mln_fec_result_free(mln_fec_result_t *fr)
{
    if (fr == NULL) return;
    if (fr->packets != NULL) {
        mln_stringVector_free(fr->packets);
    }
    free(fr);
}

/*mln_fec_t*/
mln_fec_t *mln_fec_new(void)
{
    mln_fec_t *fec;

    if ((fec = (mln_fec_t *)malloc(sizeof(mln_fec_t))) == NULL)
        return NULL;
    fec->seqNo = 0;
    fec->pt = 0;
    return fec;
}

void mln_fec_free(mln_fec_t *fec)
{
    if (fec == NULL) return;
    free(fec);
}

/*generation*/
static int
mln_fec_generateFECPacket_FECHeader(mln_fec_t *fec, \
                                    mln_size_t n, \
                                    mln_u8ptr_t *packets, \
                                    mln_u16_t *packLen, \
                                    mln_u8ptr_t buf, \
                                    mln_size_t *len)
{
    mln_u16_t tmp16;
    mln_string_t *ret, tmp, *t;
    mln_u8_t header[10] = {0}, sn[2];
    mln_u8ptr_t *end = packets + n, p;

    sn[0] = packets[0][2];
    sn[1] = packets[0][3];
    mln_string_nSet(&tmp, header, sizeof(header));
    if ((ret = mln_string_dup(&tmp)) == NULL) {
        errno = ENOMEM;
        return -1;
    }

    for (; packets < end; ++packets, ++packLen) {
        t = ret;
        tmp16 = *packLen - M_FEC_RTP_FIXEDLEN;
        memcpy(header, *packets, 8);
        p = header + 8;
        mln_bigendian_encode(tmp16, p, 2);
        mln_string_nSet(&tmp, header, sizeof(header));
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
}

static int
mln_fec_generateFECPacket_FECBody(mln_fec_t *fec, \
                                  mln_size_t n, \
                                  mln_u8ptr_t *packets, \
                                  mln_u16_t *packLen, \
                                  mln_u8ptr_t buf, \
                                  mln_size_t *len)
{
    mln_u8_t c = 0;
    mln_u64_t mask = 0;
    mln_u8ptr_t *end, *p, ptr;
    mln_string_t *t, *ret, tmp;
    mln_u16_t tmpSn, snBase, *pl;

    ptr = *packets + 2;
    mln_bigendian_decode(snBase, ptr, 2);
    ptr = packets[n-1]+2;
    mln_bigendian_decode(tmpSn, ptr, 2);

    /*calc ulp level 0 body*/
    mln_string_nSet(&tmp, &c, 1);
    if ((ret = mln_string_dup(&tmp)) == NULL) {
        errno = ENOMEM;
        return -1;
    }
    end = packets + n;
    for (p = packets, pl = packLen; p < end; ++p, ++pl) {
        t = ret;
        mln_string_nSet(&tmp, *p, *pl);
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
    if (n > 16 || tmpSn-snBase > 16) {
        for (p = packets, end = packets+n; p < end; ++p) {
            ptr = *p + 2;
            mln_bigendian_decode(tmpSn, ptr, 2);
            mask |= ((mln_u64_t)1 << (47 - (tmpSn - snBase)));
        }
        mln_bigendian_encode(mask, buf, 6);
        *len += 6;
    } else {
        for (p = packets, end = packets+n; p < end; ++p) {
            ptr = *p + 2;
            mln_bigendian_decode(tmpSn, ptr, 2);
            mask |= ((mln_u64_t)1 << (15 - (tmpSn - snBase)));
        }
        mln_bigendian_encode(mask, buf, 2);
        *len += 2;
    }

    /*fill ulp level 0 body*/
    memcpy(buf, ret->data+M_FEC_RTP_FIXEDLEN, ret->len-M_FEC_RTP_FIXEDLEN); 
    *len += ret->len-M_FEC_RTP_FIXEDLEN;
    mln_string_free(ret);

    return 0;
}

static mln_string_t *
mln_fec_generateFECPacket(mln_fec_t *fec, \
                          mln_size_t n, \
                          mln_u8ptr_t *packets, \
                          mln_u16_t *packLen)
{
    mln_string_t tmp, *ret;
    mln_size_t len = 0, mod;
    mln_u8_t buf[1472] = {0}, *p;

    if (mln_fec_generateFECPacket_FECHeader(fec, \
                                            n, \
                                            packets, \
                                            packLen, \
                                            buf+M_FEC_RTP_FIXEDLEN, \
                                            &len) < 0)
    {
        return NULL;
    }
    if (mln_fec_generateFECPacket_FECBody(fec, \
                                          n, \
                                          packets, \
                                          packLen, \
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
    mln_bigendian_encode(fec->seqNo, p, 2);
    ++fec->seqNo;
    memcpy(p, &(packets[n-1])[4], 8);/*TS and SSRC*/
    mln_string_nSet(&tmp, buf, len);
    if ((ret = mln_string_dup(&tmp)) == NULL) {
        errno = ENOMEM;
        return NULL;
    }
    return ret;
}

mln_fec_result_t *
mln_fec_encode(mln_fec_t *fec, uint8_t *packets[], uint16_t packLen[], size_t n, uint16_t groupSize)
{
    uint16_t *pl, *plend;
    mln_u8ptr_t *p, *pend;
    mln_string_t **vec, **next;
    mln_fec_result_t *result;

    if (fec == NULL || \
        packets == NULL || \
        packLen == NULL || \
        !n || \
        groupSize > 48 || \
        !groupSize)
    {
        errno = EINVAL;
        return NULL;
    }
    for (pl = packLen, plend = packLen+n; pl < plend; ++pl) {
        if (*pl < M_FEC_RTP_FIXEDLEN || *pl > M_FEC_G_MAXLEN) {
            errno = EINVAL;
            return NULL;
        }
    }

    if ((vec = mln_stringVector_new(n/groupSize)) == NULL) {
        errno = ENOMEM;
        return NULL;
    }
    next = vec;

    p = (mln_u8ptr_t *)packets;
    pend = (mln_u8ptr_t *)packets+n;
    pl = packLen;
    plend = packLen+n;
    for (; p < pend; p+=groupSize, pl+=groupSize, ++next) {
        if ((*next = mln_fec_generateFECPacket(fec, groupSize, p, pl)) == NULL) {
            mln_stringVector_free(vec);
            errno = ENOMEM;
            return NULL;
        }
    }
    if ((result = mln_fec_result_new(vec, n/groupSize)) == NULL) {
        mln_stringVector_free(vec);
        errno = ENOMEM;
        return NULL;
    }
    return result;
}

/*recovery*/
static inline void
mln_fec_recover_getHeaderInfo(mln_string_t *fecPacket, \
                              mln_u16_t *snBase, \
                              mln_u32_t *ssrc, \
                              mln_u64_t *mask, \
                              mln_u16_t *isLong)
{
    mln_u8ptr_t p = fecPacket->data + 8;
    if (ssrc != NULL) {
        mln_bigendian_decode(*ssrc, p, 4);
    } else {
        p += 4;
    }
    if (isLong != NULL) {
        if (*p & 0x40) *isLong = 1;
        else *isLong = 0;
    }
    p += 2;
    if (snBase != NULL) {
        mln_bigendian_decode(*snBase, p, 2);
    } else {
        p += 2;
    }
    p += 8;
    if (mask != NULL) {
        if (*isLong) {
            mln_bigendian_decode(*mask, p, 6);
        } else {
            mln_bigendian_decode(*mask, p, 2);
        }
    }
}

static int mln_fec_decode_header(mln_fec_t *fec, \
                                 mln_string_t *fecPacket, \
                                 mln_u8ptr_t buf, \
                                 mln_size_t *blen, \
                                 mln_u8ptr_t *packets, \
                                 mln_u16_t *packLen, \
                                 mln_size_t n, \
                                 mln_u16_t *bodyLen)
{
    mln_u8ptr_t ptr;
    mln_u64_t mask = 0;
    mln_u32_t ssrc = 0;
    mln_string_t tmp, *t, *res;
    mln_u8ptr_t *p, *pend;
    mln_u16_t *pl, *plend, seqNo, isLong = 0;
    mln_u16_t snBase = 0;
    mln_u8_t bitString[10] = {0};

    mln_fec_recover_getHeaderInfo(fecPacket, &snBase, &ssrc, &mask, &isLong);
    p = packets;
    pend = packets + n;
    pl = packLen;
    plend = packLen + n;
    mln_string_nSet(&tmp, bitString, sizeof(bitString));
    if ((res = mln_string_dup(&tmp)) == NULL) {
        errno = ENOMEM;
        return -1;
    }
    for (; p < pend; ++p, ++pl) {
        if (((*p)[1] & 0x7f) == fec->pt)
            continue;
        ptr = (*p) + 2;
        mln_bigendian_decode(seqNo, ptr, 2);
        if (seqNo < snBase)
            continue;
        if (isLong) {
            if (!(mask & ((mln_u64_t)1 << (47 - (seqNo - snBase)))))
                continue;
            mask &= (~((mln_u64_t)1 << (47 - (seqNo - snBase))));
        } else {
            if (!(mask & ((mln_u64_t)1 << (15 - (seqNo - snBase)))))
                continue;
            mask &= (~((mln_u64_t)1 << (15 - (seqNo - snBase))));
        }

        t = res;
        memcpy(bitString, *p, 8);
        ptr = bitString + 8;
        *bodyLen = *pl - M_FEC_RTP_FIXEDLEN;
        mln_bigendian_encode(*bodyLen, ptr, 2);
        mln_string_nSet(&tmp, bitString, sizeof(bitString));
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
    for (seqNo = 0; mask; mask >>= 1, ++seqNo) {
        if (mask & 0x1) {
            if (isLong) seqNo = 47 - seqNo;
            else seqNo = 15 - seqNo;
            seqNo += snBase;
            mask >>= 1;
            break;
        }
    }
    if (mask) {
        mln_string_free(res);
        errno = EFAULT;
        return -1;
    }

    memcpy(bitString, fecPacket->data+M_FEC_RTP_FIXEDLEN, sizeof(bitString));
    mln_string_nSet(&tmp, bitString, sizeof(bitString));
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
    mln_bigendian_encode(seqNo, buf, 2);
    ptr += 2;
    *buf++ = *ptr++;
    *buf++ = *ptr++;
    *buf++ = *ptr++;
    *buf++ = *ptr++;
    mln_bigendian_encode(ssrc, buf, 4);
    *blen = M_FEC_RTP_FIXEDLEN;
    mln_bigendian_decode(*bodyLen, ptr, 2);
    mln_string_free(res);
    return 0;
}

static int mln_fec_decode_body(mln_fec_t *fec, \
                               mln_string_t *fecPacket, \
                               mln_u8ptr_t buf, \
                               mln_size_t *blen, \
                               mln_u8ptr_t *packets, \
                               mln_u16_t *packLen, \
                               mln_size_t n, \
                               mln_u16_t bodyLen)
{
    mln_u64_t mask = 0;
    mln_u16_t seqNo, snBase, protectLen = 0;
    mln_u16_t *pl, isLong = 0;
    mln_u8ptr_t ptr, *p, *pend;
    mln_string_t *t, *res, tmp;
    mln_u8_t body[1472] = {0};

    mln_fec_recover_getHeaderInfo(fecPacket, &snBase, NULL, &mask, &isLong);
    ptr = fecPacket->data + 22;
    mln_bigendian_decode(protectLen, ptr, 2);
    ptr = isLong? ptr+6: ptr+2;
    if (ptr - fecPacket->data < fecPacket->len)
        memcpy(body+M_FEC_RTP_FIXEDLEN, ptr, protectLen-M_FEC_RTP_FIXEDLEN);
    mln_string_nSet(&tmp, body, protectLen);
    if ((res = mln_string_dup(&tmp)) == NULL) {
        errno = ENOMEM;
        return -1;
    }
    p = (mln_u8ptr_t *)packets;
    pend = (mln_u8ptr_t *)packets + n;
    pl = packLen;
    for (; p < pend; ++p, ++pl) {
        if (((*p)[1] & 0x7f) == fec->pt) continue;
        ptr = (*p) + 2;
        mln_bigendian_decode(seqNo, ptr, 2);
        if (seqNo < snBase) continue;
        if (isLong) {
            if (!(mask & ((mln_u64_t)1 << (47 - (seqNo - snBase)))))
                continue;
            mask &= (~((mln_u64_t)1 << (47 - (seqNo - snBase))));
        } else {
            if (!(mask & ((mln_u64_t)1 << (15 - (seqNo - snBase)))))
                continue;
            mask &= (~((mln_u64_t)1 << (15 - (seqNo - snBase))));
        }

        t = res;
        mln_string_nSet(&tmp, *p, *pl>protectLen? protectLen: *pl);
        res = mln_fec_xor(t, &tmp);
        mln_string_free(t);
        if (res == NULL) {
            errno = ENOMEM;
            return -1;
        }
    }
    memcpy(buf, res->data+M_FEC_RTP_FIXEDLEN, bodyLen);
    mln_string_free(res);
    *blen += bodyLen;
    return 0;
}

mln_fec_result_t *mln_fec_decode(mln_fec_t *fec, uint8_t *packets[], uint16_t *packLen, size_t n)
{
    mln_fec_result_t *res;
    mln_u8ptr_t *p;
    uint16_t *pl, *plend, fecCnt = 0, bodyLen = 0;
    mln_string_t fecPacket = mln_string(NULL), **vec, tmp;
    mln_u8_t buf[M_FEC_R_MAXLEN] = {0};
    mln_size_t blen = 0;

    if (fec == NULL || packets == NULL || packLen == NULL || !n) {
        errno = EINVAL;
        return NULL;
    }
    p = (mln_u8ptr_t *)packets;
    pl = packLen;
    plend = packLen + n;
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
            mln_string_nSet(&fecPacket, *p, *pl);
            ++fecCnt;
            continue;
        }
    }
    if (fecCnt > 1) {
        errno = EINVAL;
        return NULL;
    }
    if (!fecCnt) {
noRecover:
        if ((res = mln_fec_result_new(NULL, 0)) == NULL) {
            errno = ENOMEM;
            return NULL;
        }
        return res;
    }

    if (mln_fec_decode_header(fec, \
                              &fecPacket, \
                              buf, \
                              &blen, \
                              (mln_u8ptr_t *)packets, \
                              packLen, \
                              n, \
                              &bodyLen) < 0)
    {
        return NULL;
    }
    if (!blen) {/*no need to recover*/
        goto noRecover;
    }
    if (mln_fec_decode_body(fec, \
                            &fecPacket, \
                            buf+blen, \
                            &blen, \
                            (mln_u8ptr_t *)packets, \
                            packLen, \
                            n, \
                            bodyLen) < 0)
    {
        return NULL;
    } 

    if ((vec = mln_stringVector_new(1)) == NULL) {
        errno = ENOMEM;
        return NULL;
    }
    mln_string_nSet(&tmp, buf, blen);
    if ((*vec = mln_string_dup(&tmp)) == NULL) {
        mln_stringVector_free(vec);
        errno = ENOMEM;
        return NULL;
    }
    if ((res = mln_fec_result_new(vec, 1)) == NULL) {
        mln_stringVector_free(vec);
        errno = ENOMEM;
        return NULL;
    }
    return res;
}

