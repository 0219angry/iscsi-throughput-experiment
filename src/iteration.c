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

void connect_and_test_iscsi(struct client_state *clnt) {
  struct iscsi_context *iscsi;
  struct pollfd pfd;

  iscsi = iscsi_create_context(IQN_CLIENT);
  if (iscsi == NULL) {
    LOG("Failed to create context: %s", IQN_CLIENT);
    exit(EXIT_FAILURE);
  }
  LOG("Created context: %s", IQN_CLIENT);

  if (iscsi_set_alias(iscsi, ALIAS_CLIENT) != 0) {
    LOG("Failed to set alias: %s", iscsi->initiator_name);
    exit(EXIT_FAILURE);
  }
  LOG("Set alias: %s", iscsi->alias);

  clnt->state = STATE_INIT;
  clnt->message = "iSCSI throughput test";
  clnt->has_discovered_target = 0;
  
  if (iscsi_connect_async(iscsi, clnt->creds.portal, discoveryconnect_cb, clnt) != 0) {
    LOG("iSCSI failed to connect. %s", iscsi_get_error(iscsi));
    exit(EXIT_FAILURE);
  }

  clnt->state = STATE_CONNECTING;
  
  while (clnt->finished == 0) {
    pfd.fd = iscsi_get_fd(iscsi);
    pfd.events = iscsi_which_events(iscsi);

    if (poll(&pfd, 1, -1) < 0) {
      LOG("Poll failed.");
      exit(EXIT_FAILURE);
    }

    if (iscsi_service(iscsi, pfd.events) < 0) {
      LOG("iscsi_service failed with: %s", iscsi_get_error(iscsi));
      break;
    }
  }

  iscsi_destroy_context(iscsi);
}
