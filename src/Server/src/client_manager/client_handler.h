
#ifndef __CLIENT_HANLDER_H_INCLUDED__
#define __CLIENT_HANLDER_H_INCLUDED__

#include "../protocol.h"

void parse_req_vmlist(int sock_fd, const char* json_string);
void parse_req_transfer(int sock_fd, const char* json_string);
void parse_req_launch(int sock_fd, const char* json_string);
void parse_req_stop(int sock_fd, const char* json_string);

//int launch_VM(const char *tmp_overlay_disk_path, const char *tmp_overlay_mem_path, VM_Info *overlayVM, VM_Info *baseVM);

#endif  //  __CLIENT_HANLDER_H_INCLUDED__

