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

#include "log.h"
#define UNUSED(x) ((void)x)

#define IQN_CLIENT "iqn.2025-02.com.example:client01"
#define ALIAS_CLIENT "client01"


int main(int argc, char const *argv[]) {
  LOG("Start iscsi throughput experiments");

  signal(SIGINT, signal_handler);

  struct iscsi_context *iscsi;
  struct pollfd pfd;
  struct client_state clnt;

  memset(&clnt, 0, sizeof(clnt));

  if (argc < 4 && argc > 5) {
    LOG("Usage: %s", argv[0]);
    LOG("%s <test count> <write data size> <target network stress> (<comments>)", argv[0]);
    exit(EXIT_FAILURE);
  }
  char *endptr;
  int test_count = strtol(argv[1], endptr, 10);
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

  iscsi = iscsi_create_context(IQN_CLIENT);
  if (iscsi == NULL) {
    LOG("Failed to create context: %s",IQN_CLIENT);
    exit(EXIT_FAILURE);
  } else {
    LOG("Created context: %s",IQN_CLIENT);
  }

  if (iscsi_set_alias(iscsi, ALIAS_CLIENT) != 0) {
    LOG("Failed to set alias: %s", iscsi->initiator_name);
    exit(EXIT_FAILURE);
  } else {
    LOG("Set alias: %s", iscsi->alias);
  }

  clnt.state = STATE_INIT;

  clnt.message = "iSCSI throughput test";
  clnt.has_discovered_target = 0;
  if (iscsi_connect_async(iscsi, clnt.creds.portal, discoveryconnect_cb, &clnt) != 0) {
    LOG("iSCSI failed to connect. %s",iscsi_get_error(iscsi));
    exit(EXIT_FAILURE);
  }

  clnt.state = STATE_CONNECTING;


  // loop for writing
  while (clnt.finished == 0) {
    pfd.fd = iscsi_get_fd(iscsi);
    pfd.events = iscsi_which_events(iscsi);

    if (poll(&pfd, 1, -1) < 0) {
      LOG("Poll failed.");
      exit(EXIT_FAILURE);
    }

    if(iscsi_service(iscsi, pfd.events) < 0) {
      LOG("iscsi_service failed with : %s", iscsi_get_error(iscsi));
      break;
    }

  }

  // free resources
	iscsi_destroy_context(iscsi);

	if (clnt.target_name != NULL) {
		free(clnt.target_name);
	}
	if (clnt.target_address != NULL) {
		free(clnt.target_address);
	}

  exit(EXIT_SUCCESS);
}