#include <stdint.h>
#include <stdlib.h>
#include "five_tuple.h"


#define NAT_TABLE_SIZE 1024

typedef struct NatTableEntry {
    FiveTuple fwd_key;
    FiveTuple rev_key;

    uint32_t gateway_ip;
    uint16_t gateway_port;

    struct NatTableEntry* next_fwd_entry;
    struct NatTableEntry* next_rev_entry;
} NatTableEntry;

typedef struct {
    NatTableEntry* fwd_table[NAT_TABLE_SIZE];
    NatTableEntry* rev_table[NAT_TABLE_SIZE];
} NatTable;

NatTable* create_NAT_table() {
    NatTable* obj = malloc(sizeof(NatTable));
    if (!obj) return NULL;
    return obj;
}
static uint32_t fiveTuple_hash(const FiveTuple* key) {
    uint32_t h = 2166136261u; // starting hash. Need to add 'u' here because the literal might be interperetted as something else. 
    
    h ^= key->src_ip;   h *= 16777619;
    h ^= key->dst_ip;   h *= 16777619;
    h ^= key->src_port;     h *= 16777619;
    h ^= key->dst_port;     h *= 16777619;
    h ^= key->protocol;     h *= 16777619;

    return h % NAT_TABLE_SIZE;
}

NatTableEntry* NatTable_insert(NatTable* nat_table, FiveTuple* fwd_key, const uint32_t gateway_ip, const uint16_t gateway_port) {
    NatTableEntry* obj = malloc(sizeof(NatTableEntry));
    if (!obj) return NULL;

    FiveTuple rev_key = {
        .src_ip = fwd_key->dst_ip,
        .src_port = fwd_key->dst_port,
        .dst_ip = gateway_ip,
        .dst_port = gateway_port,
        .protocol = fwd_key->protocol
    };

    obj->fwd_key = *fwd_key;
    obj->rev_key = rev_key;
    obj->gateway_ip = gateway_ip;
    obj->gateway_port = gateway_port;

    // hash the fwd_key
    uint32_t fwd_key_hash = fiveTuple_hash(&fwd_key);
    obj->next_fwd_entry = nat_table->fwd_table[fwd_key_hash];
    nat_table->fwd_table[fwd_key_hash] = obj;

    // hash the rev_key
    uint32_t rev_key_hash = fiveTuple_hash(&rev_key);
    obj->next_rev_entry = nat_table->rev_table[rev_key_hash];
    nat_table->rev_table[rev_key_hash] = obj;

    return obj;
}

NatTableEntry* NatTable_fwd_lookup(const NatTable* nat_table, const FiveTuple* fwd_key) {
    // hash the key:
    uint32_t fwd_key_hash = fiveTuple_hash(fwd_key);
    NatTableEntry* entry = nat_table->fwd_table[fwd_key_hash];
    // collision chaining handling
    while (entry) {
        // need to compare the entry's forward key to the fwd key in the input
        if (memcmp(&entry->fwd_key, &fwd_key, sizeof(FiveTuple)) == 0) return entry;
        entry = entry->next_fwd_entry;
    }
    return NULL;
}

NatTableEntry* NatTable_rev_lookup(const NatTable* nat_table, const FiveTuple* rev_key) {
    // has the rev_key:
    uint32_t rev_key_hash = fiveTuple_hash(rev_key);
    NatTableEntry* entry = nat_table->rev_table[rev_key_hash];
    while (entry) {
        if (memcmp(&entry->rev_key, &rev_key, sizeof(FiveTuple)) == 0) return entry;
        entry = entry->next_rev_entry;
    }
    return NULL;
}

int NatTable_remove(NatTable* nat_table, const FiveTuple* fwd_key) {
    // hash this
    uint32_t fwd_key_hash = fiveTuple_hash(&fwd_key);

    NatTableEntry* fwd_to_free = NULL;
    uint32_t gateway_ip = NULL; // NULL initialization until we get find the gateway
    uint16_t gateway_port = NULL;

    // find the entry object so we can get the gateway, and thus we can then remove from the reverse table
    NatTableEntry* entry = nat_table->fwd_table[fwd_key_hash];
    // need to compare the first entry
    if (memcmp(&entry->fwd_key, fwd_key, sizeof(FiveTuple)) == 0) {
        fwd_to_free = entry;
        gateway_ip = fwd_to_free->gateway_ip; // make copy, not pointer
        gateway_port = fwd_to_free->gateway_port;
        nat_table->fwd_table[fwd_key_hash] = entry->next_fwd_entry;
    } else {
        while (entry) {
            if (entry->next_fwd_entry) {
                if (memcmp(&(entry->next_fwd_entry->fwd_key), fwd_key, sizeof(FiveTuple)) == 0) {
                    fwd_to_free = entry->next_fwd_entry;
                    gateway_ip = fwd_to_free->gateway_ip; // make copy, not pointer
                    gateway_port = fwd_to_free->gateway_port;
                    entry->next_fwd_entry = fwd_to_free->next_fwd_entry;
                    break;
                }
            }
            entry = entry->next_fwd_entry;
        }
        if (!entry) return 1;
    }

    NatTableEntry* rev_to_free = NULL;

    // now we can create the reverse key
    FiveTuple rev_key = {
        .src_ip = fwd_key->dst_ip,
        .src_port = fwd_key->dst_port,
        .dst_ip = gateway_ip,
        .dst_port = gateway_port,
        .protocol = fwd_key->protocol
    };

    // hash the reverse key
    uint32_t rev_key_hash = fiveTuple_hash(&rev_key);

    // now unlink the reverse
    entry = nat_table->rev_table[rev_key_hash];

    if (memcmp(&entry->rev_key, &rev_key, sizeof(FiveTuple)) == 0) {
        rev_to_free = entry;
        nat_table->rev_table[rev_key_hash] = entry->next_rev_entry;
    } else {
        while (entry) {
            if (entry->next_rev_entry) { // this outer if conditional is just ot make sure we don't get into a seg fault on the inside. 
                if (memcmp(&entry->next_rev_entry->rev_key, &rev_key, sizeof(FiveTuple)) == 0) {
                    rev_to_free = entry->next_rev_entry;
                    entry->next_rev_entry = rev_to_free->next_rev_entry;
                    break;
                }
            } 
            entry = entry->next_rev_entry;
        }
        if (!entry) return 1;
    }

    // check to ensure that the two entries being freed are the same
    if (fwd_to_free == rev_to_free) {
        free(fwd_to_free); // doesn't matter if we free fwd_to_free or rev_to_free they are the same
        return 0;
    } else {
        return 1; // this indicates that the two entries found are NOT the same!!! this should not happen. 
    }
}