
#ifndef __CLIENT_HANLDER_H_INCLUDED__
#define __CLIENT_HANLDER_H_INCLUDED__


void parse_req_vmlist(int sock_fd, const char* json_string);
void parse_req_transfer(int sock_fd, const char* json_string);
void parse_req_launch(int sock_fd, const char* json_string);
void parse_req_stop(int sock_fd, const char* json_string);

#endif  //  __CLIENT_HANLDER_H_INCLUDED__

