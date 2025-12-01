#ifndef NAT_TABLE_H
#define NAT_TABLE_H

#include "five_tuple.h"

#define NAT_TABLE_SIZE 1024

// how does the NatTable from nattable.c get linked here?
typedef struct NatTable NatTable;
typedef struct NatTableEntry NatTableEntry;


/*
This function dynamically allocates a bidirectional NAT table, and returns the pointer to it
*/
NatTable* create_NAT_table();

/*
This function is to look up whether a connection described by a five tuple (or the forward key) already exists. 
*/
NatTableEntry* NatTable_fwd_lookup(const NatTable* nat_table, const FiveTuple* fwd_key);

/*
This function is to insert a new entry into the nat_table.
Return pointer to the entry on success, NULL on error. 
*/
NatTableEntry* NatTable_insert(NatTable* nat_table, FiveTuple* fwd_key, const uint32_t gateway_ip, const uint16_t gateway_port);

/*
This function is to remove an entry from the nat_table. 
Return 0 on success, 1 on error
*/
int NatTable_remove(NatTable* nat_table, const FiveTuple* fwd_key);

#endif
