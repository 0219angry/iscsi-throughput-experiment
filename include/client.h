#include <sys/time.h>
#include <pthread.h>
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
       STATE_READY,
       STATE_DONE
};

struct credentials {
       const char *alias;
       const char *portal;
       const char *username;
       const char *password;
};
struct client_state {
       int finished;
       enum client_state_type state;
       struct credentials creds;
       const char *message;
       int write_data_size;
       int has_discovered_target;
       char *target_name;
       char *target_address;
       int lun;
       int block_size;
       int all_write_task_count;
       int completed_write_task_count;
       int generated_write_task_count;
       struct timespec write_start_time;
       struct timespec write_end_time;
       pthread_mutex_t mutex;
};


int parse_size(const char *str);
void read_credentials(struct credentials *creds);
void generate_write_tasks(struct iscsi_context *iscsi, struct client_state *clnt, unsigned char *data, iscsi_command_cb cb, void *private_data);
void signal_handler(int sig);
unsigned char *prepare_write_data(int size);

void set_start_time(struct client_state *clnt);
void set_end_time(struct client_state *clnt);
void stats_traker(struct client_state *clnt);

#endif
