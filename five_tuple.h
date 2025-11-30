#ifndef FIVE_TUPLE_H
#define FIVE_TUPLE_H

#include <stdint.h>

typedef struct {
    uint32_t src_ip;
    uint16_t src_port;
    uint32_t dst_ip;
    uint16_t dst_port;
    uint8_t protocol;
} FiveTuple;

#endif