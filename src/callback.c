#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

#include <poll.h>

#include "libiscsi/iscsi.h"
#include "libiscsi/scsi-lowlevel.h"
#include "libiscsi/iscsi-private.h"

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
		exit(EXIT_FAILURE);
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
	if (iscsi_set_alias(iscsi, clnt->creds.alias) != 0) {
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
	struct client_state *clnt = (struct client_state *)private_data;
	LOG("Connected to iscsi socket");

	if (status != 0) {
		LOG("Connection failed status: %d", status);
	}

	LOG("Connected, send login command");

	if (iscsi_set_header_digest(iscsi, ISCSI_HEADER_DIGEST_CRC32C_NONE) != 0) {
		LOG("Failed to set header digest to normal");
		exit(EXIT_FAILURE);
	}

	if (iscsi_set_target_username_pwd(iscsi, clnt->creds.username, clnt->creds.password) != 0) {
		LOG("Failed to set auth");
		exit(EXIT_FAILURE);
	}
	if (iscsi_login_async(iscsi, normallogin_cb, private_data) != 0) {
		LOG("iscsi_login_async failed : %s", iscsi_get_error(iscsi));
		exit(EXIT_FAILURE);
	}
}

void normallogin_cb(struct iscsi_context *iscsi, int status, void *command_data, void *private_data)
{
	UNUSED(command_data);

	struct client_state *clnt = (struct client_state *)private_data;
	UNUSED(clnt);

	if (status != 0) {
		LOG("Failed to log in to target. : %s", iscsi_get_error(iscsi));
		exit(EXIT_FAILURE);
	}

	LOG("Logged in normal session, send reportluns");
	if (iscsi_reportluns_task(iscsi, 0, 16, reportluns_cb, private_data) == NULL) {
		LOG("failed to send reportluns command : %s", iscsi_get_error(iscsi));
		exit(EXIT_FAILURE);
	}
}

void reportluns_cb(struct iscsi_context *iscsi, int status, void *command_data, void *private_data)
{
	struct scsi_task *task = command_data;
	struct scsi_reportluns_list *list;
	int full_report_size;
	int i;

	struct client_state *clnt = (struct client_state *)private_data;

	if (status != SCSI_STATUS_GOOD) {
		LOG("Failed to reportluns with : %s", iscsi_get_error(iscsi));
		exit(EXIT_FAILURE);
	}

	full_report_size = scsi_datain_getfullsize(task);

	LOG("REPORTLUNS datasize %d", task->datain.size);
	LOG("REPORTLUNS status %d", status);
	LOG("REPORTLUNS full report luns data size %d", full_report_size);

	if (full_report_size > task->datain.size) {
		LOG("We did not get all the data we need in reportluns, ask again");
		if (iscsi_reportluns_task(iscsi, 0, full_report_size, reportluns_cb, private_data) == NULL) {
			LOG("failed to send reportluns command");
			scsi_free_scsi_task(task);
			exit(EXIT_FAILURE);
		}
		scsi_free_scsi_task(task);
		return;
	}

	list = scsi_datain_unmarshall(task);
	if (list == NULL) {
		LOG("Failed to unmarshall reportluns data");
		scsi_free_scsi_task(task);
		exit(EXIT_FAILURE);
	}

	for (i=0; i<(int)list->num; i++) {
		LOG("(%d) LUN %d found.", i, list->luns[i]);
		clnt->lun = list->luns[i];
	}

	LOG("REPORTLUNS done, send testunitready to lun %d", clnt->lun);
	if (iscsi_testunitready_task(iscsi, clnt->lun, testunitready_cb, private_data) == NULL) {
		LOG("failed to send testunitready command");
		scsi_free_scsi_task(task);
		exit(EXIT_FAILURE);
	}
	scsi_free_scsi_task(task);
}

