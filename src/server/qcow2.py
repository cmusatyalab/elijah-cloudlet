#!/usr/bin/python
#
# Dump Qcow2 file structure
#
# qemu-img create -f qcow2 -o preallocation=off      /tmp/test.qcow2 1G
# qemu-img create -f qcow2 -o preallocation=metadata /tmp/test.qcow2 1G
# qemu-io -c 'write 0 512' -c flush -c quit          /tmp/test.qcow2
# qemu-img snapshot -c Test1234                      /tmp/test.qcow2
import sys
import hashlib
import os
import math
from struct import Struct as _Struct
import struct
import time
import mmap
from optparse import OptionParser
from operator import itemgetter
from KVMMemory import DeltaList
from KVMMemory import DeltaItem

SECTOR_SIZE = 512

class Qcow2Error(Exception):
    pass

def human(size):
    '''Print size as human readable string.
    >>> human(0)
    '0 B'
    >>> human(1)
    '1 B'
    >>> human(1023)
    '1023 B'
    >>> human(1024)
    '1 KiB'
    '''
    if size < 1024: return '%s B' % size
    size >>= 10
    if size < 1024: return '%s KiB' % size
    size >>= 10
    if size < 1024: return '%s MiB' % size
    size >>= 10
    if size < 1024: return '%s GiB' % size
    size >>= 10
    if size < 1024: return '%s TiB' % size
    size >>= 10
    return '%s PiB' % size

def round_up(value, size):
    '''Round value ot to next size.
    >>> round_up(0, 2)
    0
    >>> round_up(1, 2)
    2
    '''
    bitmask = size - 1
    assert not (size & bitmask), "size is not a power of 2"
    value += bitmask
    value &= ~bitmask
    return value

def aligned(value, size):
    '''Test if value is aligned to size.
    >>> aligned(1, 2)
    False
    >>> aligned(4, 2)
    True
    '''
    bitmask = size - 1
    value &= bitmask
    return value == 0

def dump(offset, data, valid=None):
    if options.offset:
        print '%016x:' % offset,
    if options.valid:
        if valid is None:
            print " ",
        elif valid:
            print "+",
        else:
            print "-",
    print data

class Struct(_Struct):
    def __init__(self, fmt):
        f = ''
        for l in fmt.splitlines():
            try:
                i = l.index('#')
                if i >= 0:
                    l = l[:i]
            except ValueError:
                pass
            l = l.strip()
            if l:
                f = f + l
        _Struct.__init__(self, f)

