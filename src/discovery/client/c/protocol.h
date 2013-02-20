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

#ifndef CLOUDLET_DISCOVERY_PROTOCOL_H_
#define CLOUDLET_DISCOVERY_PROTOCOL_H_


// client -> server
#define KEY_COMMAND								"command"
#define MESSAGE_COMMAND_SEND_META             0x11
#define MESSAGE_COMMAND_SEND_OVERLAY          0x12
#define MESSAGE_COMMAND_FINISH                0x13
#define MESSAGE_COMMAND_GET_RESOURCE_INFO     0x14
#define MESSAGE_COMMAND_SESSION_CREATE        0x15
#define MESSAGE_COMMAND_SESSION_CLOSE         0x16

// server -> client
#define MESSAGE_COMMAND_SUCCESS               0x01
#define MESSAGE_COMMAND_FAIELD                0x02
#define MESSAGE_COMMAND_ON_DEMAND             0x03

// other keys
#define KEY_ERROR                     "error"
#define KEY_META_SIZE                 "meta_size"
#define KEY_REQUEST_SEGMENT           "blob_uri"
#define KEY_REQUEST_SEGMENT_SIZE      "blob_size"
#define KEY_FAILED_REASON             "reasons"
#define KEY_PAYLOAD                   "payload"
#define KEY_SESSIOIN_ID               "session_id"

// synthesis option
#define KEY_SYNTHESIS_OPTION          		"synthesis_option"
#define SYNTHESIS_OPTION_DISPLAY_VNC   		"option_display_vnc"
#define SYNTHESIS_OPTION_EARLY_START   		"option_early_start"
#define SYNTHESIS_OPTION_SHOW_STATISTICS   	"option_show_statistics"



// resource information
#define MACHINE_NUMBER_TOTAL_CPU            "machine_cpu_num"
#define MACHINE_CLOCK_SPEED                 "machine_cpu_clock_speed_mhz"
#define MACHINE_MEM_TOTAL                   "machine_mem_total_mb"

#define TOTAL_CPU_USE_PERCENT                  "machine_total_cpu_usage_percent"
#define TOTAL_FREE_MEMORY                      "machine_total_free_memory_mb"
#define TOTAL_DISK_READ_BPS                    "machine_disk_read_bytes_per_sec"
#define TOTAL_DISK_WRITE_BPS                   "machine_disk_write_bytes_per_sec"
#define TOTAL_NETWORK_READ_BPS                 "machine_network_read_bytes_per_sec"
#define TOTAL_NETWORK_WRITE_BPS                "machine_newwork_write_bytes_per_sec"


#endif /* CLOUDLET_DISCOVERY_API_H_*/
