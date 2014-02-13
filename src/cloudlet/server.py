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
import functools
import traceback
import sys
import time
import SocketServer
import socket
import tempfile
import struct
import shutil
import threading

from cloudlet import synthesis as synthesis
from cloudlet.db.api import DBConnector
from cloudlet.db.table_def import BaseVM, Session, OverlayVM
from cloudlet.synthesis_protocol import Protocol as Protocol
from cloudlet.upnp_server import UPnPServer, UPnPError
from cloudlet.Configuration import Const as Cloudlet_Const
from cloudlet.Configuration import Synthesis_Const as Synthesis_Const
from cloudlet import msgpack

from pprint import pformat
from optparse import OptionParser
from multiprocessing import Process, JoinableQueue, Queue, Manager
from lzma import LZMADecompressor
from cloudlet import log as logging


LOG = logging.getLogger(__name__)


class RapidSynthesisError(Exception):
    pass


def wrap_process_fault(function):
    """Wraps a method to catch exceptions related to instances.
    This decorator wraps a method to catch any exceptions and
    terminate the request gracefully.
    """
    @functools.wraps(function)
    def decorated_function(self, *args, **kwargs):
        try:
            return function(self, *args, **kwargs)
        except Exception, e:
            if hasattr(self, 'exception_handler'):
                self.exception_handler()
            kwargs.update(dict(zip(function.func_code.co_varnames[2:], args)))
            LOG.error("failed with : %s" % str(kwargs))

    return decorated_function


class NetworkUtil(object):
    @staticmethod
    def recvall(sock, size):
        data = ''
        while len(data) < size:
            data += sock.recv(size - len(data))
        return data

    @staticmethod
    def encoding(data):
        return msgpack.packb(data)

    @staticmethod
    def decoding(data):
        return msgpack.unpackb(data)