class Qcow2(object):
    Header = Struct('''> #big-endian
            4s    #uint32_t magic;
            I    #uint32_t version;
            Q    #uint64_t backing_file_offset;
            I    #uint32_t backing_file_size;
            I    #uint32_t cluster_bits;
            Q    #uint64_t size; /* in bytes */
            I    #uint32_t crypt_method;
            I    #uint32_t l1_size;
            Q    #uint64_t l1_table_offset;
            Q    #uint64_t refcount_table_offset;
            I    #uint32_t refcount_table_clusters;
            I    #uint32_t nb_snapshots;
            Q    #uint64_t snapshots_offset;
            ''')

    SnapshotHeader = Struct('''> #big-endian
            Q    #uint64_t l1_table_offset;
            I    #uint32_t l1_size;
            H    #uint16_t id_str_size;
            H    #uint16_t name_size;
            I    #uint32_t date_sec;
            I    #uint32_t date_nsec;
            Q    #uint64_t vm_clock_nsec;
            I    #uint32_t vm_state_size;
            I    #uint32_t extra_data_size; /* for extension */
                #/* extra data follows */
                #/* id_str follows */
                #/* name follows  */
            ''')

    LD512 = int(math.log(512) / math.log(2))
    LD2M = int(math.log(2<<20) / math.log(2))

    def __init__(self, f):
        self.f = f
        self.fsize = os.fstat(f.fileno()).st_size

    def update_bits(self, cluster_bits, size):
        self.size = size
        self.cluster_bits = cluster_bits
        self.cluster_size = 1L << self.cluster_bits # bytes
        self.cluster_mask = self.cluster_size - 1
        self.l2_bits = self.cluster_bits - 3 # each entry is u64
        self.l2_size = 1L << self.l2_bits # entries
        self.l2_mask = (self.l2_size - 1) << self.cluster_bits
        self.l1_bits = 64 - self.cluster_bits - self.l2_bits
        self.l1_size_min = self.size_to_l1(self.size)
        self.l1_size_max = 1L << self.l1_bits
        self.l1_mask = -1L & ~self.cluster_bits & ~self.l2_bits

        self.RCB = Struct('''>    #big-endian
                %dH''' % (self.cluster_size / 2))
        self.L2 = Struct('''>    #big-endian
                %dQ    #offset''' % self.l2_size)

    def size_to_l1(self, size):
        """Calculate the number of entries in the L1 table for site bytes."""
        shift = self.cluster_bits + self.l2_bits
        return round_up(size, 1L << shift) >> shift

    def offset_to_l1(self, offset):
        """Return index in L1 table for offset."""
        offset &= self.l1_mask
        offset >>= self.cluster_bits + self.l2_bits
        return offset

    def offset_to_l2(self, offset):
        """Return index in L2 table for offset."""
        offset &= self.l2_mask
        offset >>= self.cluster_bits
        return offset
    
    def get_sectors(self):
        self.f.seek(0, os.SEEK_SET)
        d = self.f.read(Qcow2.Header.size)
        magic, version, backing_file_offset, backing_file_size, cluster_bits, size, crypt_method, l1_size, l1_table_offset, refcount_table_offset, refcount_table_clusters, nb_snapshots, snapshots_offset = Qcow2.Header.unpack_from(d)
        self.update_bits(cluster_bits, size)
        self.backing_file = None
        if backing_file_offset and backing_file_size > 0:
            self.f.seek(backing_file_offset, os.SEEK_SET)
            self.backing_file = self.f.read(backing_file_size)
        assert 0 < self.l2_bits
        sectors_list = []
        self.dump_L1(l1_table_offset, l1_size, '  ', sectors_list=sectors_list)
        return sectors_list

    def dump_Header(self, offset=0):
        """Dump QCow2 header."""
        self.f.seek(offset, os.SEEK_SET)
        d = self.f.read(Qcow2.Header.size)
        magic, version, backing_file_offset, backing_file_size, cluster_bits, size, crypt_method, l1_size, l1_table_offset, refcount_table_offset, refcount_table_clusters, nb_snapshots, snapshots_offset = Qcow2.Header.unpack_from(d)
        dump(offset, 'magic=%r' % magic, magic == 'QFI\xfb')
        dump(offset, 'version=%d' % version, version == 2)
        dump(offset, 'backing_file_offset=0x%016x' % backing_file_offset, 0 <= backing_file_offset < self.fsize)
        dump(offset, 'backing_file_size=0x%08x' % backing_file_size, 0 <= backing_file_offset + backing_file_size < self.fsize)
        dump(offset, 'cluster_bits=%d (%s)' % (cluster_bits, human(1L << cluster_bits)), Qcow2.LD512 <= cluster_bits <= Qcow2.LD2M)
        self.update_bits(cluster_bits, size)
        assert 0 < self.l2_bits
        dump(offset, 'size=%s' % human(size), True)
        dump(offset, 'crypt_method=0x%08x' % crypt_method, 0 <= crypt_method <= 1)
        dump(offset, 'l1_size=%d' % l1_size, self.l1_size_min <= l1_size)
        dump(offset, 'l1_table_offset=0x%016x' % l1_table_offset, 0 <= l1_table_offset < self.fsize and aligned(l1_table_offset, self.cluster_size))
        dump(offset, 'l2_size=%d' % self.l2_size, True)
        dump(offset, 'refcount_table_offset=0x%016x' % refcount_table_offset, 0 <= refcount_table_offset < self.fsize and aligned(refcount_table_offset, self.cluster_size))
        dump(offset, 'refcount_table_clusters=%d' % refcount_table_clusters, 0 <= refcount_table_offset + refcount_table_clusters * self.cluster_size < self.fsize)
        dump(offset, 'nb_snapshots=%d' % nb_snapshots, 0 <= nb_snapshots)
        dump(offset, 'snapshots_offset=0x%016x' % snapshots_offset, 0 <= snapshots_offset < self.fsize and aligned(snapshots_offset, self.cluster_size))

        if backing_file_offset and backing_file_size > 0:
            self.f.seek(backing_file_offset, os.SEEK_SET)
            backing_file = self.f.read(backing_file_size)
            dump(backing_file_offset, 'backing_file=%r' % backing_file, True)

        self.dump_Refcounts(refcount_table_offset, refcount_table_clusters, nb_snapshots)
        self.dump_L1(l1_table_offset, l1_size)
        self.dump_Snapshots(snapshots_offset, nb_snapshots)

    def load_RT(self, refcount_table_offset, refcount_table_clusters):
        """Load reference count table."""
        refcount_table_len = self.cluster_size * refcount_table_clusters / 8
        RCT = Struct('''>    #big-endian
                %dQ''' % refcount_table_len)
        self.f.seek(refcount_table_offset)
        d = self.f.read(RCT.size)
        refcount_table = RCT.unpack_from(d)
        return refcount_table

    def load_RB(self, refcount_block_offset):
        """Load reference count block."""
        self.f.seek(refcount_block_offset)
        d = self.f.read(self.RCB.size)
        refcount_block = self.RCB.unpack_from(d)
        return refcount_block

    def dump_Refcounts(self, refcount_table_offset, refcount_table_clusters, nb_snapshots):
        """Dump reference count table."""
        if not options.rcdump: return
        refcount_table = self.load_RT(refcount_table_offset, refcount_table_clusters)
        for i in range(len(refcount_table)):
            offset = refcount_table[i]
            if not offset: continue
            dump(refcount_table_offset, 'refcount_block[%d]=0x%016x' % (i, offset), 0 <= offset < self.fsize and aligned(offset, self.cluster_size))
            refcount_block = self.load_RB(offset)
            for j in range(len(refcount_block)):
                refcount = refcount_block[j]
                if not refcount: continue
                cluster = (i * (self.cluster_size / 2) + j) * self.cluster_size
                dump(offset, ' [%d][%d]=%016x: %d' % (i, j, cluster, refcount), 0 <= refcount <= nb_snapshots + 1 and 0 <= cluster < self.fsize)

    def dump_Snapshots(self, snapshots_offset, nb_snapshots):
        """Dump snapshot index."""
        if not options.ssdump: return
        if nb_snapshots == 0: return
        self.f.seek(snapshots_offset, os.SEEK_SET)
        for snapshot in range(nb_snapshots):
            dump(snapshots_offset, ' Snapshot #%d' % snapshot)
            self.dump_SnapshotHeader(self.f.tell())

    def load_L1(self, l1_table_offset, l1_size):
        """Load first level table."""
        L1 = Struct('''>    #big-endian
                %dQ    #offset''' % l1_size)
        self.f.seek(l1_table_offset)
        d = self.f.read(L1.size)
        l1_table = L1.unpack_from(d)
        return l1_table

    def dump_L1(self, l1_table_offset, l1_size, prefix='', sectors_list=None):
        """Dump first level table."""
        if not options.l1dump: return
        l1_table = self.load_L1(l1_table_offset, l1_size)
        for i in range(l1_size):
            copied = (l1_table[i] & 1L << 63) != 0
            compressed = (l1_table[i] & 1L << 62) != 0
            offset = l1_table[i] & ~(3L << 62)
            if sectors_list == None:
                dump(l1_table_offset, '%sL1[%d]=0x%016x' % (prefix, i, l1_table[i]), 0 <= offset < self.fsize)
            if offset > 0:
                self.dump_L2(offset, prefix, i, sectors_list=sectors_list)

    def load_L2(self, l2_table_offset):
        """Load second level table."""
        self.f.seek(l2_table_offset)
        d = self.f.read(self.L2.size)
        l2_table = self.L2.unpack_from(d)
        return l2_table
    
    def dump_L2(self, l2_table_offset, prefix='', l1_index=-1, sectors_list=None):
        global SECTOR_SIZE
        """Dump second level table."""
        if not options.l2dump: return
        l2_table = self.load_L2(l2_table_offset)
        for i in range(self.l2_size):
            copied = (l2_table[i] & 1L << 63) != 0
            compressed = (l2_table[i] & 1L << 62) != 0
            offset = l2_table[i] & ~(3L << 62)
            if not offset: continue
            
            if sectors_list != None:
                position = (l1_index << (self.cluster_bits+self.l2_bits)) + (i << self.cluster_bits)
                length = 1<<self.cluster_bits
                #print "l1: %ld, l2(%ld): %ld, pos: %ld, length:%ld" % (l1_index, i, i<<self.l2_bits, position, length/SECTOR_SIZE)
                for index in xrange(length/SECTOR_SIZE):
                    sectors_list.append((position/SECTOR_SIZE+index, offset+index*SECTOR_SIZE))
            else:
                dump(l2_table_offset, '%s L2[%d]=0x%016x' % (prefix, i, l2_table[i]), 0 <= offset < self.fsize)


    def dump_SnapshotHeader(self, offset):
        d = self.f.read(Qcow2.SnapshotHeader.size)
        l1_table_offset, l1_size, id_str_size, name_size, date_sec, date_nsec, vm_clock_nsec, vm_state_size, extra_data_size = Qcow2.SnapshotHeader.unpack_from(d)
        dump(offset, '  l1_table_offset=0x%016x' % l1_table_offset, 0 <= l1_table_offset < self.fsize and aligned(l1_table_offset, self.cluster_size))
        dump(offset, '  l1_size=%d' % l1_size, self.l1_size_min <= l1_size)
        dump(offset, '  id_str_size=%d' % id_str_size, 0 <= id_str_size)
        dump(offset, '  name_size=%d' % name_size, 0 <= name_size)
        dump(offset, '  date_sec=%s' % time.strftime('%Y-%m-%d %H:%M:%S', time.localtime(date_sec)), 0 <= date_sec)
        dump(offset, '  date_nsec=%d' % date_nsec, 0 <= date_nsec < 1000000000)
        dump(offset, '  vm_clock_nsec=%d' % vm_clock_nsec, 0 <= vm_clock_nsec)
        dump(offset, '  vm_state_size=%s' % human(vm_state_size), 0 <= vm_state_size)
        dump(offset, '  extra_data_size=0x%08x' % extra_data_size, 0 <= extra_data_size)
        if extra_data_size > 0:
            extra_data = self.f.read(extra_data_size)
            dump(offset, '  extra_data=%d' % len(extra_data), len(extra_data) == extra_data_size)
        if id_str_size > 0:
            id_str = self.f.read(id_str_size)
            dump(offset, '  id_str=%r' % id_str, len(id_str) == id_str_size)
        if name_size > 0:
            name = self.f.read(name_size)
            dump(offset, '  name=%r' % name, len(name) == name_size)
        if vm_state_size > 0:
            pos = self.size_to_l1(self.size)
            dump(offset, '  vm_state=L1[%d..]' % pos, self.l1_size_min < pos <= l1_size)
        ssize = Qcow2.SnapshotHeader.size + extra_data_size + id_str_size + name_size
        if ssize & 7:
            pad = self.f.read(8 - ssize & 7)

        pos = self.f.tell()
        self.dump_L1(l1_table_offset, l1_size, '  ')
        self.f.seek(pos)

    def read(self, pos, count=1):
        self.f.seek(0)
        d = self.f.read(Qcow2.Header.size)
        magic, version, backing_file_offset, backing_file_size, cluster_bits, size, crypt_method, l1_size, l1_table_offset, refcount_table_offset, refcount_table_clusters, nb_snapshots, snapshots_offset = Qcow2.Header.unpack_from(d)
        if crypt_method:
            raise Qcow2Error('Qcow2 encryption')

        self.update_bits(cluster_bits, size)

        if pos >= size:
            raise EOFError()
        cluster_index = pos & self.cluster_mask
        l2_index = self.offset_to_l2(pos)
        l1_index = self.offset_to_l1(pos)
        #print "l1_index:%ld, l2_index:%ld" % (l1_index, l2_index)

        l1_table = self.load_L1(l1_table_offset, l1_size)
        l2_table_offset = l1_table[l1_index]
        if not l2_table_offset:
            print "No l2 table offset"
            return None # '\0' * min(self.cluster_size * self.l2_size, size - pos)
        if l2_table_offset & (1L << 62):
            raise Qcow2Error('Qcow2 compression')
        l2_table_offset &= ~(3L << 62)

        l2_table = self.load_L2(l2_table_offset)
        cluster_offset = l2_table[l2_index]
        if not cluster_offset:
            print "No Cluster offset"
            return None # '\0' * min(self.cluster_size, size - pos)
        if cluster_offset & (1L << 62):
            raise Qcow2Error('Qcow2 compression')
        cluster_offset &= ~(3L << 62)
        print "cluster_offset : 0x%x, cluster_index" % cluster_offset, cluster_index
        print "address : %ld" % (cluster_offset + cluster_index)

        self.f.seek(cluster_offset)
        d = self.f.read(self.cluster_size)
        return d[cluster_index]


    @staticmethod
    def _find_hash_matching(hash_list, value):
        pass

    @staticmethod
    def get_modified_sectors(s_filepath, s_hashlist, m_path):
        # return list of DeltaItem
        global SECTOR_SIZE

        m_file = open(m_path, "rb")
        m_file.seek(0, os.SEEK_SET)
        d = m_file.read(Qcow2.Header.size)
        magic, version, backing_file_offset, backing_file_size, cluster_bits, size, crypt_method, l1_size, l1_table_offset, refcount_table_offset, refcount_table_clusters, nb_snapshots, snapshots_offset = Qcow2.Header.unpack_from(d)

        #sanity check
        if magic != "QFI\xfb" or version != 2:
            raise Qcow2Error("modified file is not Qcow2 format")
        if (not backing_file_offset) or (backing_file_size <= 0):
            raise Qcow2Error("modified file does not have backing file")
        m_file.seek(backing_file_offset, os.SEEK_SET)
        backing_file = m_file.read(backing_file_size)
        if os.path.absname(s_filepath) != backing_file:
            raise Qcow2Error("backing file of modified file does not same with given base")

        # Load L1 table
        L1 = Struct('''>    #big-endian
                %dQ    #offset''' % l1_size)
        m_file.seek(l1_table_offset)
        l1_table = L1.unpack_from(m_file.read(L1.size))

        # set cluster bits
        cluster_bits = cluster_bits
        l2_bits = cluster_bits - 3 # each entry is u64
        l2_size = 1L << l2_bits # entries
        L2 = Struct('''>    #big-endian
                %dQ    #offset''' % l2_size)

        delta_list = []
        # traverse each L1 entry
        for l1_index in range(l1_size):
            offset = l1_table[l1_index] & ~(3L << 62)
            if not (offset > 0): continue
            # Load L2 table
            m_file.seek(offset)
            d = m_file.read(L2.size)
            l2_table = L2.unpack_from(d)

            #traverse each L2 entry
            for l2_index in range(l2_size):
                offset = l2_table[l2_index] & ~(3L << 62)
                if not offset:
                    continue
                
                position = (l1_index << (cluster_bits+l2_bits)) + (l2_index << cluster_bits)
                original_offset = m_file.tell()
                m_file.seek(position)
                m_data = m_file.read(1<<cluster_bits)
                m_file.seek(original_offset)
                m_hash = hashlib.sha256(m_data).digest()
                s_offset = Qcow2._find_hash_matching(s_hashlist, m_hash)
                if s_offset >= 0:
                    #get xdelta comparing self.raw
                    source_data = self.get_raw_data(offset, self.RAM_PAGE_SIZE)
                    try:
                        patch = tool.diff_data(source_data, data, 2*len(source_data))
                        delta_item = DeltaItem(offset, self.RAM_PAGE_SIZE, 
                                hash_value=sha256(data).digest(),
                                ref_id=DeltaItem.REF_XDELTA,
                                data_len=len(patch),
                                data=patch)
                    except IOError:
                        print "[INFO] xdelta failed, so save it as raw"
                        #print "%ld, %ld" % (len(source_data), len(data))
                        #open("./error_source", "wb").write(source_data)
                        #open("./error_modi", "wb").write(data)
                        #sys.exit(1)
                        patch = data
                        delta_item = DeltaItem(offset, self.RAM_PAGE_SIZE, 
                                hash_value=sha256(data).digest(),
                                ref_id=DeltaItem.REF_RAW,
                                data_len=len(patch),
                                data=patch)
                    hash_list.append(delta_item)


                '''
                for index in xrange(length/SECTOR_SIZE):
                    sectors_list.append((position/SECTOR_SIZE+index, offset+index*SECTOR_SIZE))
                '''


    @staticmethod
    def recover_qcow2(header_info, delta_list, out_path):
        if type(header_info) != Qcow2.Header:
            raise Qcow2Error("Need Qcow2 Header")
        if len(delta_list) == 0 or type(delta_list[0]) != DeltaItem:
            raise Qcow2Error("Need list of DeltaItem")
        pass