void testunitready_cb(struct iscsi_context *iscsi, int status, void *command_data, void *private_data)
{
	struct client_state *clnt = (struct client_state *)private_data;
	struct scsi_task *task = command_data;
	struct scsi_reportluns_list *list;
	UNUSED(list);

	if (status == SCSI_STATUS_CHECK_CONDITION) {
		LOG("First testunitready failed with sense key:%d ascq:%04x", task->sense.key, task->sense.ascq);
		if (task->sense.key == SCSI_SENSE_UNIT_ATTENTION && task->sense.ascq == SCSI_SENSE_ASCQ_BUS_RESET) {
			LOG("target device just came online, try again");

			if (iscsi_testunitready_task(iscsi, clnt->lun, testunitready_cb, private_data) == NULL) {
				LOG("failed to send testunitready command");
				scsi_free_scsi_task(task);
				exit(EXIT_FAILURE);
			}
		}
		scsi_free_scsi_task(task);
		return;
	}

	LOG("testunitready status %d", status);
	LOG("Do an inquiry on lun %d", clnt->lun);
	if(iscsi_inquiry_task(iscsi, clnt->lun, 0, 0, 255, inquiry_cb, private_data) == NULL) {
		LOG("failed to send inquiry command");
		scsi_free_scsi_task(task);
		exit(EXIT_FAILURE);
	}
	scsi_free_scsi_task(task);
}

void inquiry_cb(struct iscsi_context *iscsi, int status, void *command_data, void *private_data)
{
	struct scsi_task *task = command_data;
	struct scsi_inquiry_standard *inq;
	struct client_state *clnt = (struct client_state *)private_data;

	if (status == SCSI_STATUS_CHECK_CONDITION) {
		LOG("Inquiry failed with sense key:%d ascq:%04x", task->sense.key, task->sense.ascq);
		exit(EXIT_FAILURE);
	}

	inq = scsi_datain_unmarshall(task);
	if (inq == NULL) {
		LOG("Failed to unmarshall inquiry data");
		scsi_free_scsi_task(task);
		exit(EXIT_FAILURE);
	}

	LOG("Inquiry data:");
	LOG("  Device type: %d", inq->device_type);
	LOG("  VendorId   : %s", inq->vendor_identification);
	LOG("  Product    : %s", inq->product_identification);
	LOG("  Revision   : %s", inq->product_revision_level);

	LOG("Send modesense6 command");
	if (iscsi_modesense6_task(iscsi, clnt->lun, 0, SCSI_MODESENSE_PC_CURRENT, SCSI_MODEPAGE_RETURN_ALL_PAGES, 0, 4, modesense6_cb, private_data) == NULL) {
		LOG("failed to send modesense6 command");
		scsi_free_scsi_task(task);
		exit(EXIT_FAILURE);
	}
	scsi_free_scsi_task(task);
}

void modesense6_cb(struct iscsi_context *iscsi, int status, void *command_data, void *private_data)
{
	struct scsi_task *task = command_data;
	struct scsi_modesense6 *ms6;
	struct client_state *clnt = (struct client_state *)private_data;
	int full_size;

	if (status == SCSI_STATUS_CHECK_CONDITION) {
		LOG("Modesense6 failed with sense key:%d ascq:%04x", task->sense.key, task->sense.ascq);
		exit(EXIT_FAILURE);
	}

	full_size = scsi_datain_getfullsize(task);

	if (full_size > task->datain.size) {
		LOG("We did not get all the data we need in modesense6, ask again");
		if (iscsi_modesense6_task(iscsi, clnt->lun, 0, SCSI_MODESENSE_PC_CURRENT, SCSI_MODEPAGE_RETURN_ALL_PAGES, 0, full_size, modesense6_cb, private_data) == NULL) {
			LOG("failed to send modesense6 command");
			scsi_free_scsi_task(task);
			exit(EXIT_FAILURE);
		}
		scsi_free_scsi_task(task);
		return;
	}

	LOG("MODESENSE6 successful. Data size %d", task->datain.size);

	ms6 = scsi_datain_unmarshall(task);
	if (ms6 == NULL) {
		LOG("Failed to unmarshall modesense6 data");
		scsi_free_scsi_task(task);
		exit(EXIT_FAILURE);
	}

	LOG("Send READCAPACITY10 command");
	if (iscsi_readcapacity10_task(iscsi, clnt->lun, 0, 0, readcapacity10_cb, private_data) == NULL) {
		LOG("failed to send readcapacity10 command");
		scsi_free_scsi_task(task);
		exit(EXIT_FAILURE);
	}
	scsi_free_scsi_task(task);
}

