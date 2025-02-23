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
#define MB (1024 * KB)
#define GB (1024 * MB)

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

void generate_write_tasks(struct iscsi_context *iscsi, struct client_state *clnt, iscsi_command_cb cb, void *private_data) {
  int data_length = (int)iscsi->target_max_recv_data_segment_length;

  struct scsi_task *task = iscsi_write10_task(iscsi, 
    clnt->lun, 
    clnt->lba,
    clnt->buf,
    clnt->bufsize,
    clnt->block_size,
    0, 0, 1, 1, 0, cb, private_data);
  
  print_progress_bar(clnt->generated_write_data_length, clnt->write_data_size);
  
  clnt->lba = clnt->lba + (data_length / clnt->block_size);
  
  
  if (task == NULL) {
      LOG("Failed to send write10 command.");
      LOG("Error: %s", iscsi_get_error(iscsi));
      free(clnt->buf);
      exit(EXIT_FAILURE);
  }
}

/*

*/
void write_command(struct iscsi_context *iscsi, struct client_state *clnt, iscsi_command_cb cb, void *private_data) {
  int data_size = clnt->write_data_size - clnt->completed_write_data_length;
  if (data_size > (int)iscsi->target_max_recv_data_segment_length) {
    data_size = (int)iscsi->target_max_recv_data_segment_length;
  }
  if(clnt->buf != NULL) {
    free(clnt->buf);
  }
  unsigned char *data = prepare_write_data(data_size);
  clnt->generated_write_data_length += data_size;
  clnt->buf = data;
  clnt->bufsize = data_size;
  generate_write_tasks(iscsi, clnt, cb, private_data);

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
    data[i] = (i % (126 - 32)) + 33;
}

  return data;
}

void set_start_time(struct client_state *clnt) {
  clock_gettime(CLOCK_REALTIME, &clnt->write_start_time);
}

void set_end_time(struct client_state *clnt) {
  clock_gettime(CLOCK_REALTIME, &clnt->write_end_time);
}

void stats_tracker(struct client_state *clnt) {
  
  time_t t = time(NULL);
  struct tm *tm_info = localtime(&t);
  char date_buf[11];
  char time_buf[9];
  strftime(date_buf, sizeof(date_buf), "%Y-%m-%d", tm_info);
  strftime(time_buf, sizeof(time_buf), "%H:%M:%S", tm_info);

  LOG("Write10 successful");
  if(clnt->write_end_time.tv_nsec < clnt->write_start_time.tv_nsec) {
    clnt->write_end_time.tv_sec--;
    clnt->write_end_time.tv_nsec += 1000000000;
  }
  double elapsed_time = (double)(clnt->write_end_time.tv_sec - clnt->write_start_time.tv_sec) + (double)(clnt->write_end_time.tv_nsec - clnt->write_start_time.tv_nsec) / 1000000000;
  LOG("Elapsed time: %f sec", elapsed_time);
  LOG("Write data size: %.2f MB", (double)clnt->write_data_size / MB);
  LOG("Overall write speed: %f MB/s", (double)(clnt->write_data_size / MB) / elapsed_time);

  // append log file
  fprintf(clnt->logfilep, "%s, %s, %d, %f, %f, %s, %s\n",
    date_buf,
    time_buf,
    clnt->write_data_size,
    elapsed_time,
    (double)(clnt->write_data_size / MB) / elapsed_time,
    clnt->target_network_stress,
    clnt->comments
  );

}

FILE *create_logfile() {
  FILE *file = fopen("log/log.csv", "r");

  char header[] = "date, time, write_data_size[B], elapsed_time[s], write_throghput[MB/s], target_network_stress[MB/s], comments\n";
  if (!file) {
    file = fopen("log/log.csv", "a");
    if (!file) {
      LOG("Failed to create log file");
      exit(EXIT_FAILURE);
    }
    fprintf(file, "%s", header);
    LOG("Log file created successfully");
    return file;
  }
  LOG("Log file already exists");
  return file;
}

void print_progress_bar(int progress, int total) {
  int bar_width = 70;
  float progress_ratio = (float)progress / total;
  int pos = bar_width * progress_ratio;

  time_t t = time(NULL);
  struct tm *tm_info = localtime(&t);
  char time_buf[20];
  strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", tm_info);

  printf("\r%s [", time_buf);
  
  for (int i = 0; i < bar_width; i++) {
    if (i < pos) {
      printf("=");
    } else if (i == pos) {
      printf(">");
    } else {
      printf(" ");
    }
  }
  printf("] %d%%", (int)(progress_ratio * 100));
  if (progress == total) {
    printf("\n");
  }

  fflush(stdout);
}