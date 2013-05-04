#!/usr/bin/env python

from __future__ import with_statement

from errno import EACCES
from os.path import realpath
from sys import argv, exit
from threading import Lock
from multiprocessing import Process, JoinableQueue, Queue, Manager

import os

from fuse import FUSE, FuseOSError, Operations, LoggingMixIn


class CacheFuse(LoggingMixIn, Operations):
    def __init__(self, URI_list):
        self.root = realpath('/home/krha/Downloads')
        self.rwlock = Lock()

    def __call__(self, op, path, *args):
        return super(CacheFuse, self).__call__(op, self.root + path, *args)

    def access(self, path, mode):
        if not os.access(path, mode):
            raise FuseOSError(EACCES)

    chmod = os.chmod
    chown = os.chown

    def create(self, path, mode):
        return os.open(path, os.O_WRONLY | os.O_CREAT, mode)

    def flush(self, path, fh):
        return os.fsync(fh)

    def fsync(self, path, datasync, fh):
        return os.fsync(fh)

    def getattr(self, path, fh=None):
        st = os.lstat(path)
        return dict((key, getattr(st, key)) for key in ('st_atime', 'st_ctime',
            'st_gid', 'st_mode', 'st_mtime', 'st_nlink', 'st_size', 'st_uid'))

    getxattr = None

    def link(self, target, source):
        return os.link(source, target)

    listxattr = None
    mkdir = os.mkdir
    mknod = os.mknod
    open = os.open

    def read(self, path, size, offset, fh):
        with self.rwlock:
            os.lseek(fh, offset, 0)
            return os.read(fh, size)

    def readdir(self, path, fh):
        return ['.', '..'] + os.listdir(path)

    readlink = os.readlink

    def release(self, path, fh):
        return os.close(fh)

    def rename(self, old, new):
        return os.rename(old, self.root + new)

    rmdir = os.rmdir

    def statfs(self, path):
        stv = os.statvfs(path)
        return dict((key, getattr(stv, key)) for key in ('f_bavail', 'f_bfree',
            'f_blocks', 'f_bsize', 'f_favail', 'f_ffree', 'f_files', 'f_flag',
            'f_frsize', 'f_namemax'))

    def symlink(self, target, source):
        return os.symlink(source, target)

    def truncate(self, path, length, fh=None):
        with open(path, 'r+') as f:
            f.truncate(length)

    unlink = os.unlink
    utimens = os.utime

    def write(self, path, data, offset, fh):
        with self.rwlock:
            os.lseek(fh, offset, 0)
            return os.write(fh, data)


class CacheFuseBinder(Process):
    def __init__(self, URI_list, fuse_root, command_queue):
        self.URI_list = URI_list
        self.fuse_root = fuse_root
        self.command_queue = command_queue
        Process.__init__(self)

    def run(self):
        print "x"
        self.fuse = FUSE(CacheFuse(self.URI_list), fuse_root, foreground=False)
        print "b"
        while True:
            print "a"
            import time
            time.sleep(1)
            #command = self.command_queue.get()
            #print "handling command : %s" % command
            #if command == 'q':
            #    break;
            


if __name__ == '__main__':
    if len(argv) != 2:
        print('usage: %s <root_uri>' % argv[0])
        exit(1)

    from CloudletCache import Util, _URIInfo
    import CloudletCache
    import requests

    import sys
    compiled_list = Util.get_compiled_URIs(sys.argv[1])
    uri = compiled_list[0].info_dict[_URIInfo.URI]
    parse_ret = requests.utils.urlparse(uri)
    fuse_root = os.path.join(CloudletCache.cache_rootdir, parse_ret.netloc, '.')
    if os.path.exists(fuse_root) == True:
        sys.stderr.write("FUSE directory %s is not empty\n" % fuse_root)
    else:
        os.makedirs(fuse_root)
    sys.stdout.write("mount %s to %s\n" % (sys.argv[1], fuse_root))

    
    command_queue = JoinableQueue()
    fusebinder = CacheFuseBinder(compiled_list, fuse_root, command_queue)
    fusebinder.start()
    while True:
        mesg = raw_input("input : ")
        command_queue.put(mesg)
    fusebinder.join()

