#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include <poll.h>

#include "libiscsi/iscsi.h"
#include "libiscsi/scsi-lowlevel.h"

#include "callback.h"
#include "client.h"

#include "log.h"
#define UNUSED(x) ((void)x)

void discoveryconnect_cb(struct iscsi_context *iscsi, int status, void *command_data, void *private_data)
{
  UNUSED(command_data);

  LOG("Connected to iscsi socket status: 0x%08x", status);

  if (status != 0) {
    LOG("discoveryconnect_cb: connection failed : %s",iscsi_get_error(iscsi));
  }

  LOG("connected, send login command");
  iscsi_set_session_type(iscsi, ISCSI_SESSION_DISCOVERY);
	if (iscsi_login_async(iscsi, discoverylogin_cb, private_data) != 0) {
		LOG("iscsi_login_async failed : %s", iscsi_get_error(iscsi));
		exit(EXIT_FAILURE);
	}

}

void discoverylogin_cb(struct iscsi_context *iscsi, int status, void *command_data, void *private_data)
{
  UNUSED(command_data);

  if (status != 0) {
		LOG("Failed to log in to target. : %s", iscsi_get_error(iscsi));
		exit(EXIT_FAILURE);
	}

	LOG("Logged in to target, send discovery command");
	if (iscsi_discovery_async(iscsi, discovery_cb, private_data) != 0) {
		LOG("failed to send discovery command : %s", iscsi_get_error(iscsi));
		exit(EXIT_FAILURE);
	}
}

void discovery_cb(struct iscsi_context *iscsi, int status, void *command_data, void *private_data)
{
  UNUSED(command_data);

	struct client_state *clnt = (struct client_state *)private_data;
	struct iscsi_discovery_address *addr;

	LOG("discovery callback   status:%04x", status);

	if (status != 0 || command_data == NULL) {
		LOG("Failed to do discovery on target. : %s", iscsi_get_error(iscsi));
		exit(EXIT_FAILURE);
	}

	for(addr = command_data; addr; addr = addr->next) {	
		LOG("Target:%s Address:%s", addr->target_name, addr->portals->portal);
	}

	addr=command_data;
	clnt->has_discovered_target = 1;
	clnt->target_name    = strdup(addr->target_name);
	clnt->target_address = strdup(addr->portals->portal);


	LOG("discovery complete, send logout command");

	if (iscsi_logout_async(iscsi, discoverylogout_cb, private_data) != 0) {
		LOG("iscsi_logout_async failed : %s", iscsi_get_error(iscsi));
		exit(EXIT_FAILURE);
	}
}

void discoverylogout_cb(struct iscsi_context *iscsi, int status, void *command_data, void *private_data)
{
  UNUSED(command_data);

	struct client_state *clnt = (struct client_state *)private_data;
	
	LOG("discovery session logged out, Message from main() was:[%s]", clnt->message);

	if (status != 0) {
		LOG("Failed to logout from target. : %s", iscsi_get_error(iscsi));
		exit(EXIT_FAILURE);
	}

	LOG("disconnect socket");
	if (iscsi_disconnect(iscsi) != 0) {
		LOG("Failed to disconnect old socket");
		exit(EXIT_FAILURE);
	}

	LOG("reconnect with normal login to [%s]", clnt->target_address);
	LOG("Use targetname [%s] when connecting", clnt->target_name);
	if (iscsi_set_targetname(iscsi, clnt->target_name)) {
		LOG("Failed to set target name");
		exit(EXIT_FAILURE);
	}
	if (iscsi_set_alias(iscsi, "client01") != 0) {
		LOG("Failed to add alias");
		exit(EXIT_FAILURE);
	}
	if (iscsi_set_session_type(iscsi, ISCSI_SESSION_NORMAL) != 0) {
		LOG("Failed to set settion type to normal");
		exit(EXIT_FAILURE);
	}

	if (iscsi_connect_async(iscsi, clnt->target_address, normalconnect_cb, clnt) != 0) {
		LOG("iscsi_connect failed : %s", iscsi_get_error(iscsi));
		exit(EXIT_FAILURE);
	}
}

void normalconnect_cb(struct iscsi_context *iscsi, int status, void *command_data, void *private_data)
{
  UNUSED(command_data);

  LOG("reach normalconnect_cb");
}

