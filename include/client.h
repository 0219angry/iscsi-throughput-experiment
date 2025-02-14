#ifndef _CLIENT_H_
#define _CLIENT_H_

/*
private data for callback function
*/
enum client_state_type {
       STATE_INIT,
       STATE_CONNECTING,
       STATE_CONNECTED,
       STATE_LOGGED_IN,
       STATE_DISCOVERY,
       STATE_LOGGED_OUT,
       STATE_DONE
};

struct client_state {
       int finished;
       enum client_state_type state;
       const char *message;
       int has_discovered_target;
       char *target_name;
       char *target_address;
       int lun;
       int block_size;
};


void generate_write_tasks(struct iscsi_context *iscsi, void *private_data);


#endif
