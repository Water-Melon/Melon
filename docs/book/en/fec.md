## FEC

FEC is defined by RFC5109 and is used to perform forward error correction on RTP packets.

This article does not give an example, and the use is relatively simple. The basic process is: initialize the fec structure -> generate/repair FEC packets -> custom processing -> release the generated/repaired FEC packets -> release the fec structure.



### Header file

```c
#include "mln_fec.h"
```



### Module

`fec`



### Functions/Macros



#### mln_fec_new

```c
mln_fec_t *mln_fec_new(void);
```

Description: Create the `mln_fec_t` structure.

Return value: return the structure pointer successfully, otherwise return `NULL`



#### mln_fec_free

```c
void mln_fec_free(mln_fec_t *fec);
```

Description: Free the `mln_fec_t` structure.

Return value: none



#### mln_fec_encode

```c
mln_fec_result_t *mln_fec_encode(mln_fec_t *fec, uint8_t *packets[], uint16_t packlen[], size_t n, uint16_t group_size);
```

Description: Generate a corresponding FEC message for a given RTP message, where:

- `packets` RTP packet array, each element is an RTP packet. **Note**: Here is the complete RTP message (including header and body).
- `packlen` RTP packet length array, each element corresponds to an element in `packets`.
- `n` is the number of RTP packets.
- `group_size` indicates that one FEC packet is generated for every `group_size` RTP packet, that is, this function may generate multiple FEC packets at a time.

Return value: return `mln_fec_result_t` structure pointer if successful, otherwise return `NULL`



#### mln_fec_result_free

```c
void mln_fec_result_free(mln_fec_result_t *fr);
```

Description: Free the `mln_fec_result_t` structure memory.

Return value: none



#### mln_fec_decode

```c
mln_fec_result_t *mln_fec_decode(mln_fec_t *fec, uint8_t *packets[], uint16_t *packLen, size_t n);
```

Description: Repair RTP packets. FEC uses XOR to repair, so it can only accept the case where only one RTP packet is lost. If more than one RTP packet is lost, it cannot be repaired.

- `packets` An array of FEC packets and RTP packets, each element is an RTP or FEC packet. **Note**: Here is the complete RTP message (including header and body).
- `packlen` An array of packet lengths, each element corresponds to an element in `packets`.
- `n` is the number of packets.

Return value: return `mln_fec_result_t` structure pointer if successful, otherwise return `NULL`



#### mln_fec_set_pt

```c
mln_fec_set_pt(fec,_pt)
```

Description: Set the PT field in the RTP header.

Return value: none



#### mln_fec_get_result

```c
mln_fec_get_result(_result,index,_len)
```

Description: Get the `index`-th packet in `_result` of class behavior `mln_fec_result_t`, `index` starts from 0. `_len` is the length of the obtained packet.

Return value: the memory address of the obtained message content



#### mln_fec_get_result_num

```c
mln_fec_get_result_num(_result)
```

Description: Get the number of results in `_result` of class behavior `mln_fec_result_t`.

Return value: the number of results

