#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libiscsi/iscsi.h"
#include "libiscsi/scsi-lowlevel.h"
#include "libiscsi/iscsi-private.h"

#include "client.h"

#include "log.h"
#define UNUSED(x) ((void)x)


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

void generate_write_tasks(struct iscsi_context *iscsi, void *private_data)
{

}