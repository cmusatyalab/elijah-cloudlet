
#ifndef KRHA_LIB_SOCKET_H_
#define KRHA_LIB_SOCKET_H_

inline unsigned long long int rdtsc();
int read_full(int fd, char *buffer, int count);
int write_full(int fd, char *buffer, int count);
const char *myip();
void next_line();
int get_next_num();
void nagles_off(int sock);
int make_local_udp_socket(int *port);
int make_local_tcp_server_socket(int *port, int num_of_client);
void nonblock(int sockfd);

int endian_swap_int(int data);

#endif /* ANALYSIS_H_ */
