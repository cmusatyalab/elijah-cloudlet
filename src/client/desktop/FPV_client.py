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
from optparse import OptionParser
import socket
from datetime import datetime
import time
import cloudlet_client
import cv
from threading import Thread
import threading

CLIENT_PATH = "/home/krha/cloudlet/src/client/applications/"
application_names = ["moped", "face", "graphics", "speech", "mar", "null"]
application_ports = {"moped":9092, "face":9876, "graphics":9093, "speech":6789, "mar":9094}

WINDOW_NAME = "FPV"
camera_index = 0
capture = cv.CaptureFromCAM(camera_index)
FPV_thread_stop = False
font = cv.InitFont(cv.CV_FONT_HERSHEY_SIMPLEX, 1, 1)
overlay_message = "Initiaiting"
latest_frame = ''
frame_lock = threading.Lock()


def FPV_init():
    global capture
    global latest_frame
    #cv.SetCaptureProperty(capture, cv.CV_CAP_PROP_FRAME_WIDTH, 640)
    #cv.SetCaptureProperty(capture, cv.CV_CAP_PROP_FRAME_HEIGHT, 480)


def FPV_close():
    cv.DestroyWindow(WINDOW_NAME)


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
    global overlay_message

    # Connection
    try:
        print "Connecting to (%s, %d).." % (server, application_ports[app_name])
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.setblocking(True)
        sock.connect((server, 9092))
    except socket.error, msg:
        sys.stderr.write("Error, %s\n" % msg[1])
        overlay_message = "Error, %s\n" % msg[1]
        return

    capture_image = "./.fpv_capture.jpg"
    while True:
        # Retreive Image
        if FPV_thread_stop:
            break;
        frame_lock.acquire()
        try:
            cv.SaveImage(capture_image, latest_frame)
        except Exception:
            frame_lock.release()
            break;
        frame_lock.release()

        # Application request
        start_time = time.time()
        image_bin = open(capture_image, 'rb').read();
        if app_name == application_names[0]: # moped
            ret_obj = moped_client.moped_request(sock, image_bin)
            overlay_message = "Return : %s (latency:%02.03f)" % (ret_obj, time.time()-start_time)
        elif app_name == application_names[1]: # face
            ret_obj = moped_client.moped_request(sock, image_bin)
            overlay_message = "Return : %s (latency:%02.03f)" % (ret_obj, time.time()-start_time)
        else:
            overlay_message = "Does not support %s, yet" % (app_name)

        print overlay_message
        print "-"*20

    return True


def FPV_thread():
    global camera_index
    global capture
    global WINDOW_NAME
    global latest_frame
    global FPV_thread_stop
    global overlay_message

    FPV_init()

    cv.NamedWindow(WINDOW_NAME, cv.CV_WINDOW_NORMAL)
    cv.MoveWindow(WINDOW_NAME, 0, 0)
    while True:
        frame = cv.QueryFrame(capture)
        cv.Flip(frame, None, 1)

        #copy to buffer
        frame_lock.acquire()
        if not latest_frame:
            latest_frame = cv.CreateImage((640, 480), frame.depth, frame.nChannels)
        cv.Resize(frame, latest_frame)
        frame_lock.release()


        #Display Result
        cv.PutText(frame, overlay_message, (10, 50), font, cv.Scalar(0,0,0))
        cv.ShowImage(WINDOW_NAME, frame)
        cv.ResizeWindow(WINDOW_NAME, 200, 100)
        cv.NamedWindow(WINDOW_NAME, cv.CV_WINDOW_NORMAL);
        cv.SetWindowProperty(WINDOW_NAME, 0, cv.CV_WINDOW_FULLSCREEN);
        c = cv.WaitKey(10)
        if c == ord('q'):
            break

    print "FPV Thread is finished"
    FPV_thread_stop = True
    FPV_close()

def main():
    global frame_lock
    settings, args = process_command_line(sys.argv[1:])

    # FPV camera Thread
    F_thread = Thread(target=FPV_thread, args=())
    F_thread.start()

    # Synthesis
    cloudlet_client.synthesis(settings.server, settings.port, settings.app)

    # Run Client
    run_application(settings.server, settings.app)


if __name__ == "__main__":
    if CLIENT_PATH not in sys.path:
        sys.path.append(CLIENT_PATH)
        import moped_client
    try:
        status = main()
        sys.exit(status)
    except KeyboardInterrupt:
        sys.exit(1)
