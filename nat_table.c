#include <stdint.h>
#include <stdlib.h>


#define NAT_TABLE_SIZE 1024
#define GATEWAY_IP_ADDR 0xFFFFFFFF // 255.255.255.255

typedef struct {
    uint32_t ipAddr;
    uint16_t port;
} Socket;

typedef struct {
    Socket src;
    Socket dst;
    uint8_t protocol;
} NatTableKey;

typedef struct NatTableEntry {
    NatTableKey fwd_key;
    NatTableKey rev_key;

    Socket gateway;

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

Socket* create_socket(const uint32_t ipAddr, const uint16_t port) {
    Socket* obj = malloc(sizeof(Socket));
    if (!obj) return NULL;
    obj->ipAddr = ipAddr;
    obj->port = port;
    return obj;
}

static uint32_t NatTableKey_hash(const NatTableKey* key) {
    uint32_t h = 2166136261u; // starting hash. Need to add 'u' here because the literal might be interperetted as something else. 
    
    h ^= key->src.ipAddr;   h *= 16777619;
    h ^= key->dst.ipAddr;   h *= 16777619;
    h ^= key->src.port;     h *= 16777619;
    h ^= key->dst.port;     h *= 16777619;
    h ^= key->protocol;     h *= 16777619;

    return h % NAT_TABLE_SIZE;
}

Socket generate_gateway_socket() {
    // this function will mimic generating a port from the dynamic range of ports (49152-65535)
    // but hard coded for now
    Socket gateway_socket = {
        .ipAddr = GATEWAY_IP_ADDR,
        .port = 49152
    };
    return gateway_socket;
}

int NatTable_insert(NatTable* nat_table, const Socket* internal, const Socket* external, const uint8_t protocol) {
    NatTableEntry* obj = malloc(sizeof(NatTableEntry));
    if (!obj) return 1;

    // create the forward key:
    NatTableKey fwd_key = {
        .src = *internal,
        .dst = *external,
        .protocol = protocol
    };

    // here we need to generate a gateway
    Socket new_gateway_socket = generate_gateway_socket();

    // create the reverse key:
    NatTableKey rev_key = {
        .src = *external,
        .dst = new_gateway_socket,
        .protocol = protocol
    };

    obj->fwd_key = fwd_key;
    obj->rev_key = rev_key;
    obj->gateway = new_gateway_socket;

    // hash the fwd_key
    uint32_t fwd_key_hash = NatTableKey_hash(&fwd_key);
    obj->next_fwd_entry = nat_table->fwd_table[fwd_key_hash];
    nat_table->fwd_table[fwd_key_hash] = obj;

    // hash the rev_key
    uint32_t rev_key_hash = NatTableKey_hash(&rev_key);
    obj->next_rev_entry = nat_table->rev_table[rev_key_hash];
    nat_table->rev_table[rev_key_hash] = obj;

    return 1;
}

NatTableEntry* NatTable_fwd_lookup(const NatTable* nat_table, const NatTableKey* fwd_key) {
    // hash the key:
    uint32_t fwd_key_hash = NatTableKey_hash(fwd_key);
    NatTableEntry* entry = nat_table->fwd_table[fwd_key_hash];
    // collision chaining handling
    while (entry) {
        NatTableEntry entry_obj = *entry; // just so we don't have to keep dereferencing since it is costly
        NatTableKey fwd_key_obj = *fwd_key; // just so we don't have to keep dereferencing since it is costly
        // need to compare the entry's forward key to the fwd key in the input
        if (
            memcmp(&entry_obj.fwd_key.src, &fwd_key_obj.src, sizeof(Socket)) == 0 &&
            memcmp(&entry_obj.fwd_key.dst, &fwd_key_obj.dst, sizeof(Socket)) == 0 &&
            entry_obj.fwd_key.protocol == fwd_key_obj.protocol
        ) return entry;
        entry = entry->next_fwd_entry;
    }
    return NULL;
}

NatTableEntry* NatTable_rev_lookup(const NatTable* nat_table, const NatTableKey* rev_key) {
    // has the rev_key:
    uint32_t rev_key_hash = NatTableKey_hash(rev_key);
    NatTableEntry* entry = nat_table->rev_table[rev_key_hash];
    while (entry) {
        NatTableEntry entry_obj = *entry;
        NatTableKey rev_key_obj = *rev_key;
        if (
            memcmp(&entry_obj.rev_key.src, &rev_key_obj.src, sizeof(Socket)) == 0 &&
            memcmp(&entry_obj.rev_key.dst, &rev_key_obj.dst, sizeof(Socket)) == 0 &&
            entry_obj.rev_key.protocol == rev_key_obj.protocol
        ) return entry;
        entry = entry->next_rev_entry;
    }
    return NULL;
}

int natTable_remove(NatTable* nat_table, const Socket* internal, const Socket* external, const uint8_t protocol) {
    // remember to remove from both tables!

    // generate the forward key:
    NatTableKey fwd_key = {
        .src = *internal,
        .dst = *external,
        .protocol = protocol
    };
    // hash this
    uint32_t fwd_key_hash = NatTableKey_hash(&fwd_key);

    NatTableEntry* fwd_to_free = NULL;
    Socket gateway = {0}; // dummy 0 initialization until we get find the gateway socket

    // find the entry object so we can get the gateway, and thus we can then remove from the reverse table
    NatTableEntry* entry = nat_table->fwd_table[fwd_key_hash];
    // need to compare the first entry
    if (
        memcmp(&entry->fwd_key.src, internal, sizeof(Socket)) == 0 &&
        memcmp(&entry->fwd_key.dst, external, sizeof(Socket)) == 0 &&
        entry->fwd_key.protocol == protocol
    ) {
        fwd_to_free = entry;
        gateway = fwd_to_free->gateway; // make copy, not pointer
        nat_table->fwd_table[fwd_key_hash] = entry->next_fwd_entry;
    } else {
        while (entry) {
            if (entry->next_fwd_entry) {
                if (
                    memcmp(&(entry->next_fwd_entry->fwd_key.src), internal, sizeof(Socket)) == 0 &&
                    memcmp(&(entry->next_fwd_entry->fwd_key.dst), external, sizeof(Socket)) == 0 &&
                    entry->next_fwd_entry->fwd_key.protocol == protocol
                ) {
                    fwd_to_free = entry->next_fwd_entry;
                    gateway = fwd_to_free->gateway; // make copy, not pointer
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
    NatTableKey rev_key = {
        .src = *external,
        .dst = gateway,
        .protocol = protocol
    };

    // hash the reverse key
    uint32_t rev_key_hash = NatTableKey_hash(&rev_key);

    // now unlink the reverse
    entry = nat_table->rev_table[rev_key_hash];

    if (
        memcmp(&entry->rev_key.src, external, sizeof(Socket)) == 0 &&
        memcmp(&entry->rev_key.dst, &gateway, sizeof(Socket)) == 0 &&
        entry->rev_key.protocol == protocol
    ) {
        rev_to_free = entry;
        nat_table->rev_table[rev_key_hash] = entry->next_rev_entry;
    } else {
        while (entry) {
            if (entry->next_rev_entry) { // this outer if conditional is just ot make sure we don't get into a seg fault on the inside. 
                if (
                    memcmp(&entry->next_rev_entry->rev_key.src, external, sizeof(Socket)) == 0 &&
                    memcmp(&entry->next_rev_entry->rev_key.dst, &gateway, sizeof(Socket)) == 0 &&
                    entry->rev_key.protocol == protocol
                ) {
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
