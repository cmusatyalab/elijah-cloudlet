#!/usr/bin/env python
import sys
import SocketServer
import signal
import socket
import uuid
import json
import time
import struct
from urlparse import urlparse

import nova_client

# global variable
server = None


def sigint_handler(signum, frame):
    global server
    sys.stdout.write("Exit by user\n")
    if server != None:
        server.terminate()
    sys.exit(0)


class FileServerError(Exception):
    pass


class RelayServer(SocketServer.TCPServer):
    LOCAL_IPADDRESS = "0.0.0.0"
    PORT = 8081

    def __init__(self, args):
        server_address = (RelayServer.LOCAL_IPADDRESS, RelayServer.PORT)

        self.allow_reuse_address = True
        try:
            SocketServer.TCPServer.__init__(self, server_address, \
                    OpenStackCommandReplay)
        except socket.error as e:
            sys.stderr.write(str(e))
            sys.stderr.write("Check IP/Port : %s\n" % (str(server_address)))
            sys.exit(1)
        self.socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        sys.stdout.write("Server start at %s\n" % (str(server_address)))

    def terminate(self):
        pass

    def process_command_line(self, args):
        return None


class OpenStackCommandReplay(SocketServer.StreamRequestHandler):

    def handle(self):
        print "new connection"
        header_data = self.request.recv(4)
        if header_data == None or len(header_data) != 4:
            raise FileServerError("Failed to receive first byte of header")
        data_size = struct.unpack("!I", header_data)[0]
        json_msg = ''
        while len(json_msg) < data_size:
            json_msg += self.request.recv(data_size-len(header_data))
        if not json_msg:
            self.ret_fail("Cannot read data")
            return
        request_msg = json.loads(json_msg)
        overlay_meta_url = request_msg.get('overlay_meta_url')
        overlay_blob_url = request_msg.get('overlay_blob_url')
        application_name = request_msg.get('application_name')

        # openstakc user specific arguments
        server_address = "rain.elijah.cs.cmu.edu"
        user_name = 'admin'
        user_password = 'password'
        tenant_name = 'admin'
        key_name = "mykey"
        base_disk_name = 'Ubuntu-server-base-disk'
        launch_vm_name = 'synthesis-' + str(uuid.uuid4())[0:8]
        
        token, endpoint, glance_endpoint = \
                nova_client.get_token(server_address, user_name, 
                        user_password, tenant_name)

        ret = nova_client.request_synthesis(server_address, token, urlparse(endpoint), 
                key_name=key_name, image_name=base_disk_name, server_name=launch_vm_name,
                overlay_meta_url=overlay_meta_url, overlay_blob_url=overlay_blob_url)

        server_uuid = ret['server']['id']
        while True:
            status, ip_list = nova_client.request_cloudlet_ipaddress(
                    server_address, token, urlparse(endpoint), 
                    server_uuid=server_uuid)
            status = status.lower()
            if status == 'active':
                break
            elif status == 'error' or status == 'shut-off' or status == 'deleted':
                ip_list = None
                break
            elif status == 'build':
                time.sleep(1)
                print 'waiting to build'
                continue
        
        if ip_list == None:
            ret = {
                    'result' : 'failed',
                    'error' : 'failed to allocate floating ip adddress',
                    }
        else:
            ret = {
                    'result' : 'success',
                    'vm-ip' : ip_list,
                    'application_name' : application_name,
                    'vm-id' : server_uuid,
                    }

        # return packet
        json_ret = json.dumps(ret)
        ret_data_size = len(json_ret)
        self.request.send(struct.pack("!I", ret_data_size)) 
        self.wfile.write(json_ret)
        print "[INFO] finishing request"


    def finish(self):
        pass


    @staticmethod
    def recvall(sock, size):
        data = ''
        while len(data) < size:
            data += sock.recv(size - len(data))
        return data


def main(argv):
    global server
    signal.signal(signal.SIGINT, sigint_handler)

    server = RelayServer(sys.argv[1:])
    try:
        server.serve_forever()
    except Exception as e:
        #sys.stderr.write(str(e))
        server.terminate()
        sys.exit(1)
    except KeyboardInterrupt as e:
        sys.stdout.write("Exit by user\n")
        server.terminate()
        sys.exit(1)
    else:
        server.terminate()
        sys.exit(0)


if __name__ == "__main__":
    main(sys.argv)

