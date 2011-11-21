#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <stropts.h>
#include <net/if.h>
#include <linux/sockios.h>
#include <netdb.h>

/*
 * Get System TS
 */
inline unsigned long long int rdtsc() {
	unsigned long long int x;
	__asm__ volatile (".byte 0x0f, 0x31" : "=A" (x));
	return x;
}

/*
 * Read exact amount of data from fd
 */
int read_full(int fd, char *buffer, int count) {
	int got, done;
	done = 0;
	char *buf = buffer;
	while (count > 0) {
		got = read(fd, buf, count);
		if (got == 0)
			return done;
		if (got < 0)
			return -1;
		done += got;
		count -= got;
		buf = (char*) buf + got;
	}
	return done;
}

/*
 * Write exact amount of data at fd
 */
int write_full(int fd, const char *buffer, int count) {
	int got, done;
	done = 0;
	char *buf = buffer;
	while (count > 0) {
		got = write(fd, buf, count);
		if (got == 0)
			return 0;
		if (got < 0) {
			return -1;
		}
		done += got;
		count -= got;
		buf = (char*) buf + got;
	}
	return done;
}

/*
 * Get own IP
 */
#define BUFFERSIZE 	1024
#define MAX_NIC		10
const char *myip() {
	const char * localip = "0.0.0.0";
	return localip;
	struct ifconf ifc;
	struct ifreq ifr[MAX_NIC];
	static char ip[BUFFERSIZE];
	int s;
	int nNumIFs;
	int i;
	int count;
	int max = 2;
	int cmd = SIOCGIFCONF;

	max++;

	ifc.ifc_len = sizeof ifr;
	ifc.ifc_ifcu.ifcu_req = ifr;

	if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket");
		exit(1);
	}

#if defined(_AIX)
	cmd = CSIOCGIFCONF;
#endif

	if ((s = ioctl(s, cmd, &ifc)) < 0) {
		perror("ioctl");
		exit(1);
	}

	nNumIFs = ifc.ifc_len / sizeof(struct ifreq);
	count = 0;
	strcpy(ip, localip);
	for (i = 0; i < nNumIFs; i++) {
		struct in_addr addr;
		if (ifc.ifc_ifcu.ifcu_req[i].ifr_ifru.ifru_addr.sa_family != AF_INET) {
			continue;
		}

		addr
				= ((struct sockaddr_in *) &ifc.ifc_ifcu.ifcu_req[i].ifr_ifru.ifru_addr)->sin_addr;
		if (addr.s_addr == htonl(0x7f000001)) {
			continue;
		}
		strcpy(ip, inet_ntoa(addr));
	}
	return ip;
}

/*
 * Another getting IP address method
 * This will print out all IPs assigned to all interfaces
 * Then we can find public IP address discarding private information
 */
const char* print_addresses() {
	int domains[] = { AF_INET };
	int s;
	struct ifconf ifconf;
	struct ifreq ifr[50];
	int ifs;
	int i;

	s = socket(AF_INET, SOCK_STREAM, 0);
	if (s < 0) {
		perror("socket");
		return 0;
	}

	ifconf.ifc_buf = (char *) ifr;
	ifconf.ifc_len = sizeof ifr;

	if (ioctl(s, SIOCGIFCONF, &ifconf) == -1) {
		perror("ioctl");
		return 0;
	}

	char* myIP = (char*)malloc(15 * sizeof(char));
	ifs = ifconf.ifc_len / sizeof(ifr[0]);
	printf("interfaces = %d:\n", ifs);
	for (i = 0; i < ifs; i++) {
		char ip[INET_ADDRSTRLEN];
		struct sockaddr_in *s_in = (struct sockaddr_in *) &ifr[i].ifr_addr;

		if (!inet_ntop(AF_INET, &s_in->sin_addr, ip, sizeof(ip))) {
			perror("inet_ntop");
			return 0;
		}

//		printf("%s - %s\n", ifr[i].ifr_name, ip);
		if((strncmp(ip, "127.0.0.1", 3) != 0) && (strncmp(ip, "0.0.0.0", 1) != 0) && strncmp(ip, "192.168", 7) != 0){
			strcpy(myIP, ip);
			break;
		}
	}

	close(s);
	return myIP;
}

