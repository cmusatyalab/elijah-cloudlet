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

import os
import subprocess
import select
import threading
import multiprocessing
import time
import sys
from cloudlet import log as logging

LOG = logging.getLogger(__name__)


# system.py is built at install time, so pylint may fail to import it.
# Also avoid warning on variable name.
# pylint: disable=F0401,C0103
# pylint: enable=F0401,C0103

class VMNetFSError(Exception):
    pass


class VMNetFS(threading.Thread):
    FUSE_TYPE_DISK      =   "disk"
    FUSE_TYPE_MEMORY    =   "memory"

    def __init__(self, bin_path, args, **kwargs):
        self.vmnetfs_path = bin_path
        self._args = '%d\n%s\n' % (len(args),
                '\n'.join(a.replace('\n', '') for a in args))
        self._pipe = None
        self.mountpoint = None
        self.stop = threading.Event()

        # fuse can handle on-demand fetching
        # TODO: passing these argument through kwargs
        self.demanding_queue = kwargs.get("demanding_queue", None)
        self.meta_info = kwargs.get("meta_info", None)
        threading.Thread.__init__(self, target=self.fuse_read)

    def fuse_read(self):
        wait_statistics = list()
        if (self.meta_info != None) and (self.demanding_queue != None):
            memory_overlay_dict = dict()
            disk_overlay_dict = dict()
            from Configuration import Const
            for blob in self.meta_info[Const.META_OVERLAY_FILES]:
                overlay_url = blob[Const.META_OVERLAY_FILE_NAME]
                memory_chunks = blob[Const.META_OVERLAY_FILE_MEMORY_CHUNKS]
                for chunk in memory_chunks:
                    memory_overlay_dict[chunk] = overlay_url
                disk_chunks = blob[Const.META_OVERLAY_FILE_DISK_CHUNKS]
                for chunk in disk_chunks:
                    disk_overlay_dict[chunk] = overlay_url

        while(not self.stop.wait(0.0001)):
            self._running = True
            oneline = self.proc.stdout.readline()
            if len(oneline.strip()) > 0:
                request_split = oneline.split(",")
                if (self.demanding_queue != None) and \
                        (len(request_split) > 0) and \
                        (request_split[0].find("REQUEST") > 0):
                    overlay_type = request_split[1].split(":")[1].strip()
                    chunk = long(request_split[2].split(":")[1])
                    if overlay_type == VMNetFS.FUSE_TYPE_DISK:
                        url = disk_overlay_dict.get(chunk, None)
                    elif overlay_type == VMNetFS.FUSE_TYPE_MEMORY:
                        url = memory_overlay_dict.get(chunk, None)
                    else:
                        msg = "FUSE type does not match : %s" % overlay_type
                        raise VMNetFSError(msg)

                    if url == None:
                        msg = "Cannot find matching blob with chunk(%ld)" % chunk
                        raise VMNetFSError(msg)
                    #LOG.debug("requesting chunk(%ld) at %s" % (chunk, url))
                    self.demanding_queue.put(url)
                elif (len(request_split) > 0) and (request_split[0].find("STATISTICS-WAIT") > 0):
                    type_name, overlay_type = request_split[1].split(":")
                    chunk_name, chunk = request_split[2].split(":")
                    wait_name, wait_time = request_split[3].split(":")
                    data = {type_name:overlay_type, chunk_name:long(chunk.strip()), wait_name:float(wait_time.strip())}
                    wait_statistics.append(data)

        if len(wait_statistics) > 0:
            total_wait_time = 0.0
            for item in wait_statistics:
                total_wait_time += item['time']
            LOG.info("%d chunks waited for synthesizing for avg %f s, total: %f s" % \
                    (len(wait_statistics), total_wait_time/len(wait_statistics), total_wait_time))
        else:
            LOG.info("NO chunks has been waited at FUSE")
        self._running = False
        LOG.info("close Fuse Exec thread")

    def fuse_write(self, data):
        self._pipe.write(data + "\n")
        self._pipe.flush()

    # pylint is confused by the values returned from Popen.communicate()
    # pylint: disable=E1103
    def launch(self):
        read, write = os.pipe()
        try:
            self.proc = subprocess.Popen([self.vmnetfs_path], stdin=read,
                    stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                    close_fds=True)
            self._pipe = os.fdopen(write, 'w')
            self._pipe.write(self._args)
            self._pipe.flush()
            out = self.proc.stdout.readline()
            self.mountpoint = out.strip()
        except:
            if self._pipe is not None:
                self._pipe.close()
            else:
                os.close(write)
            raise
        finally:
            pass
            #os.close(read)
        if os.path.exists(self.mountpoint) == False:
            msg = "Failed to mount FUSE file system\n"
            msg += "  1. Check FUSE permission of /dev/fuse to have '666'\n"
            msg += "  2. Check FUSE configuration at /etc/fuse.conf to have 'allow_others' option\n"
            msg += "     and permission of '422'\n"
            raise VMNetFSError(msg)
    # pylint: enable=E1103

    def terminate(self):
        self.stop.set()
        if self._pipe is not None:
            LOG.info("Fuse close pipe")
            # invalid formated string will shutdown fuse
            self.fuse_write("terminate")
            self._pipe.close()
            self._pipe = None


