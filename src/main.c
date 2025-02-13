#include <stdio.h>
#include <stdlib.h>
#include <poll.h>

#include "libiscsi/iscsi.h"
#include "libiscsi/scsi-lowlevel.h"
#include "libiscsi/iscsi-private.h"

#include "client.h"
#include "callback.h"

#define IQN_CLIENT "iqn.2025-02.local:client01"

#define TARGET "192.168.0.26:3260"



int main(int argc, char const *argv[]) {
  struct iscsi_context *iscsi;
  struct pollfd pfd;
  struct client_state clnt;

  memset(&clnt, 0, sizeof(clnt));

  iscsi = iscsi_create_context(IQN_CLIENT);
  if (iscsi == NULL) {
    printf("Failed to create context: %s\n",IQN_CLIENT);
    exit(EXIT_FAILURE);
  }

  if (iscsi_set_alias(iscsi, "client01") != 0) {
    printf("Failed to set alias\n", iscsi->initiator_name);
    exit(EXIT_FAILURE);
  }

  clnt.message = "iSCSI throughput test";
  clnt.has_discovered_target = 0;
  if (iscsi_connect_async(iscsi, TARGET, discoveryconnect_cb, &clnt) != 0) {
    printf("iSCSI failed to connect. %s\n",iscsi_get_error(iscsi));
    exit(EXIT_FAILURE);
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