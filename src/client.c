#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <execinfo.h>

#include "libiscsi/iscsi.h"
#include "libiscsi/scsi-lowlevel.h"
#include "libiscsi/iscsi-private.h"

#include "client.h"

#include "log.h"
#define UNUSED(x) ((void)x)

#define KB 1024
#define MB 1024 * KB
#define GB 1024 * MB

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

void generate_write_tasks(struct iscsi_context *iscsi, void *private_data)
{
  UNUSED(iscsi);
  UNUSED(private_data);
  LOG("Generate write tasks here");
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