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

/*
This function is to compare the five tuples. Compares each member of the five tuple. Returns 1 on equal comparison, 0 on unequal comparison
*/
int FiveTuple_compare(FiveTuple fiveTuple_1, FiveTuple fiveTuple_2);

#endif