class NetworkStepThread(threading.Thread):
    MAX_REQUEST_SIZE = 1024*512 # 512 KB

    def __init__(self, network_handler, overlay_urls, overlay_urls_size, demanding_queue, out_queue, time_queue, chunk_size):
        self.network_handler = network_handler
        self.read_stream = network_handler.rfile
        self.overlay_urls = overlay_urls
        self.overlay_urls_size = overlay_urls_size
        self.demanding_queue = demanding_queue
        self.out_queue = out_queue
        self.time_queue = time_queue
        self.chunk_size = chunk_size
        threading.Thread.__init__(self, target=self.receive_overlay_blobs)

    def exception_handler(self):
        self.out_queue.put(Synthesis_Const.ERROR_OCCURED)
        self.time_queue.put({'start_time':-1, 'end_time':-1, "bw_mbps":0})

    @wrap_process_fault
    def receive_overlay_blobs(self):
        total_read_size = 0
        counter = 0
        index = 0 
        finished_url = dict()
        requesting_list = list()
        out_of_order_count = 0
        total_urls_count = len(self.overlay_urls)
        start_time = time.time()

        while len(finished_url) < total_urls_count:
            #request to client until it becomes more than MAX_REQUEST_SIZE
            while True:
                requesting_size = sum([self.overlay_urls_size[item] for item in requesting_list])
                if requesting_size > self.MAX_REQUEST_SIZE or len(self.overlay_urls) == 0:
                    # Enough requesting list or nothing left to request
                    break;

                # find overlay to request
                urgent_overlay_url = None
                while not self.demanding_queue.empty():
                    # demanding_queue can have multiple same request
                    demanding_url = self.demanding_queue.get()
                    if (finished_url.get(demanding_url, False) == False) and \
                            (demanding_url not in requesting_list):
                        urgent_overlay_url = demanding_url
                        break

                requesting_overlay = None
                if urgent_overlay_url != None:
                    requesting_overlay = urgent_overlay_url
                    out_of_order_count += 1
                    if requesting_overlay in self.overlay_urls:
                        self.overlay_urls.remove(requesting_overlay)
                else:
                    requesting_overlay = self.overlay_urls.pop(0)

                # request overlay blob to client
                message = NetworkUtil.encoding({
                    Protocol.KEY_COMMAND : Protocol.MESSAGE_COMMAND_ON_DEMAND,
                    Protocol.KEY_REQUEST_SEGMENT:requesting_overlay
                    })
                message_size = struct.pack("!I", len(message))
                self.network_handler.request.send(message_size)
                self.network_handler.wfile.write(message)
                self.network_handler.wfile.flush()
                requesting_list.append(requesting_overlay)

            # read header
            blob_header_size = struct.unpack("!I", self.read_stream.read(4))[0]
            blob_header_data = self.read_stream.read(blob_header_size)
            blob_header = NetworkUtil.decoding(blob_header_data)
            command = blob_header.get(Protocol.KEY_COMMAND, None)
            if command != Protocol.MESSAGE_COMMAND_SEND_OVERLAY:
                msg = "Unexpected command while streaming overlay VM: %d" % command
                raise RapidSynthesisError(msg)
            blob_size = blob_header.get(Protocol.KEY_REQUEST_SEGMENT_SIZE, 0)
            blob_url = blob_header.get(Protocol.KEY_REQUEST_SEGMENT, None)
            if blob_size == 0 or blob_url == None:
                raise RapidSynthesisError("Invalid header for overlay segment")

            finished_url[blob_url] = True
            requesting_list.remove(blob_url)
            read_count = 0
            while read_count < blob_size:
                read_min_size = min(self.chunk_size, blob_size-read_count)
                chunk = self.read_stream.read(read_min_size)
                read_size = len(chunk)
                if chunk:
                    self.out_queue.put(chunk)
                else:
                    break

                counter += 1
                read_count += read_size
            total_read_size += read_count
            index += 1

        self.out_queue.put(Synthesis_Const.END_OF_FILE)
        end_time = time.time()
        time_delta= end_time-start_time

        if time_delta > 0:
            bw = total_read_size*8.0/time_delta/1024/1024
        else:
            bw = 1

        self.time_queue.put({'start_time':start_time, 'end_time':end_time, "bw_mbps":bw})
        LOG.info("[Transfer] out-of-order fetching : %d / %d == %5.2f %%" % \
                (out_of_order_count, total_urls_count, \
                100.0*out_of_order_count/total_urls_count))
        try:
            LOG.info("[Transfer] : (%s)~(%s)=(%s) (%d loop, %d bytes, %lf Mbps)" % \
                    (start_time, end_time, (time_delta),\
                    counter, total_read_size, \
                    total_read_size*8.0/time_delta/1024/1024))
        except ZeroDivisionError:
            LOG.info("[Transfer] : (%s)~(%s)=(%s) (%d, %d)" % \
                    (start_time, end_time, (time_delta),\
                    counter, total_read_size))


class DecompStepProc(Process):
    def __init__(self, input_queue, output_path, time_queue, temp_overlay_file=None):
        self.input_queue = input_queue
        self.time_queue = time_queue
        self.output_path = output_path
        self.decompressor = LZMADecompressor()
        self.temp_overlay_file = temp_overlay_file
        Process.__init__(self, target=self.decompress_blobs)

    def exception_handler(self):
        LOG.error("decompress step error")

    @wrap_process_fault
    def decompress_blobs(self):
        self.output_queue = open(self.output_path, "w")
        start_time = time.time()
        data_size = 0
        counter = 0

        while True:
            chunk = self.input_queue.get()
            if chunk == Synthesis_Const.END_OF_FILE:
                break
            if chunk == Synthesis_Const.ERROR_OCCURED:
                break;
            data_size = data_size + len(chunk)
            decomp_chunk = self.decompressor.decompress(chunk)

            self.input_queue.task_done()
            self.output_queue.write(decomp_chunk)
            counter = counter + 1
            if self.temp_overlay_file:
                self.temp_overlay_file.write(decomp_chunk)

        decomp_chunk = self.decompressor.flush()
        self.output_queue.write(decomp_chunk)
        self.output_queue.close()
        if self.temp_overlay_file:
            self.temp_overlay_file.write(decomp_chunk)
            self.temp_overlay_file.close()

        end_time = time.time()
        self.time_queue.put({'start_time':start_time, 'end_time':end_time})
        LOG.info("[Decomp] : (%s)-(%s)=(%s) (%d loop, %d bytes)" % \
                (start_time, end_time, (end_time-start_time), 
                counter, data_size))


