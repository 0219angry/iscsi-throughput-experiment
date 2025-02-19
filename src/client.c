#define _POSIX_C_SOURCE 199309L
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <execinfo.h>
#include <time.h>

#include "libiscsi/iscsi.h"
#include "libiscsi/scsi-lowlevel.h"
#include "libiscsi/iscsi-private.h"

#include "client.h"

#include "log.h"
#define UNUSED(x) ((void)x)

#define KB 1024
#define MB 1024 * KB
#define GB 1024 * MB

int parse_size(const char *str)
{
  char unit[3] = "";
  int value;

  if (sscanf(str, "%d%2s", &value, unit) < 1) {
      return -1;
  }

  if (strcasecmp(unit, "KB") == 0) {
      return value * KB;
  } else if (strcasecmp(unit, "MB") == 0) {
      return value * MB;
  } else if (strcasecmp(unit, "GB") == 0) {
      return value * GB;
  } else if (unit[0] == '\0') { 
      return value;
  }

  return -1;
}

void read_credentials(struct credentials *creds)
{
  FILE *file = fopen("./credentials.conf", "r");
  if(file == NULL)
  {
    LOG("Failed to open credentials file");
    exit(EXIT_FAILURE);
  }

  char line[256];
  while(fgets(line, sizeof(line), file))
  {
    if (strncmp(line, "alias", 5) == 0)
    {
      creds->alias = strdup(strchr(line, '=') + 1);
    }
    else if (strncmp(line, "portal", 6) == 0)
    {
      creds->portal = strdup(strchr(line, '=') + 1);
    }
    else if (strncmp(line, "username", 8) == 0)
    {
      creds->username = strdup(strchr(line, '=') + 1);
    }
    else if (strncmp(line, "password", 8) == 0)
    {
      creds->password = strdup(strchr(line, '=') + 1);
    }
  }

  fclose(file);
  LOG("Credentials read successfully");
}

void generate_write_tasks(struct iscsi_context *iscsi, struct client_state *clnt, unsigned char *data, iscsi_command_cb cb, void *private_data) {
  int max_segment = (int)iscsi->target_max_recv_data_segment_length;

  for (int i = 0; i < clnt->write_data_size; i += max_segment) {
      struct scsi_task *task = iscsi_write10_task(iscsi, clnt->lun, i / clnt->block_size, data + i, max_segment, max_segment, 0, 0, 0, 1, 0, cb, private_data);
      clnt->generated_write_task_count ++;
      LOG("GENERATE WRITE TASK %d", clnt->generated_write_task_count);
      
      if (task == NULL) {
          LOG("Failed to send write10 command at offset %d", i);
          free(data);
          exit(EXIT_FAILURE);
      }

      if (clnt->generated_write_task_count == clnt->all_write_task_count) {
          break;
      }
  }
}

void signal_handler(int sig) {
  LOG("Received signal %d", sig);

  void *array[10];
  size_t size = backtrace(array, 10);

  char **symbols = backtrace_symbols(array, size);
  LOG("Stack trace:");
  for (size_t i = 0; i < size; i++) {
    LOG("%s", symbols[i]);
  }

  free(symbols);
  exit(EXIT_FAILURE);
}

unsigned char *prepare_write_data(int size) {
  unsigned char *data = malloc(size);
  if (data == NULL) {
      LOG("Failed to allocate memory for data");
      exit(EXIT_FAILURE);
  }

  for (int i = 0; i < size; i++) {
      data[i] = i & 0xff;
  }

  return data;
}

void set_start_time(struct client_state *clnt) {
  clock_gettime(CLOCK_REALTIME, &clnt->write_start_time);
}

void set_end_time(struct client_state *clnt) {
  clock_gettime(CLOCK_REALTIME, &clnt->write_end_time);
}

void stats_traker(struct client_state *clnt) {
  LOG("Write10 successful");
  if(clnt->write_end_time.tv_nsec < clnt->write_start_time.tv_nsec) {
    clnt->write_end_time.tv_sec--;
    clnt->write_end_time.tv_nsec += 1000000000;
  }
  LOG("Elapsed time: %ld.%09ld", \
    clnt->write_end_time.tv_sec - clnt->write_start_time.tv_sec, \
    clnt->write_end_time.tv_nsec - clnt->write_start_time.tv_nsec);
}