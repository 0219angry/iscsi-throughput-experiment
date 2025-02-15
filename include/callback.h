#ifndef _CALLBACK_H_
#define _CALLBACK_H_

void discoveryconnect_cb(struct iscsi_context *iscsi, int status, void *command_data, void *private_data);
void discoverylogin_cb(struct iscsi_context *iscsi, int status, void *command_data, void *private_data);
void discovery_cb(struct iscsi_context *iscsi, int status, void *command_data, void *private_data);
void discoverylogout_cb(struct iscsi_context *iscsi, int status, void *command_data, void *private_data);
void normalconnect_cb(struct iscsi_context *iscsi, int status, void *command_data, void *private_data);
void normallogin_cb(struct iscsi_context *iscsi, int status, void *command_data, void *private_data);
void reportluns_cb(struct iscsi_context *iscsi, int status, void *command_data, void *private_data);

#endif
