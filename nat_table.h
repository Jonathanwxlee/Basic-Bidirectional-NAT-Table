#ifndef NAT_TABLE_H
#define NAT_TABLE_H

typedef struct Socket Socket;

// how does the NatTable from nattable.c get linked here?
typedef struct NatTable NatTable;

/*
This function dynamically allocates a bidirectional NAT table, and returns the pointer to it
*/
NatTable* create_NAT_table();

/*
This function dynamically allocates a socket, and returns the pointer to it
*/
Socket* create_socket();




#endif