def obsolete_main():
    usage = 'usage: %prog [options] file.qcwo2'
    parser = OptionParser(usage=usage)
    parser.set_defaults(rcdump=True, l1dump=True, l2dump=True, ssdump=True, offset=True, valid=True)
    parser.add_option('-r', '--rc', dest="rcdump", action='store_false', help='hide refcount')
    parser.add_option('-1', '--l1', dest="l1dump", action='store_false', help='hide l1 dump')
    parser.add_option('-2', '--l2', dest="l2dump", action='store_false', help='hide l2 dump')
    parser.add_option('-s', '--snap', dest="ssdump", action='store_false', help='hide snapshot dump')
    parser.add_option('-o', '--offset', dest="offset", action='store_false', help='hide offset')
    parser.add_option('-v', '--valid', dest="valid", action='store_false', help='hide valid')
    parser.add_option('-t', '--test', dest="test", action='store_true', help='run unit test')
    options, args = parser.parse_args()

    if options.test:
        import doctest
        doctest.testmod()

    try:
        NAME = args[0]
    except IndexError:
        parser.error('Missing argument')

    f = open(NAME, 'r')
    try:
        q = Qcow2(f)
        q.dump_Header()
        #q.read(offset)
        #print q.read((1 << 30 - 1))
        #print q.read(1 << 30)
    finally:
        f.close()

