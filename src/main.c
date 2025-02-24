#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include <poll.h>

#include "libiscsi/iscsi.h"
#include "libiscsi/scsi-lowlevel.h"
#include "libiscsi/iscsi-private.h"

#include "client.h"
#include "callback.h"
#include "iteration.h"

#include "log.h"
#define UNUSED(x) ((void)x)

#define IQN_CLIENT "iqn.2025-02.com.example:client01"
#define ALIAS_CLIENT "client01"


int main(int argc, char const *argv[]) {
  LOG("Start iscsi throughput experiments");

  signal(SIGINT, signal_handler);

  struct client_state clnt;

  memset(&clnt, 0, sizeof(clnt));

  if (argc < 4 || argc > 5) {
    LOG("Usage: %s", argv[0]);
    LOG("%s <test count> <write data size> <target network stress> (<comments>)", argv[0]);
    exit(EXIT_FAILURE);
  }
  char *endptr = NULL;
  int test_count = strtol(argv[1], &endptr, 10);
  if (*endptr != '\0') {
    LOG("Invalid test count: %s", argv[1]);
    exit(EXIT_FAILURE);
  }
  LOG("testing throughputs, %d times", test_count);

  clnt.write_data_size = parse_size(argv[2]);
  if (clnt.write_data_size < 0) {
    LOG("Invalid write data size: %s", argv[2]);
    exit(EXIT_FAILURE);
  }
  LOG("write data size: %d", clnt.write_data_size);
  
  clnt.target_network_stress = strdup(argv[3]);
  LOG("network stress on target node: %s[MB/s]", clnt.target_network_stress);
  if (argc == 5) {
    clnt.comments = strdup(argv[4]);
    LOG("logfile comment: %s", clnt.comments);
  } else {
    clnt.comments = "";
  }


  read_credentials(&clnt.creds);

  clnt.logfilep = create_logfile();

  struct client_state default_clnt;
  memcpy(&default_clnt, &clnt, sizeof(clnt));

  for (int i = 0; i < test_count; i++) {
    memcpy(&clnt, &default_clnt, sizeof(clnt));
    connect_and_test_iscsi(&clnt);
  }

  exit(EXIT_SUCCESS);
}