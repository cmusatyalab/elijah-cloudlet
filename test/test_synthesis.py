#!/usr/bin/env python 

import unittest
import sys
import os
from hashlib import sha1
from tempfile import NamedTemporaryFile

# library path
SRC_PATH = "../src/server/"

class Const(object):
    BASE_DISK_PATH                  = "./memory_snapshot/precise.raw"
    BASE_MEMORY_PATH                = "./memory_snapshot/precise.base-mem"
    MODIFIED_MEMORY_SNAPSHOT        = "./memory_snapshot/modified-memory"
    OVERLAY_MEMA_FILE               = "./memory_snapshot/precise.overlay-meta"

    file_list = [BASE_DISK_PATH, BASE_MEMORY_PATH, MODIFIED_MEMORY_SNAPSHOT, OVERLAY_MEMA_FILE]


class TestSynthesisFunction(unittest.TestCase):

    def setUp(self):
        for each_file in Const.file_list:
            if os.path.exists(each_file) == False:
                sys.stderr.write("Cannot find required file for test: %s\n" % \
                        os.path.abspath(each_file))
                sys.exit(1)

    def test_memory_synthesis(self):
        from tool import decomp_overlay
        from lib_cloudlet import recover_launchVM
        Log = open("/dev/null", "rwb")

        meta = Const.OVERLAY_MEMA_FILE
        base_disk = Const.BASE_DISK_PATH

        overlay_filename = NamedTemporaryFile(prefix="cloudlet-overlay-file-")
        meta_info = decomp_overlay(meta, overlay_filename.name, print_out=Log)

        # recover modified VM
        modified_img, modified_mem, fuse, delta_proc, fuse_thread = \
                recover_launchVM(base_disk, meta_info, overlay_filename.name, log=Log)

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
    unittest.main()
