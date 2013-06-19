#!/usr/bin/env python 
#
# Cloudlet Infrastructure for Mobile Computing
#
#   Author: Kiryong Ha <krha@cmu.edu>
#
#   Copyright (C) 2011-2013 Carnegie Mellon University
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
#

class Protocol(object):
    #
    # Command List "command": command_number
    #
    KEY_COMMAND                 = "command"
    # client -> server
    MESSAGE_COMMAND_SEND_META           = 0x11
    MESSAGE_COMMAND_SEND_OVERLAY        = 0x12
    MESSAGE_COMMAND_FINISH              = 0x13
    MESSAGE_COMMAND_GET_RESOURCE_INFO   = 0x14
    MESSAGE_COMMAND_SESSION_CREATE      = 0x15
    MESSAGE_COMMAND_SESSION_CLOSE       = 0x16
    # server -> client as return
    MESSAGE_COMMAND_SUCCESS             = 0x01
    MESSAGE_COMMAND_FAIELD              = 0x02
    # server -> client as command
    MESSAGE_COMMAND_ON_DEMAND           = 0x03
    MESSAGE_COMMAND_SYNTHESIS_DONE      = 0x04

    #
    # other keys
    #
    KEY_ERROR                   = "error"
    KEY_META_SIZE               = "meta_size"
    KEY_REQUEST_SEGMENT         = "blob_uri"
    KEY_REQUEST_SEGMENT_SIZE    = "blob_size"
    KEY_FAILED_REASON           = "reasons"
    KEY_PAYLOAD                 = "payload"
    KEY_SESSION_ID             = "session_id"
    KEY_REQUESTED_COMMAND       = "requested_command"

    # synthesis option
    KEY_SYNTHESIS_OPTION        = "synthesis_option"
    SYNTHESIS_OPTION_DISPLAY_VNC = "option_display_vnc"
    SYNTHESIS_OPTION_EARLY_START = "option_early_start"
    SYNTHESIS_OPTION_SHOW_STATISTICS = "option_show_statistics"

