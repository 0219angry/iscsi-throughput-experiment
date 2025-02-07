#ifndef _CLIENT_H_
#define _CLIENT_H_

/*
private data for callback function
*/
struct client_state {
       int finished;
       const char *message;
       int has_discovered_target;
       char *target_name;
       char *target_address;
       int lun;
       int block_size;
};


#endif
