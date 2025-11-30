#include "five_tuple.h"
#include "nat_table.h"
#include <stdlib.h>
#include <stdio.h>

#define GATEWAY_IP_ADDRESS 0xFFFFFFFF

uint16_t generate_gateway_port() {
    // this function will simulate providing a new and unused port between 49152 to 65535
    // for now, it just hard codes it. 
    return 49152;
}

/*
This assumes that the src will be from the inside of the gateway (private side), the dst is outside the gateway (public side)
*/
NatTableEntry* create_connection(NatTable* nat_table, const uint32_t src_ip, const uint16_t src_port, const uint32_t dst_ip, const uint16_t dst_port, uint8_t protocol) {
    // create the five tuple
    FiveTuple fwdKey = {
        .src_ip = src_ip,
        .src_port = src_port,
        .dst_ip = dst_ip, 
        .dst_port = dst_port,
        .protocol = protocol
    };

    // check if the connection already exists 
    NatTableEntry* entry = NatTable_fwd_lookup(nat_table, &fwdKey);
    if (entry) {
        printf("connection already exists!");
        return entry; // this indicates that an entry already exists. Return the entry
    }
    // otherwise, need to create a new mapping

    const uint16_t gateway_port = generate_gateway_port();

    NatTableEntry* entry = NatTable_insert(nat_table, &fwdKey, GATEWAY_IP_ADDRESS, gateway_port);
    return (entry); // if entry failed, returns NULL. otherwise, return the actual NatTableEntry pointer. 
}

/*
This assumes that the src will be from the inside of the gateway (private side), the dst is outside the gateway (public side)
*/
int terminate_connection(NatTable* nat_table, const uint32_t src_ip, const uint16_t src_port, const uint32_t dst_ip, const uint16_t dst_port, uint8_t protocol) {
    FiveTuple fwdKey = {
        .src_ip = src_ip,
        .src_port = src_port,
        .dst_ip = dst_ip, 
        .dst_port = dst_port,
        .protocol = protocol
    };

    // remove function will determine if the entry exists or not. 
    
    int fail = NatTable_remove(nat_table, &fwdKey);
    return fail; // 0 on success, 1 on error
}

int main(void) {
    /*
    TEST SCENARIO:
    inside private address: 192.168.1.60
    inside port: 80

    outside public address: 8.8.8.8
    outside port: 72
    */
    const uint32_t private_ip = 0xA8C00108;
    const uint16_t private_port = 0x50;
    const uint32_t public_ip = 0x8080808;
    const uint16_t public_port = 0x48;

    NatTable* nat_table = create_NAT_table();
    if (!nat_table) return 1; 

    NatTableEntry* entry = create_connection(nat_table, private_ip, private_port, public_ip, public_port, 70);
    if (!entry) return 1;
    printf("Entry created");

    // try to add the same entry: 
    NatTableEntry* repeat_entry = create_connection(nat_table, private_ip, private_port, public_ip, public_port, 70);
    if (!repeat_entry) return 1;
    if (repeat_entry != entry) return 1;


    // remove the conenction
    int fail = terminate_connection(nat_table, private_ip, private_port, public_ip, public_port, 70);
    if (fail) return 1;
    printf("success");
    return 0;
}


