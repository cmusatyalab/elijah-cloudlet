#!/usr/bin/env python
import sys
import urllib2
from optparse import OptionParser
import json
import httplib, urllib
from poster.encode import multipart_encode
from poster.streaminghttp import register_openers
import urllib2

# Overlya URL
BASE_DIR = '/home/krha/Cloudlet/image/overlay'
MOPED_DISK = BASE_DIR + '/overlay/moped/overlay1/moped.qcow2.lzma'
MOPED_MEMORY = BASE_DIR + '/overlay/moped/overlay1/moped.mem.lzma'

def run_client(server_address):
    request_option = {'CPU-core':'2', 'Memory-Size':'4GB'}
    VM_info = [{"name":"ubuntuLTS", "type":"baseVM", "version":"linux"}]
    request_option['VM'] = VM_info

    register_openers()
    disk_file = open(MOPED_DISK, "rb")
    memory_file = open(MOPED_MEMORY, "rb")
    post_data={"info":json.dumps(request_option), "disk_file":disk_file, "mem_file":memory_file}
    datagen, headers = multipart_encode(post_data)

    # Create the Request object
    print "JSON format : \n" +  json.dumps(request_option, indent=4)
    print "connecting to (%s)" % (server_address)
    request = urllib2.Request(server_address, datagen, headers)

    # Actually do the request, and get the response
    ret = ''
    try:
        ret = urllib2.urlopen(request).read()
    except urllib2.URLError:
        print "Connection Error (%s)" % (server_address)
    print ret


def process_command_line(argv):
    global operation_mode

    parser = OptionParser(usage="usage: %prog" + " [option]",
            version="Cloudlet (Android)Emulated Client")
    parser.add_option(
            '-s', '--server', type='string', action='store', dest='address', default="http://server.krha.kr:8021/synthesis",
            help='Set Server HTTP Address, default is localhost:8021')

    settings, args = parser.parse_args(argv)
    if args:
        parser.error('program takes no command-line arguments; "%s" ignored.' % (args,))

    return settings, args


def main(argv=None):
    settings, args = process_command_line(sys.argv[1:])
    run_client(settings.address)
    return 0

if __name__ == "__main__":
    status = main()
    sys.exit(status)
