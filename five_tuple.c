#include "five_tuple.h"

// return 1 if equal, 0 if not equal
int FiveTuple_compare(FiveTuple fiveTuple_1, FiveTuple fiveTuple_2) {
    return (fiveTuple_1.src_ip == fiveTuple_2.src_ip &&
            fiveTuple_1.src_port == fiveTuple_2.src_port &&
            fiveTuple_1.dst_ip == fiveTuple_2.dst_ip &&
            fiveTuple_1.dst_port == fiveTuple_2.dst_port &&
            fiveTuple_1.protocol == fiveTuple_2.protocol);
}   