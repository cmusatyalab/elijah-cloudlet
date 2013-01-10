#!/usr/bin/env python
#
# Elijah: Cloudlet Infrastructure for Mobile Computing
# Copyright (C) 2011-2012 Carnegie Mellon University
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of version 2 of the GNU General Public License as published
# by the Free Software Foundation.  A copy of the GNU General Public License
# should have been distributed along with this program in the file
# LICENSE.GPL.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.
#

class Protocol(object):
    KEY_COMMAND                 = "command"
    KEY_META_SIZE               = "meta_size"
    KEY_REQUEST_SEGMENT         = "blob_uri"
    KEY_REQUEST_SEGMENT_SIZE    = "blob_size"
    KEY_FAILED_REAONS           = "reasons"

    # client -> server
    MESSAGE_COMMAND_SEND_META       = 0x11
    MESSAGE_COMMAND_SEND_OVERLAY    = 0x12
    MESSAGE_COMMAND_FINISH          = 0x13

    # server -> client
    MESSAGE_COMMAND_SUCCESS     = 0x01
    MESSAGE_COMMAND_FAIELD      = 0x02
    MESSAGE_COMMAND_ON_DEMAND   = 0x03