/*
 * Get next argument removing empty space
 */
int get_next_num() {
	int result = 0;
	char tmp;

	while (1) {
		tmp = getchar();
		if (tmp == ' ' || tmp == '\t')
			continue;
		else if (tmp == '\n')
			return -1;
		break;
	}

	while (1) {
		if (tmp < '0' || tmp > '9')
			break;
		result *= 10;
		result += tmp - '0';
		tmp = getchar();
	}

	return result;
}

/*
 * Nagle disable
 */
void nagles_off(int sock) {
	int one = 1;
	if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char *) &one, sizeof(one))
			< 0) {
		fprintf(stderr, "Nagles Fail...\n");
	}
}

/*
 * Convert to non-blocking mode
 */
void nonblock(int sockfd) {
	int opts;
	opts = fcntl(sockfd, F_GETFL);
	if (opts < 0) {
		perror("fcntl(F_GETFL)\n");
		exit(1);
	}
	opts = (opts | O_NONBLOCK);
	if (fcntl(sockfd, F_SETFL, opts) < 0) {
		perror("fcntl(F_SETFL)\n");
		exit(1);
	}
}

/*
 * Make local udp socket
 * Failed when return value is -1
 */
int make_local_udp_socket(int *port) {
	struct sockaddr_in addr;
	int sock;

	sock = socket(PF_INET, SOCK_DGRAM, 0);
	if (sock < 0) {
		return -1;
	}

	memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(*port);

	if (bind(sock, (struct sockaddr_in*) &addr, sizeof(struct sockaddr_in)) < 0) {
		return -1;
	}

	if (*port != 0)
		return sock;

	struct sockaddr_in adr_inet;
	socklen_t len_inet = (socklen_t) sizeof(adr_inet);
	getsockname(sock, (struct sockaddr*) &adr_inet, &len_inet);
	*port = (unsigned) ntohs(adr_inet.sin_port);
	return sock;
}

/*
 * Make local TCP port
 * failed when return value is -1
 */
int read_full_non_block(int fd, char *buffer, int count);
int make_local_tcp_server_socket(int *port, int num_of_client) {
	int server_sock;
	struct sockaddr_in server_addr;

	server_sock = socket(PF_INET, SOCK_STREAM, 0);
	if (server_sock == -1) {
		return -1;
	}

	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(*port);

	int option = 1;
	setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, (void*) &option,
			sizeof(option));

	if (bind(server_sock, (struct sockaddr*) &server_addr, sizeof(server_addr))
			< 0) {
		return -1;
	}

	if (listen(server_sock, num_of_client) < 0) {
		return -1;
	}

	if (*port != 0)
		return server_sock;

	struct sockaddr_in adr_inet;
	socklen_t len_inet = (socklen_t) sizeof(adr_inet);
	getsockname(server_sock, (struct sockaddr*) &adr_inet, &len_inet);
	*port = (unsigned) ntohs(adr_inet.sin_port);

	return server_sock;
}

/*
 * Make current process as Daemon
 */
static void makedaemon(int mode) {
	pid_t pid;

	if ((pid = fork()) < 0)
		exit(0);

	else if (pid != 0)
		exit(0);

	if (mode == 0) {
		close(0);
		close(1);
		close(2);
	}
	setsid();
}

/*
 * Convert int from big endian to little endian
 */
int endian_swap_int(int data) {
	int ret = ((data >> 24) & 0x000000FF) | ((data << 8) & 0x00FF0000) | ((data
			>> 8) & 0x0000FF00) | ((data << 24) & 0xFF000000);
	return ret;
}
