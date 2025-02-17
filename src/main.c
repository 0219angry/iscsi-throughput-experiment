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

  if (argc != 2) {
    LOG("Usage: %s", argv[0]);
    exit(EXIT_FAILURE);
  }
  clnt.write_data_size = parse_size(argv[1]);
  if (clnt.write_data_size < 0) {
    LOG("Invalid write data size: %s", argv[1]);
    exit(EXIT_FAILURE);
  }

  read_credentials(&clnt.creds);

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