void readcapacity10_cb(struct iscsi_context *iscsi, int status, void *command_data, void *private_data)
{
	struct scsi_task *task = command_data;
	struct scsi_readcapacity10 *rc10;
	struct client_state *clnt = (struct client_state *)private_data;
	int full_size;

	int i;

	if (status == SCSI_STATUS_CHECK_CONDITION) {
		LOG("Readcapacity10 failed with sense key:%d ascq:%04x", task->sense.key, task->sense.ascq);
		exit(EXIT_FAILURE);
	}

	full_size = scsi_datain_getfullsize(task);
	if (full_size < task->datain.size) {
		LOG("We did not get all the data we need in readcapacity10, ask again");
		if (iscsi_readcapacity10_task(iscsi, clnt->lun, 0, 0, readcapacity10_cb, private_data) == NULL) {
			LOG("failed to send readcapacity10 command");
			scsi_free_scsi_task(task);
			exit(EXIT_FAILURE);
		}
		scsi_free_scsi_task(task);
		return;
	}

	rc10 = scsi_datain_unmarshall(task);
	if (rc10 == NULL) {
		LOG("Failed to unmarshall readcapacity10 data");
		scsi_free_scsi_task(task);
		exit(EXIT_FAILURE);
	}

	clnt->block_size = rc10->block_size;
	LOG("READCAPACITY10 successful.");
	LOG("  LBA       : %d",rc10->lba);
	LOG("  Block size: %d", rc10->block_size);


	scsi_free_scsi_task(task);

	unsigned char *data = malloc((int)iscsi->target_max_recv_data_segment_length);
	LOG("MAX_RECV_DATA_SEGMENT_LENGTH: %d", (int)iscsi->target_max_recv_data_segment_length);
	if (data == NULL) {
		LOG("Failed to allocate memory for data");
		exit(EXIT_FAILURE);
	}

	for (i = 0; i < (int)iscsi->target_max_recv_data_segment_length; i++) {
		data[i] = i & 0xff;
	}

	LOG("Generate write tasks. %d bytes.", clnt->write_data_size);
	gettimeofday(&clnt->write_start_time, NULL);

	clnt->all_write_task_count = clnt->write_data_size / 512;
	pthread_mutex_lock(&clnt->mutex);
	clnt->completed_write_task_count = 0;
	pthread_mutex_unlock(&clnt->mutex);

	for (int i = 0; i < (int)iscsi->target_max_recv_data_segment_length; i += 512) {
		task = iscsi_write10_task(iscsi, clnt->lun, i / 512, data + i, 512, 512, 0, 0, 0, 1, 0, write10_cb, private_data);
		clnt->generated_write_task_count++;	
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

	free(data);
}

void write10_cb(struct iscsi_context *iscsi, int status, void *command_data, void *private_data)
{
	UNUSED(iscsi);
	struct scsi_task *task = command_data;
	struct client_state *clnt = (struct client_state *)private_data;
	int i;

	if (status == SCSI_STATUS_CHECK_CONDITION) {
		LOG("Write10 failed with sense key:%d ascq:%04x", task->sense.key, task->sense.ascq);
		exit(EXIT_FAILURE);
	}
	if (status != SCSI_STATUS_GOOD) {
		LOG("Write10 failed with status:%d", status);
		exit(EXIT_FAILURE);
	}

	pthread_mutex_lock(&clnt->mutex);
	clnt->completed_write_task_count++;
	pthread_mutex_unlock(&clnt->mutex);
	
	LOG("Command Sequence Number: %d(%d/%d)",task->cmdsn, clnt->completed_write_task_count, clnt->all_write_task_count);
	
	// genrate next write tasks
	if (clnt->completed_write_task_count == clnt->generated_write_task_count && clnt->generated_write_task_count < clnt->all_write_task_count) {
		unsigned char *data = malloc(iscsi->target_max_recv_data_segment_length);
		if (data == NULL) {
			LOG("Failed to allocate memory for data");
			exit(EXIT_FAILURE);
		}
	
		for (i = 0; i < (int)iscsi->target_max_recv_data_segment_length; i++) {
			data[i] = i & 0xff;
		}

		for (int i = 0; i < (int)iscsi->target_max_recv_data_segment_length; i += 512) {
			task = iscsi_write10_task(iscsi, clnt->lun, clnt->generated_write_task_count, data + clnt->generated_write_task_count * 512, 512, 512, 0, 0, 0, 1, 0, write10_cb, private_data);
			clnt->generated_write_task_count++;	
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
	
	// all write tasks are completed
	if(clnt->completed_write_task_count == clnt->all_write_task_count) {
		gettimeofday(&clnt->write_end_time, NULL);
		LOG("Write10 successful");
		LOG("Elapsed time: %ld.%06ld", clnt->write_end_time.tv_sec - clnt->write_start_time.tv_sec, clnt->write_end_time.tv_usec - clnt->write_start_time.tv_usec);
		scsi_free_scsi_task(task);
		clnt->finished = 1;
	}
}