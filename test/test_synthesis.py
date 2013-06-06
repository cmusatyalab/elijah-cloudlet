#!/usr/bin/env python 

import unittest
import sys
import os
import threading
from hashlib import sha1
from tempfile import NamedTemporaryFile

# library path
SRC_PATH = "../src/server/"

class Const(object):
    BASE_DISK_PATH                  = "./memory_snapshot/precise.raw"
    BASE_MEMORY_PATH                = "./memory_snapshot/precise.base-mem"
    MODIFIED_MEMORY_SNAPSHOT        = "./memory_snapshot/modified-memory"
    OVERLAY_MEMA_FILE               = "./memory_snapshot/precise.overlay-meta"

    file_list = [BASE_DISK_PATH, BASE_MEMORY_PATH]


class App_installer(threading.Thread):
    def __init__(self, ssh_ip, ssh_port):
        self.ssh_ip = ssh_ip
        self.ssh_port = ssh_port
        threading.Thread.__init__(self, target=self.run)

    def run(self):
        import paramiko
        import socket
        import time

        ssh = paramiko.SSHClient()
        ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
        counter = 0
        while counter < 60:
            try:
                ssh.connect(self.ssh_ip, port=self.ssh_port, username='cloudlet', password='cloudlet')
                break
            except socket.error:
                counter += 1
                sys.stdout.write(".")
                sys.stdout.flush()
                time.sleep(1)
        sys.stdout.write("\n")
        sys.stdout.flush()
        stdin, stdout, stderr = ssh.exec_command("dd if=/dev/urandom of=./test bs=1M count=100")
        stdin, stdout, stderr = ssh.exec_command("sync")
        ssh.close()


class TestSynthesisFunction(unittest.TestCase):

    def setUp(self):
        for each_file in Const.file_list:
            if os.path.exists(each_file) == False:
                sys.stderr.write("Cannot find required file for test: %s\n" % \
                        os.path.abspath(each_file))
                sys.exit(1)

    def test_synthesis(self):
        #self.create_base()
        self.create_overlay()
        self.perform_synthesis()

    def create_base(self):
        pass

    def create_overlay(self):
        from Configuration import Options

        disk_path = Const.BASE_DISK_PATH
        options = Options()
        options.disk_only = False
        options.TRIM_SUPPORT = False
        options.FREE_SUPPORT = False 
        options.MEMORY_SAVE_PATH = Const.MODIFIED_MEMORY_SNAPSHOT
        overlay = cloudlet.VM_Overlay(disk_path, options)
        app_installer = App_installer('localhost', 2222)
        overlay.start()
        app_installer.start()
        app_installer.join()
        overlay.join()

        print "[INFO] overlay metafile : %s" % overlay.overlay_metafile
        print "[INFO] overlay : %s" % str(overlay.overlay_files[0])

    def perform_synthesis(self):
        from tool import decomp_overlay

        Log = open("/dev/null", "w+b")
        #Log = sys.stdout

        meta = Const.OVERLAY_MEMA_FILE
        base_disk = Const.BASE_DISK_PATH

        overlay_filename = NamedTemporaryFile(prefix="cloudlet-overlay-file-")
        meta_info = decomp_overlay(meta, overlay_filename.name, print_out=Log)

        # recover modified VM
        modified_img, modified_mem, fuse, delta_proc, fuse_thread = \
                cloudlet.recover_launchVM(base_disk, meta_info, overlay_filename.name, log=Log)

        delta_proc.start()
        fuse_thread.start()
        delta_proc.join()
        fuse_thread.join()

        residue_img = os.path.join(fuse.mountpoint, 'disk', 'image')
        residue_mem = os.path.join(fuse.mountpoint, 'memory', 'image')

        # compare hash
        import tool
        print "[INFO] Modified disk is recovered at %s" % residue_img
        print "[INFO] Modified memory is recovered at %s" % residue_mem
        print "getting sha1 for comparison"
        sha1_modified_img = tool.sha1_fromfile(residue_img)
        sha1_modified_mem = tool.sha1_fromfile(residue_mem)
        sha1_original_mem = tool.sha1_fromfile(Const.MODIFIED_MEMORY_SNAPSHOT)
        fuse.terminate()

        self.assertEqual(sha1_original_mem, sha1_modified_mem, "recover memory should be same with original")


    def tearDown(self):
        pass


if __name__ == "__main__":
    sys.path.append(SRC_PATH)
    import lib_cloudlet as cloudlet
    unittest.main()