class BaseDisk(object):
    HASH_FILE_MAGIC = 0x1145511b
    HASH_FILE_VERSION = 0x00000001

    def __init__(self, base_file):
        self.f = base_file
        self.f.seek(0, os.SEEK_SET)
        d = self.f.read(Qcow2.Header.size)
        magic, version, backing_file_offset, backing_file_size, cluster_bits, size, crypt_method, l1_size, l1_table_offset, refcount_table_offset, refcount_table_clusters, nb_snapshots, snapshots_offset = Qcow2.Header.unpack_from(d)

        # base disk type checking
        if magic == 'QFI\xfb' and version == 2:
            if backing_file_offset and backing_file_size > 0:
                raise Qcow2Error("Base VM should not have backing file")
            self.file_type = 'qcow2'
        else:
            self.file_type = 'raw'

    def get_hashlist(self):
        # get hash
        self.f.seek(0, 2)
        file_size = self.f.tell()
        self.f.seek(0, os.SEEK_SET)
        chunk_size = SECTOR_SIZE*128
        hash_dic = {}
        total = 0
        duplicated = 0
        if self.file_type == 'raw':
            while True:
                offset = self.f.tell()
                data = self.f.read(chunk_size)
                if (not data) or (len(data) != chunk_size):
                    break
                hash_value = hashlib.sha256(data).digest()
                if not hash_dic.get(hash_value):
                    hash_dic[hash_value] = (offset, chunk_size, hash_value)
                else:
                    duplicated += 1

                total += 1
                if duplicated%10000 == 0:
                    print "[%d%%] total: %ld, dup:%ld --> %f" % \
                            ((100*offset/file_size), total, duplicated, 1.0*duplicated/total)
        elif self.file_type == 'qcow2':
            hash_list, backing_file = get_hash_list(self.f)
        else:
            raise Qcow2Error("We support raw and qcow2 as a base disk")

        hash_list = hash_dic.values()
        hash_list.sort(key=itemgetter(2)) # sort by hash value
        return hash_list

    @staticmethod
    def hashlist_tofile(hash_list, out_path):
        out_file = open(out_path, "wb")

        # Write MAGIC & VERSION
        out_file.write(struct.pack("<q", BaseDisk.HASH_FILE_MAGIC))
        out_file.write(struct.pack("<q", BaseDisk.HASH_FILE_VERSION))
        for (offset, length, hash_value) in hash_list:
            data = struct.pack("<QI32s", offset, length, hash_value)
            out_file.write(data)
        out_file.close()

    @staticmethod
    def hashlist_fromfile(hash_filepath):
        hash_list = []
        hash_file = open(hash_filepath, "rb")
        magic, version = struct.unpack("<qq", hash_file.read(8+8))
        if magic != BaseDisk.HASH_FILE_MAGIC:
            msg = "Hash file magic number(%ld != %ld) does not match" % (magic, BaseDisk.HASH_FILE_MAGIC)
            raise IOError(msg)
        if version != BaseDisk.HASH_FILE_VERSION:
            msg = "Hash file version(%ld != %ld) does not match" % \
                    (version, BaseDisk.HASH_FILE_VERSION)
            raise IOError(msg)

        item_length = 8+4+32
        while True:
            data = hash_file.read(item_length)
            if (data == None) or (len(data) != item_length):
                break
            offset, length, hash_value = struct.unpack("<QI32s", data)
            hash_list.append((offset, length, hash_value))
        return hash_list

    @staticmethod
    def get_delta(source_hashlist, target_hashlist):
        pass
        #item = DeltaItem(offset, offset_len, hash_value, ref_id, data_len, data)


