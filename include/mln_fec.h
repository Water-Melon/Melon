
/*
 * Copyright (C) Niklaus F.Schen.
 * RFC 5109
 */
#ifndef __MLN_FEC_H
#define __MLN_FEC_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#if !defined(MSVC)
#include <unistd.h>
#endif
#include "mln_string.h"

#define M_FEC_RTP_FIXEDLEN  12
#define M_FEC_FECHEADER_LEN 10
#define M_FEC_G_MAXLEN      1442
#define M_FEC_R_MAXLEN      1472

typedef struct {
    mln_string_t          **packets;
    mln_size_t              nr_packets;
} mln_fec_result_t;

typedef struct {
    mln_u16_t               seq_no;  /*fec seq_no*/
    mln_u16_t               pt:7;    /*fec's rtp pt field*/
} mln_fec_t;

#define mln_fec_set_pt(fec,_pt)        ((fec)->pt = (_pt))
#define mln_fec_get_result(_result,index,_len)   \
  ((_result)->nr_packets<=(index)? \
    NULL: \
    ((_len)=(_result)->packets[(index)]->len, (uint8_t *)((_result)->packets[(index)]->data)))
#define mln_fec_get_result_num(_result) ((_result)->nr_packets)

extern mln_fec_t *mln_fec_new(void);
extern void mln_fec_free(mln_fec_t *fec);
extern mln_fec_result_t *
mln_fec_encode(mln_fec_t *fec, uint8_t *packets[], uint16_t packlen[], size_t n, uint16_t group_size);
extern void mln_fec_result_free(mln_fec_result_t *fr);
extern mln_fec_result_t *
mln_fec_decode(mln_fec_t *fec, uint8_t *packets[], uint16_t *packlen, size_t n);
#endif
