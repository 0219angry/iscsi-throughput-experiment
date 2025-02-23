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
       FILE *logfilep;
       const char *message;
       const char *target_network_stress;
       const char *comments;
       int write_data_size;
       int has_discovered_target;
       char *target_name;
       char *target_address;
       int bufsize;
       unsigned char *buf;
       int lun;
       int lba;
       int block_size;
       int completed_write_data_length;
       int generated_write_data_length;
       struct timespec write_start_time;
       struct timespec write_end_time;
       pthread_mutex_t mutex;
};


int parse_size(const char *str);
void read_credentials(struct credentials *creds);
void write_command(struct iscsi_context *iscsi, struct client_state *clnt, iscsi_command_cb cb, void *private_data);
void generate_write_tasks(struct iscsi_context *iscsi, struct client_state *clnt, iscsi_command_cb cb, void *private_data);
void signal_handler(int sig);
unsigned char *prepare_write_data(int size);

void set_start_time(struct client_state *clnt);
void set_end_time(struct client_state *clnt);
void stats_tracker(struct client_state *clnt);
void print_progress_bar(int progress, int total);
FILE *create_logfile();

#endif
