/*
 * Elijah: Cloudlet Infrastructure for Mobile Computing
 * Copyright (C) 2011-2012 Carnegie Mellon University
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as published
 * by the Free Software Foundation.  A copy of the GNU General Public License
 * should have been distributed along with this program in the file
 * LICENSE.GPL.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 *      Author: Kiryong Ha (krha@cmu.edu)
 */

#ifndef CLOUDLET_DISCOVERY_API_H_
#define CLOUDLET_DISCOVERY_API_H_

#define CLOUDLET_DOMAIN		"search.findcloudlet.org"
#define CLOUDLET_PORT		8021
#define RET_FAILED  		0
#define RET_SUCCESS 		1

/*
 * Data Struct
 */
typedef struct cloudlet_resource_t {
	int number_total_cpu;
	int number_sockets;
	int number_cores_psocket;
	int number_threads_pcore;
	float cpu_clock_speed_mhz;
	int mem_total_mb;
    int mem_free_mb;
    float cpu_usage_percent;
} cloudlet_resource_t;

typedef struct cloudlet_t{
	char ip_v4[16];
	int port_number;
	cloudlet_resource_t hw_resource;
	struct cloudlet_t *next;
} cloudlet_t;

char discovery_error[256];


/*
 * Methods
 */
int find_nearby_cloudlets(cloudlet_t **cloudlet_list, int *size);
int get_cloudlet_info(cloudlet_t *cloudlet);
long associate_with_cloudlet(cloudlet_t *cloudlet);
int disassociate(cloudlet_t *cloudlet, long session_id);

/*
 * printout method
 */
void print_cloudlet_t(cloudlet_t *cloudlet);
void print_cloudlets(cloudlet_t *cloudlet_list);

#endif /* CLOUDLET_DISCOVERY_API_H_*/