class SynthesisHandler(SocketServer.StreamRequestHandler):
    synthesis_option = {
            Protocol.SYNTHESIS_OPTION_DISPLAY_VNC : False,
            Protocol.SYNTHESIS_OPTION_EARLY_START : False,
            Protocol.SYNTHESIS_OPTION_SHOW_STATISTICS : False
            }

    def ret_fail(self, message):
        LOG.error("%s" % str(message))
        message = NetworkUtil.encoding({
            Protocol.KEY_COMMAND : Protocol.MESSAGE_COMMAND_FAIELD,
            Protocol.KEY_FAILED_REASON : message
            })
        message_size = struct.pack("!I", len(message))
        self.request.send(message_size)
        self.wfile.write(message)

    def ret_success(self, req_command, payload=None):
        send_message = {
            Protocol.KEY_COMMAND : Protocol.MESSAGE_COMMAND_SUCCESS,
            Protocol.KEY_REQUESTED_COMMAND : req_command,
            }
        if payload:
            send_message.update(payload)
        message = NetworkUtil.encoding(send_message)
        message_size = struct.pack("!I", len(message))
        self.request.send(message_size)
        self.wfile.write(message)
        self.wfile.flush()

    def send_synthesis_done(self):
        message = NetworkUtil.encoding({
            Protocol.KEY_COMMAND : Protocol.MESSAGE_COMMAND_SYNTHESIS_DONE,
            })
        LOG.info("SUCCESS to launch VM")
        message_size = struct.pack("!I", len(message))
        self.request.send(message_size)
        self.wfile.write(message)

    def _check_validity(self, message):
        header_info = None
        requested_base = None

        if (message.get(Protocol.KEY_META_SIZE, 0) > 0):
            # check header option
            client_syn_option = message.get(Protocol.KEY_SYNTHESIS_OPTION, None)
            if client_syn_option != None and len(client_syn_option) > 0:
                self.synthesis_option.update(client_syn_option)

            # receive overlay meta file
            meta_file_size = message.get(Protocol.KEY_META_SIZE)
            header_data = self.request.recv(meta_file_size)
            while len(header_data) < meta_file_size:
                header_data += self.request.recv(meta_file_size- len(header_data))
            header = NetworkUtil.decoding(header_data)
            base_hashvalue = header.get(Cloudlet_Const.META_BASE_VM_SHA256, None)

            # check base VM
            for each_basevm in self.server.basevm_list:
                if base_hashvalue == each_basevm.hash_value:
                    LOG.info("New client request %s VM" \
                            % (each_basevm.disk_path))
                    requested_base = each_basevm.disk_path
                    header_info = header
        return [requested_base, header_info]

    def _handle_synthesis(self, message):
        LOG.info("\n\n----------------------- New Connection --------------")
        # check overlay meta info
        start_time = time.time()
        header_start_time = time.time()
        base_path, meta_info = self._check_validity(message)
        session_id = message.get(Protocol.KEY_SESSION_ID, None)
        if base_path and meta_info and meta_info.get(Cloudlet_Const.META_OVERLAY_FILES, None):
            self.ret_success(Protocol.MESSAGE_COMMAND_SEND_META)
        else:
            self.ret_fail("No matching Base VM")
            return

        # update DB
        new_overlayvm = OverlayVM(session_id, base_path)
        self.server.dbconn.add_item(new_overlayvm)

        # start synthesis process
        url_manager = Manager()
        overlay_urls = url_manager.list()
        overlay_urls_size = url_manager.dict()
        for blob in meta_info[Cloudlet_Const.META_OVERLAY_FILES]:
            url = blob[Cloudlet_Const.META_OVERLAY_FILE_NAME]
            size = blob[Cloudlet_Const.META_OVERLAY_FILE_SIZE]
            overlay_urls.append(url)
            overlay_urls_size[url] = size
        LOG.info("  - %s" % str(pformat(self.synthesis_option)))
        LOG.info("  - Base VM     : %s" % base_path)
        LOG.info("  - Blob count  : %d" % len(overlay_urls))
        if overlay_urls == None:
            self.ret_fail("No overlay info listed")
            return
        (base_diskmeta, base_mem, base_memmeta) = \
                Cloudlet_Const.get_basepath(base_path, check_exist=True)
        header_end_time = time.time()
        LOG.info("Meta header processing time: %f" % (header_end_time-header_start_time))

        # read overlay files
        # create named pipe to convert queue to stream
        time_transfer = Queue(); time_decomp = Queue();
        time_delta = Queue(); time_fuse = Queue();
        self.tmp_overlay_dir = tempfile.mkdtemp()
        self.overlay_pipe = os.path.join(self.tmp_overlay_dir, 'overlay_pipe')
        os.mkfifo(self.overlay_pipe)

        # save overlay decomp result for measurement
        temp_overlay_file = None
        if self.synthesis_option.get(Protocol.SYNTHESIS_OPTION_SHOW_STATISTICS):
            temp_overlay_filepath = os.path.join(self.tmp_overlay_dir, "overlay_file")
            temp_overlay_file = open(temp_overlay_filepath, "w+b")

        # overlay
        demanding_queue = Queue()
        download_queue = JoinableQueue()
        download_process = NetworkStepThread(self, 
                    overlay_urls, overlay_urls_size, demanding_queue, 
                    download_queue, time_transfer, Synthesis_Const.TRANSFER_SIZE, 
                    )
        decomp_process = DecompStepProc(
                download_queue, self.overlay_pipe, time_decomp, temp_overlay_file,
                )
        modified_img, modified_mem, self.fuse, self.delta_proc, self.fuse_proc = \
                synthesis.recover_launchVM(base_path, meta_info, self.overlay_pipe, 
                        log=sys.stdout, demanding_queue=demanding_queue)
        self.delta_proc.time_queue = time_delta # for measurement
        self.fuse_proc.time_queue = time_fuse # for measurement

        if self.synthesis_option.get(Protocol.SYNTHESIS_OPTION_EARLY_START, False):
            # 1. resume VM
            self.resumed_VM = synthesis.SynthesizedVM(modified_img, modified_mem, self.fuse)
            time_start_resume = time.time()
            self.resumed_VM.start()
            time_end_resume = time.time()

            # 2. start processes
            download_process.start()
            decomp_process.start()
            self.delta_proc.start()
            self.fuse_proc.start()

            # 3. return success right after resuming VM
            # before receiving all chunks
            self.resumed_VM.join()
            self.send_synthesis_done()

            # 4. then wait fuse end
            self.fuse_proc.join()
        else:
            # 1. start processes
            download_process.start()
            decomp_process.start()
            self.delta_proc.start()
            self.fuse_proc.start()

            # 2. resume VM
            self.resumed_VM = synthesis.SynthesizedVM(modified_img, modified_mem, self.fuse)
            self.resumed_VM.start()

            # 3. wait for fuse end
            self.fuse_proc.join()

            # 4. return success to client
            time_start_resume = time.time()     # measure pure resume time
            self.resumed_VM.join()
            time_end_resume = time.time()
            self.send_synthesis_done()

        end_time = time.time()

        # printout result
        SynthesisHandler.print_statistics(start_time, end_time, \
                time_transfer, time_decomp, time_delta, time_fuse, \
                resume_time=(time_end_resume-time_start_resume))

        if self.synthesis_option.get(Protocol.SYNTHESIS_OPTION_DISPLAY_VNC, False):
            synthesis.connect_vnc(self.resumed_VM.machine, no_wait=True)

        # wait for finish message from client
        LOG.info("[SOCKET] waiting for client exit message")
        data = self.request.recv(4)
        msgpack_size = struct.unpack("!I", data)[0]
        msgpack_data = self.request.recv(msgpack_size)
        while len(msgpack_data) < msgpack_size:
            msgpack_data += self.request.recv(msgpack_size- len(msgpack_data))
        finish_message = NetworkUtil.decoding(msgpack_data)
        command = finish_message.get(Protocol.KEY_COMMAND, None)
        if command != Protocol.MESSAGE_COMMAND_FINISH:
            msg = "Unexpected command while streaming overlay VM: %d" % command
            raise RapidSynthesisError(msg)
        self.ret_success(Protocol.MESSAGE_COMMAND_FINISH)
        LOG.info("  - %s" % str(pformat(finish_message)))

        # printout synthesis statistics
        if self.synthesis_option.get(Protocol.SYNTHESIS_OPTION_SHOW_STATISTICS):
            mem_access_list = self.resumed_VM.monitor.mem_access_chunk_list
            disk_access_list = self.resumed_VM.monitor.disk_access_chunk_list
            synthesis.synthesis_statistics(meta_info, temp_overlay_filepath, \
                    mem_access_list, disk_access_list)

        # update DB
        new_overlayvm.terminate()

    def _handle_get_resource_info(self, message):
        if hasattr(self.server, 'resource_monitor'):
            resource = self.server.resource_monitor.get_static_resource()
            resource.update(self.server.resource_monitor.get_dynamic_resource())
            # send response
            pay_load = {Protocol.KEY_PAYLOAD: resource}
            self.ret_success(Protocol.MESSAGE_COMMAND_GET_RESOURCE_INFO, pay_load)
        else:
            self.ret_fail("resource function is not implemented")

    def _handle_session_create(self, message):
        new_session = Session()
        self.server.dbconn.add_item(new_session)

        # send response
        pay_load = {Protocol.KEY_SESSION_ID : new_session.session_id}
        self.ret_success(Protocol.MESSAGE_COMMAND_SESSION_CREATE, pay_load)

    def _handle_session_close(self, message):
        my_session_id = message.get(Protocol.KEY_SESSION_ID, None)
        ret_session = self.server.dbconn.session.query(Session).filter(Session.session_id==my_session_id).first()
        if ret_session:
            ret_session.terminate()
        self.server.dbconn.session.commit()

        # send response
        self.ret_success(Protocol.MESSAGE_COMMAND_SESSION_CLOSE)

    def _check_session(self, message):
        my_session_id = message.get(Protocol.KEY_SESSION_ID, None)
        ret_session = self.server.dbconn.session.query(Session).filter(Session.session_id==my_session_id).first()
        if ret_session and ret_session.status == Session.STATUS_RUNNING:
            return True
        else:
            # send response
            self.ret_fail("Not Valid session %s" % (my_session_id))
            return False

    def force_session_close(self, message):
        my_session_id = message.get(Protocol.KEY_SESSION_ID, None)
        ret_session = self.server.dbconn.session.query(Session).filter(Session.session_id==my_session_id).first()
        ret_session.terminate(status=Session.STATUS_UNEXPECT_CLOSE)

    def handle(self):
        '''Handle request from the client
        Each request follows this format: 

        | message_pack size | message_pack data |
        |   (4 bytes)       | (variable length) |
        '''

        # get header
        data = self.request.recv(4)
        if data == None or len(data) != 4:
            raise RapidSynthesisError("Failed to receive first byte of header")
        message_size = struct.unpack("!I", data)[0]
        msgpack_data = self.request.recv(message_size)
        while len(msgpack_data) < message_size:
            msgpack_data += self.request.recv(message_size-len(msgpack_data))
        message = NetworkUtil.decoding(msgpack_data)
        command = message.get(Protocol.KEY_COMMAND, None)

        # handle request that requries session
        try:
            if command == Protocol.MESSAGE_COMMAND_SEND_META:
                if self._check_session(message):
                    self._handle_synthesis(message)
            elif command == Protocol.MESSAGE_COMMAND_SEND_OVERLAY:
                # handled at _handle_synthesis
                pass
            elif command == Protocol.MESSAGE_COMMAND_FINISH:
                # handled at _handle_synthesis
                pass
            elif command == Protocol.MESSAGE_COMMAND_GET_RESOURCE_INFO:
                self._handle_get_resource_info(message)
            elif command == Protocol.MESSAGE_COMMAND_SESSION_CREATE:
                self._handle_session_create(message)
            elif command == Protocol.MESSAGE_COMMAND_SESSION_CLOSE:
                if self._check_session(message):
                    self._handle_session_close(message)
            else:
                LOG.info("Invalid command number : %d" % command)
        except Exception as e:
            # close session if synthesis failed
            if command == Protocol.MESSAGE_COMMAND_SEND_META:
                self.force_session_close()
            sys.stderr.write(traceback.format_exc())
            sys.stderr.write("%s" % str(e))
            sys.stderr.write("handler raises exception\n")
            self.terminate()
            raise e

    def finish(self):
        if hasattr(self, 'delta_proc') and self.delta_proc != None:
            self.delta_proc.join()
            self.delta_proc.finish()
        if hasattr(self, 'resumed_VM') and self.resumed_VM != None:
            self.resumed_VM.terminate()
        if hasattr(self, 'fuse') and self.fuse != None:
            self.fuse.terminate()

        if hasattr(self, 'overlay_pipe') and os.path.exists(self.overlay_pipe):
            os.unlink(self.overlay_pipe)
        if hasattr(self, 'tmp_overlay_dir') and os.path.exists(self.tmp_overlay_dir):
            shutil.rmtree(self.tmp_overlay_dir)

    def terminate(self):
        # force terminate when something wrong in handling request
        # do not wait for joinining
        if hasattr(self, 'detla_proc') and self.delta_proc != None:
            self.delta_proc.finish()
            if self.delta_proc.is_alive():
                self.delta_proc.terminate()
            self.delta_proc = None
        if hasattr(self, 'resumed') and self.resumed_VM != None:
            self.resumed_VM.terminate()
            self.resumed_VM = None
        if hasattr(self, 'fuse') and self.fuse != None:
            self.fuse.terminate()
            self.fuse = None
        if hasattr(self, 'overlay_pipe') and os.path.exists(self.overlay_pipe):
            os.unlink(self.overlay_pipe)
        if hasattr(self, 'tmp_overlay_dir') and os.path.exists(self.tmp_overlay_dir):
            shutil.rmtree(self.tmp_overlay_dir)


    @staticmethod
    def print_statistics(start_time, end_time, \
            time_transfer, time_decomp, time_delta, time_fuse,
            resume_time=0):
        # Print out Time Measurement
        transfer_time = time_transfer.get()
        decomp_time = time_decomp.get()
        delta_time = time_delta.get()
        fuse_time = time_fuse.get()
        transfer_start_time = transfer_time['start_time']
        transfer_end_time = transfer_time['end_time']
        transfer_bw = transfer_time.get('bw_mbps', -1)
        decomp_start_time = decomp_time['start_time']
        decomp_end_time = decomp_time['end_time']
        delta_start_time = delta_time['start_time']
        delta_end_time = delta_time['end_time']
        fuse_start_time = fuse_time['start_time']
        fuse_end_time = fuse_time['end_time']

        transfer_diff = (transfer_end_time-transfer_start_time)
        decomp_diff = (decomp_end_time-transfer_end_time)
        delta_diff = (fuse_end_time-decomp_end_time)

        message = "\n"
        message += "Pipelined measurement\n"
        message += 'Transfer\tDecomp\t\tDelta(Fuse)\tResume\t\tTotal\n'
        message += "%011.06f\t" % (transfer_diff)
        message += "%011.06f\t" % (decomp_diff)
        message += "%011.06f\t" % (delta_diff)
        message += "%011.06f\t" % (resume_time)
        message += "%011.06f\t" % (end_time-start_time)
        message += "\n"
        message += "Transmission BW : %f" % transfer_bw
        LOG.debug(message)


