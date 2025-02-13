#include <stdio.h>
#include <stdlib.h>
#include <poll.h>

#include "libiscsi/iscsi.h"
#include "libiscsi/scsi-lowlevel.h"

#include "callback.h"

void discoveryconnect_cb(struct iscsi_context *iscsi, int status, void *command_data, void *private_data){
  
}
