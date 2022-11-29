// See README.md for references
// Lifted and mildly adapted from [5]
#include <stdint.h>

// Binds to INADDR_ANY and listens on a port.
// returns a fd as a positive integer if successful
// asserts if passed an invalid port number
// returns -2 if opening the socket failed
// returns -3 if binding the socket failed
// returns -4 if listening failed
int create_listen_ipv4_socket(uint16_t port, int backlog);
