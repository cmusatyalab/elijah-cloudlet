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

