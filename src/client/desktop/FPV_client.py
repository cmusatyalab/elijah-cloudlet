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
import sys
import socket
from optparse import OptionParser
from datetime import datetime
import time
import cloudlet_client
import cv
from threading import Thread

MOPED_CLIENT_PATH = "/home/krha/cloudlet/src/client/applications/"
application_names = ["moped", "face", "graphics", "speech", "mar", "null"]

camera_index = 0
capture = cv.CaptureFromCAM(camera_index)
FPV_thread_stop = False
font = cv.InitFont(1, 3, 3)

def FPV_init():
    global capture
    #cv.SetCaptureProperty(capture, cv.CV_CAP_PROP_FRAME_WIDTH, 640)
    #cv.SetCaptureProperty(capture, cv.CV_CAP_PROP_FRAME_HEIGHT, 480)

def FPV_capture():
    global camera_index
    global capture

    frame = cv.QueryFrame(capture)
    cv.Flip(frame, None, 1)
    resize = cv.CreateImage((640, 480), frame.depth, frame.nChannels)
    cv.Resize(frame, resize)
    ret_obj = "Test:asad"

    #Display Result
    cv.PutText(frame, "Objects: " + str(ret_obj), (100, 100), font, cv.Scalar(0, 0, 0))
    cv.ShowImage("FPV", frame)
    c = cv.WaitKey(10)
    if c == ord('q'):
        sys.exit(0)

    return frame


def process_command_line(argv):
    global command_type
    global application_names

    parser = OptionParser(usage="usage: ./FPV_client.py -s server -a app_name", version="FPV Desktop Client")
    parser.add_option(
            '-s', '--server', action='store', type='string', dest='server', default="server.krha.kr",
            help="Set Server IP")
    parser.add_option(
            '-a', '--app', action='store', type='string', dest='app',
            help="Set Application name among (%s)" % ",".join(application_names))
    parser.add_option(
            '-p', '--port', dest='port', type='int', default='8021',
            help="Set Server Port")
    settings, args = parser.parse_args(argv)
    if not len(args) == 0:
        parser.error('program takes no command-line arguments; "%s" ignored.' % (args,))
    
    if not settings.app:
        parser.error("Application name is required :%s" % ' '.join(application_names))
    if not settings.app in application_names:
        parser.error("Application name is required :%s" % ' '.join(application_names))

    return settings, args


def run_application(server, app_name):
    if app_name == application_names[0]: # moped
        capture_image = "./.fpv_capture.jpg"
        while True:
            frame = cv.QueryFrame(capture)
            cv.Flip(frame, None, 1)
            resize = cv.CreateImage((640, 480), frame.depth, frame.nChannels)
            cv.Resize(frame, resize)
            cv.SaveImage(capture_image, resize)
            ret_obj = moped_client.send_request(server, 9092, [capture_image])
            print "Return : %s" % (ret_obj)
            print "-"*20

            #Display Result
            cv.PutText(frame, "Objects: " + str(ret_obj), (100, 100), font, cv.Scalar(0xaa, 0xaa, 0xaa))
            cv.ShowImage("FPV", frame)
            c = cv.WaitKey(10)
            if c == ord('q'):
                break;
    else:
        sys.stderr.write("Error, not support app(%s), yet" % app_name)
        sys.exit(1)
    return True


def FPV_thread():
    FPV_init()
    while not FPV_thread_stop:
        FPV_capture()


def main():
    settings, args = process_command_line(sys.argv[1:])

    # FPV camera Thread
    '''
    F_thread = Thread(target=FPV_thread, args=())
    F_thread.start()
    '''
    # Init FPV camera
    FPV_init()

    # Synthesis
    cloudlet_client.synthesis(settings.server, settings.port, settings.app)

    # Run Client
    run_application(settings.server, settings.app)


if __name__ == "__main__":
    if MOPED_CLIENT_PATH not in sys.path:
        sys.path.append(MOPED_CLIENT_PATH)
        import moped_client

    try:
        status = main()
        sys.exit(status)
    except KeyboardInterrupt:
        FPV_thread_stop = True
        sys.exit(1)