def get_local_ipaddress():
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.connect(("gmail.com",80))
    ipaddress = (s.getsockname()[0])
    s.close()
    return ipaddress


class SynthesisServer(SocketServer.TCPServer):

    def __init__(self, args):
        settings, args = SynthesisServer.process_command_line(args)
        self.dbconn = DBConnector()
        self.basevm_list = self.check_basevm()

        Synthesis_Const.LOCAL_IPADDRESS = "0.0.0.0"
        server_address = (Synthesis_Const.LOCAL_IPADDRESS, Synthesis_Const.SERVER_PORT_NUMBER)

        self.allow_reuse_address = True
        try:
            SocketServer.TCPServer.__init__(self, server_address, SynthesisHandler)
        except socket.error as e:
            sys.stderr.write(str(e))
            sys.stderr.write("Check IP/Port : %s\n" % (str(server_address)))
            sys.exit(1)
        self.socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.socket.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
        LOG.info("* Server configuration")
        LOG.info(" - Open TCP Server at %s" % (str(server_address)))
        LOG.info(" - Disable Nagle(No TCP delay)  : %s" \
                % str(self.socket.getsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY)))
        LOG.info("-"*50)

        # Start UPnP Server
        try:
            self.upnp_server = UPnPServer()
            self.upnp_server.start()
        except UPnPError as e:
            LOG.info(str(e))
            LOG.info("[Warning] Cannot start UPnP Server")
            self.upnp_server = None
        LOG.info("[INFO] Start UPnP Server")

        # This is cloudlet discovery related part and separated out
        '''
        if settings.register_server:
            try:
                self.register_client = RegisterThread(
                        settings.register_server,
                        update_period=Synthesis_Const.DIRECTORY_UPDATE_PERIOD)
                self.register_client.start()
                LOG.info("[INFO] Register to Cloudlet direcory service")
            except RegisterError as e:
                LOG.info(str(e))
                LOG.info("[Warning] Cannot register Cloudlet to central server")
        try:
            self.rest_server = RESTServer()
            self.rest_server.start()
        except RESTServerError as e:
            LOG.info(str(e))
            LOG.info("[Warning] Cannot start REST API Server")
            self.rest_server = None
        LOG.info("[INFO] Start RESTful API Server")
        try:
            self.resource_monitor = ResourceMonitorThread()
            self.resource_monitor.start()
        except ResourceMonitorError as e:
            LOG.info(str(e))
            LOG.info("[Warning] Cannot register Cloudlet to central server\n")
        '''

    def handle_error(self, request, client_address):
        #SocketServer.TCPServer.handle_error(self, request, client_address)
        #sys.stderr.write("handling error from client %s\n" % (str(client_address)))
        pass

    def expire_all_sessions(self):
        from db.table_def import Session

        LOG.info("Close all running sessions")
        session_list = self.dbconn.list_item(Session)
        for item in session_list:
            if item.status == Session.STATUS_RUNNING:
                item.terminate(Session.STATUS_UNEXPECT_CLOSE)
        self.dbconn.session.commit()


    def terminate(self):
        # expire all existing session
        self.expire_all_sessions()

        # close all thread
        if self.socket != -1:
            self.socket.close()
        if hasattr(self, 'upnp_server') and self.upnp_server != None:
            LOG.info("[TERMINATE] Terminate UPnP Server")
            self.upnp_server.terminate()
            self.upnp_server.join()
        if hasattr(self, 'register_client') and self.register_client != None:
            LOG.info("[TERMINATE] Deregister from directory service")
            self.register_client.terminate()
            self.register_client.join()
        if hasattr(self, 'rest_server') and self.rest_server != None:
            LOG.info("[TERMINATE] Terminate REST API monitor")
            self.rest_server.terminate()
            self.rest_server.join()
        if hasattr(self, 'resource_monitor') and self.resource_monitor != None:
            LOG.info("[TERMINATE] Terminate resource monitor")
            self.resource_monitor.terminate()
            self.resource_monitor.join()
        LOG.info("[TERMINATE] Finish synthesis server connection")

    @staticmethod
    def process_command_line(argv):
        global operation_mode
        VERSION = 'VM Synthesis Server: %s' % Cloudlet_Const.VERSION

        parser = OptionParser(usage="usage: %prog " + " [option]",
                version=VERSION)
        parser.add_option(
                '-r', '--register-server', action='store', dest='register_server',
                default=None, help= 'Domain address for registration server.\n \
                        Specify this if you like to register your \
                        Cloudlet to registration server.')
        settings, args = parser.parse_args(argv)
        return settings, args

    def check_basevm(self):
        basevm_list = self.dbconn.list_item(BaseVM)
        ret_list = list()
        LOG.info("-"*50)
        LOG.info("* Base VM Configuration")
        for index, item in enumerate(basevm_list):
            # check file location
            (base_diskmeta, base_mempath, base_memmeta) = \
                    Cloudlet_Const.get_basepath(item.disk_path)
            if not os.path.exists(item.disk_path):
                LOG.warning("disk image (%s) is not exist" % (item.disk_path))
                continue
            if not os.path.exists(base_mempath):
                LOG.warning("memory snapshot (%s) is not exist" % (base_mempath))
                continue

            # add to list
            ret_list.append(item)
            LOG.info(" %d : %s (Disk %d MB, Memory %d MB)" % \
                    (index, item.disk_path, os.path.getsize(item.disk_path)/1024/1024, \
                    os.path.getsize(base_mempath)/1024/1024))
        LOG.info("-"*50)

        if len(ret_list) == 0:
            LOG.error("[Error] NO valid Base VM")
            sys.exit(2)
        return ret_list