def get_hash_list(qcow2_file):
    # return list of (offset, length, hash value)
    # list is sorted by offset value
    qcow2_file.seek(0, os.SEEK_SET)
    q = Qcow2(qcow2_file)
    modi_sectors = q.get_sectors()
    modi_sectors.sort(key=itemgetter(1)) # sort by offset at qcow2 file
    hash_list = []
    for sector, offset in modi_sectors:
        qcow2_file.seek(offset)
        sector_data = qcow2_file.read(SECTOR_SIZE)
        hash_value = hashlib.sha256(sector_data).digest()
        hash_list.append((offset, SECTOR_SIZE, hash_value))
    return hash_list, q.backing_file


if __name__ == '__main__':
    usage = 'usage: %prog [options] file.qcwo2'
    parser = OptionParser(usage=usage)
    parser.set_defaults(rcdump=True, l1dump=True, l2dump=True, ssdump=True, offset=True, valid=True)
    parser.add_option('-m', '--modi', dest="modi", action='store_true', help='Get Modified Sectors')
    parser.add_option('-c', '--comp', dest="comp", action='store_true', help='Compare qcow2 with KVM result')
    parser.add_option('-r', '--read', dest="read", action='store_true', help='Read Modified Sectors')
    parser.add_option('-l', '--list', dest="list", action='store_true', help='List hash of each sector')
    parser.add_option('-s', '--hash', dest="hash", action='store_true', help='Get Hashlist of Base')
    parser.add_option('-d', '--delta', dest="delta", action='store_true', help='Get delta comparing hash')
    options, args = parser.parse_args()

    try:
        qcow2_path = args[0]
    except IndexError:
        parser.error('Missing argument')

    if options.list:
        hash_list, backing_filepath = get_hash_list(open(qcow2_path, "r"))
    elif options.hash:
        base_disk = BaseDisk(open(qcow2_path, "r"))
        hash_list = base_disk.get_hashlist()
        BaseDisk.hashlist_tofile(hash_list, qcow2_path+".hash")
    elif options.delta:
        hash_path = args[0]
        modi_path = args[1]
        hash_list = BaseDisk.hashlist_fromfile(hash_path) #sorted by hash value

        # 1. get modified sector
        modi_hashlist = get_hash_list(modi_path) #sorted by offset

        # 2. find shared with Base disk
        delta_list = BaseDisk.get_delta(hash_list, modi_hashlist)

        # 3. find shared within self
        DeltaList.get_self_delta(delta_list)
    elif options.comp:
        qcow2_file = open(qcow2_path, "r")
        try:
            q = Qcow2(qcow2_file)
            modi_sectors = q.get_sectors()
            modi_sectors = [sector for sector, offset in modi_sectors]
            kvm_sectors = open("/home/krha/cloudlet/tmp/qcow2/run_sectors", "r").read().split("\n")
            for sector in kvm_sectors:
                if len(sector.strip()) == 0:
                    continue
                sector = long(sector.strip())
                if sector not in modi_sectors:
                    print "Error, Sector(%ld) is not at modified list" % (sector)

            #statistics
            print "KVM detected modified sector : %d" % len(kvm_sectors)
            print "QCOW2 modified sector : %d" % len(modi_sectors)
            print "QCOW2/KVM ratio : %f" % (1.0*len(modi_sectors)/len(kvm_sectors))
            print "Cluster_Size/Sector_size : %f" % (1.0*(2<<16)/512)

        finally:
            qcow2_file.close()
    elif options.read:
        qcow2_file = open(qcow2_path, "r")
        try:
            q = Qcow2(qcow2_file)
            modi_sectors = q.get_sectors()
            modi_sectors.sort(key=itemgetter(1)) # sort by offset at qcow2 file
            for sector, offset in modi_sectors:
                print "offset : %ld" % offset
            print "-"*20
            min_offset = modi_sectors[0][1]
            max_offset = modi_sectors[-1][1] + SECTOR_SIZE
            print "file size : %ld" % os.path.getsize(qcow2_path)
            print "min offset: %ld" % min_offset
            print "max offset: %ld" % max_offset
        finally:
            qcow2_file.close()
    elif options.modi:
        hash_list, backing_filepath = get_hash_list(open(qcow2_path, "r"))
        print backing_filepath
        if not backing_filepath:
            sys.stderr.write("No backing file")
            sys.exit(1)
        print backing_filepath
        backing_file = open(backing_filepath, "r")
        raw_mmap = mmap.mmap(backing_file.fileno(), 0, prot=mmap.PROT_READ)
        for (offset, length, hash_value) in hash_list:
            raw_hash_value = hashlib.sha256(raw_mmap[offset:offset+length]).digest()
            if hash_value == raw_hash_value:
                print "%ld --> same value" % (offset)
            else:
                print "%ld --> diff value" % (offset)
        backing_file.close()

