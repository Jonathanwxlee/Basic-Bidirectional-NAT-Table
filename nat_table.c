#include <stdint.h>
#include <stdlib.h>


#define NAT_TABLE_SIZE 1024

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


int NatTable_insert(NatTable* nat_table, const Socket* internal, const Socket* external, const Socket* gateway, const uint8_t protocol) {
    NatTableEntry* obj = malloc(sizeof(NatTableEntry));
    if (!obj) return 1;

    // create the forward key:
    NatTableKey fwd_key = {
        .src = *internal,
        .dst = *external,
        .protocol = protocol
    };

    // create the reverse key:
    NatTableKey rev_key = {
        .src = *external,
        .dst = *gateway,
        .protocol = protocol
    };

    obj->fwd_key = fwd_key;
    obj->rev_key = rev_key;

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

NatTableEntry* NatTable_fwd_lookup(NatTable* nat_table, NatTableKey* fwd_key) {
    // hash the key:
    uint32_t fwd_key_hash = NatTableKey_hash(fwd_key);
    NatTableEntry* entry = nat_table->fwd_table[fwd_key_hash];
    // collision chaining handling
    while (entry) {
        NatTableEntry entry_obj = *entry; // just so we don't have to keep dereferencing since it is costly
        NatTableKey fwd_key_obj = *fwd_key; // just so we don't have to keep dereferencing since it is costly
        // need to compare the entry's forward key to the fwd key in the input
        if (
            entry_obj.fwd_key.src.ipAddr == fwd_key_obj.src.ipAddr &&
            entry_obj.fwd_key.src.port == fwd_key_obj.src.port &&
            entry_obj.fwd_key.dst.ipAddr == fwd_key_obj.dst.ipAddr &&
            entry_obj.fwd_key.dst.port == fwd_key_obj.dst.port &&
            entry_obj.fwd_key.protocol == fwd_key_obj.protocol
        ) return entry;
        entry = entry->next_fwd_entry;
    }
    return NULL;
}

NatTableEntry* NatTable_rev_lookup(NatTable* nat_table, NatTableKey* rev_key) {
    // has the rev_key:
    uint32_t rev_key_hash = NatTableKey_hash(rev_key);
    NatTableEntry* entry = nat_table->rev_table[rev_key_hash];
    while (entry) {
        NatTableEntry entry_obj = *entry;
        NatTableKey rev_key_obj = *rev_key;
        if (
            entry_obj.rev_key.src.ipAddr == rev_key_obj.src.ipAddr &&
            entry_obj.rev_key.src.port == rev_key_obj.src.port &&
            entry_obj.rev_key.dst.ipAddr == rev_key_obj.dst.ipAddr &&
            entry_obj.rev_key.dst.port == rev_key_obj.dst.port &&
            entry_obj.rev_key.protocol == rev_key_obj.protocol
        ) return entry;
        entry = entry->next_rev_entry;
    }
    return NULL;
}