class StreamMonitor(threading.Thread):
    DISK_MODIFY = "DISK_MODIFY"
    DISK_ACCESS = "DISK_ACCESS"
    MEMORY_ACCESS = "MEMORY_ACCESS"

    def __init__(self):
        self.epoll = select.epoll()
        self.stream_dict = dict()
        self._running = False
        self.stop = threading.Event()
        self.modified_chunk_dict = dict()
        self.disk_access_chunk_list = list()
        self.mem_access_chunk_list = list()
        threading.Thread.__init__(self, target=self.io_watch)

    def add_path(self, path, name):
        # We need to set O_NONBLOCK in open() because FUSE doesn't pass
        # through fcntl()
        LOG.info("start monitoring at %s" % path)
        fd = os.open(path, os.O_RDONLY | os.O_NONBLOCK)
        self.stream_dict[fd] = {'name':name, 'buf':'', 'path':path}
        self.epoll.register(fd, select.EPOLLIN | select.EPOLLOUT | select.EPOLLPRI)

    def del_path(self, name):
        # We need to set O_NONBLOCK in open() because FUSE doesn't pass
        # through fcntl()
        for fileno, item in self.stream_dict.items():
            monitor_path = item['path']
            monitor_name = item['name']
            if name == monitor_name:
                LOG.info("stop monitoring at %s" % monitor_path)
                self.epoll.unregister(fileno)
                os.close(fileno)
                del self.stream_dict[fileno]

    def io_watch(self):
        while(not self.stop.wait(0.0001)):
            self._running = True
            events = self.epoll.poll(0.0001)
            for fileno, event in events:
                self._handle(fileno, event)
        
        for fileno in self.stream_dict.keys():
            self.epoll.unregister(fileno)
            os.close(fileno)
        self._running = False
        LOG.info("close Stream monitoring thread")

    def _handle(self, fd, event):
        if event & select.EPOLLIN:
            #LOG.debug("%d, %s" % (fd, self.stream_dict[fd]['name']))
            try:
                buf = os.read(fd, 1024)
            except OSError as e:
                # TODO: "Resource temporarily unavailable" Error
                return

            # We got some output
            lines = (self.stream_dict[fd]['buf']+ buf).split('\n')
            # Save partial last line, if any
            self.stream_dict[fd]['buf'] = lines.pop()
            for line in lines:
                stream_name = self.stream_dict[fd]['name']
                if stream_name == StreamMonitor.DISK_MODIFY:
                    self._handle_chunks_modification(line)
                elif stream_name == StreamMonitor.DISK_ACCESS:
                    self._handle_disk_access(line)
                elif stream_name == StreamMonitor.MEMORY_ACCESS:
                    self._handle_memory_access(line)
                else:
                    raise IOError("Error, invalid stream")
                    break
        elif event & select.EPOLLOUT:
            pass
        elif event & select.EPOLLPRI:
            LOG.debug("error?")

    def _handle_chunks_modification(self, line):
        ctime, chunk = line.split("\t")
        ctime = float(ctime)
        chunk = int(chunk)
        self.modified_chunk_dict[chunk] = ctime
        #LOG.debug("%s: %f, %d" % ("modification", ctime, chunk))

    def _handle_disk_access(self, line):
        ctime, chunk = line.split("\t")
        ctime = float(ctime)
        chunk = int(chunk)
        self.disk_access_chunk_list.append(chunk)

    def _handle_memory_access(self, line):
        ctime, chunk = line.split("\t")
        ctime = float(ctime)
        chunk = int(chunk)
        self.mem_access_chunk_list.append(chunk)

    def terminate(self):
        self.stop.set()


class FileMonitor(threading.Thread):
    QEMU_LOG    = "QEMU_LOG"

    def __init__(self, path, name):
        self.stream_dict = dict()
        self._running = False
        self.stop = threading.Event()
        self.fd = os.open(path, os.O_RDONLY | os.O_NONBLOCK)
        self.buf = ''
        threading.Thread.__init__(self, target=self.io_watch)
        LOG.info("start monitoring at %s" % path)

    def io_watch(self):
        while(not self.stop.wait(0.0001)):
            self._running = True
            line = os.read(self.fd, 1024)
            lines = (self.buf+line).split('\n')
            self.buf = lines.pop()
            for line in lines:
                self._handle_qemu_log(line)
            else:
                time.sleep(0.001)
        self._running = False
        LOG.info("close File monitoring thread")

    def _handle_qemu_log(self, line):
        splits = line.split(",", 2)
        event_time = splits[0].strip()
        header = splits[1].strip()
        data = splits[2].strip()
        if header == 'dma':
            #sys.stdout.write("(%s)\n" % line)
            pass
        elif header == 'bdrv_discard':
            #sys.stdout.write("discard:(%s, %s)\n" % (event_time, data))
            pass
        else:
            sys.stdout.write("invalid log: (%s)(%s)(%s)\n" % (event_time, header, data))

    def terminate(self):
        self.stop.set()


class FuseFeedingProc(multiprocessing.Process):
    def __init__(self, fuse, input_pipename, END_OF_PIPE, **kwargs):
        self.fuse = fuse
        self.input_pipename = input_pipename
        self.END_OF_PIPE = END_OF_PIPE
        self.time_queue = None
        self.stop = threading.Event()
        #threading.Thread.__init__(self, target=self.feeding_thread)
        multiprocessing.Process.__init__(self, target=self.feeding_thread)

    def feeding_thread(self):
        self.input_pipe = open(self.input_pipename, "r")
        start_time = time.time()
        while(not self.stop.wait(0.0000001)):
            self._running = True
            try:
                #chunks = self.input_pipe.recv()
                chunks_str = self.input_pipe.readline().strip()
                if chunks_str == self.END_OF_PIPE:
                    break
            except EOFError:
                break
            self.fuse.fuse_write(chunks_str)

        end_time = time.time()
        if self.time_queue != None: 
            self.time_queue.put({'start_time':start_time, 'end_time':end_time})
        LOG.info("[FUSE] : (%s)-(%s)=(%s)\n" % \
                (start_time, end_time, (end_time-start_time)))
        self.fuse.fuse_write("END_OF_TRANSMISSION")


    def terminate(self):
        self.stop.set()
