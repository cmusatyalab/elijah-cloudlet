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

from synthesis_protocol import Protocol as protocol
from rapid_app import app_synthesis

ip = "cloudlet.krha.kr"
#ip = "192.168.2.2"
port = 8021
options = {
        protocol.SYNTHESIS_OPTION_DISPLAY_VNC: False,
        protocol.SYNTHESIS_OPTION_EARLY_START: False
        }

app_synthesis(ip, port, "moped_access_32", options=options)
app_synthesis(ip, port, "moped_access_1024", options=options)
app_synthesis(ip, port, "moped_access_1048576", options=options)
app_synthesis(ip, port, "moped_linear_32", options=options)
app_synthesis(ip, port, "moped_linear_1024", options=options)
app_synthesis(ip, port, "moped_linear_1048576", options=options)

app_synthesis(ip, port, "face_access_32", options=options)
app_synthesis(ip, port, "face_access_1024", options=options)
app_synthesis(ip, port, "face_access_1048576", options=options)
app_synthesis(ip, port, "face_linear_32", options=options)
app_synthesis(ip, port, "face_linear_1024", options=options)
app_synthesis(ip, port, "face_linear_1048576", options=options)

app_synthesis(ip, port, "speech_access_32", options=options)
app_synthesis(ip, port, "speech_access_1024", options=options)
app_synthesis(ip, port, "speech_access_1048576", options=options)
app_synthesis(ip, port, "speech_linear_32", options=options)
app_synthesis(ip, port, "speech_linear_1024", options=options)
app_synthesis(ip, port, "speech_linear_1048576", options=options)

app_synthesis(ip, port, "graphics_access_32", options=options)
app_synthesis(ip, port, "graphics_access_1024", options=options)
app_synthesis(ip, port, "graphics_access_1048576", options=options)
app_synthesis(ip, port, "graphics_linear_32", options=options)
app_synthesis(ip, port, "graphics_linear_1024", options=options)
app_synthesis(ip, port, "graphics_linear_1048576", options=options)

app_synthesis(ip, port, "graphics_access_32", options=options)
app_synthesis(ip, port, "graphics_access_1024", options=options)
app_synthesis(ip, port, "graphics_access_1048576", options=options)
app_synthesis(ip, port, "graphics_linear_32", options=options)
app_synthesis(ip, port, "graphics_linear_1024", options=options)
app_synthesis(ip, port, "graphics_linear_1048576", options=options)

app_synthesis(ip, port, "mar_access_32", options=options)
app_synthesis(ip, port, "mar_access_1024", options=options)
app_synthesis(ip, port, "mar_access_1048576", options=options)
app_synthesis(ip, port, "mar_linear_32", options=options)
app_synthesis(ip, port, "mar_linear_1024", options=options)
app_synthesis(ip, port, "mar_linear_1048576", options=options)

