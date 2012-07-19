#!/usr/bin/python
#
# Dump Qcow2 file structure
#
# qemu-img create -f qcow2 -o preallocation=off      /tmp/test.qcow2 1G
# qemu-img create -f qcow2 -o preallocation=metadata /tmp/test.qcow2 1G
# qemu-io -c 'write 0 512' -c flush -c quit          /tmp/test.qcow2
# qemu-img snapshot -c Test1234                      /tmp/test.qcow2
import sys
import os
import math
from struct import Struct as _Struct
import time
from optparse import OptionParser

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
			4s	#uint32_t magic;
			I	#uint32_t version;
			Q	#uint64_t backing_file_offset;
			I	#uint32_t backing_file_size;
			I	#uint32_t cluster_bits;
			Q	#uint64_t size; /* in bytes */
			I	#uint32_t crypt_method;
			I	#uint32_t l1_size;
			Q	#uint64_t l1_table_offset;
			Q	#uint64_t refcount_table_offset;
			I	#uint32_t refcount_table_clusters;
			I	#uint32_t nb_snapshots;
			Q	#uint64_t snapshots_offset;
			''')

	SnapshotHeader = Struct('''> #big-endian
			Q	#uint64_t l1_table_offset;
			I	#uint32_t l1_size;
			H	#uint16_t id_str_size;
			H	#uint16_t name_size;
			I	#uint32_t date_sec;
			I	#uint32_t date_nsec;
			Q	#uint64_t vm_clock_nsec;
			I	#uint32_t vm_state_size;
			I	#uint32_t extra_data_size; /* for extension */
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

		self.RCB = Struct('''>	#big-endian
				%dH''' % (self.cluster_size / 2))
		self.L2 = Struct('''>	#big-endian
				%dQ	#offset''' % self.l2_size)

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
		RCT = Struct('''>	#big-endian
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
		L1 = Struct('''>	#big-endian
				%dQ	#offset''' % l1_size)
		self.f.seek(l1_table_offset)
		d = self.f.read(L1.size)
		l1_table = L1.unpack_from(d)
		return l1_table

	def dump_L1(self, l1_table_offset, l1_size, prefix=''):
		"""Dump first level table."""
		if not options.l1dump: return
		l1_table = self.load_L1(l1_table_offset, l1_size)
		for i in range(l1_size):
			copied = (l1_table[i] & 1L << 63) != 0
			compressed = (l1_table[i] & 1L << 62) != 0
			offset = l1_table[i] & ~(3L << 62)
			dump(l1_table_offset, '%sL1[%d]=0x%016x' % (prefix, i, l1_table[i]), 0 <= offset < self.fsize)
			if offset > 0:
				self.dump_L2(offset, prefix, i)

	def load_L2(self, l2_table_offset):
		"""Load second level table."""
		self.f.seek(l2_table_offset)
		d = self.f.read(self.L2.size)
		l2_table = self.L2.unpack_from(d)
		return l2_table
	
	def dump_L2(self, l2_table_offset, prefix='', l1_index=-1):
		"""Dump second level table."""
		if not options.l2dump: return
		l2_table = self.load_L2(l2_table_offset)
		for i in range(self.l2_size):
		    copied = (l2_table[i] & 1L << 63) != 0
		    compressed = (l2_table[i] & 1L << 62) != 0
		    offset = l2_table[i] & ~(3L << 62)
		    if not offset: continue
		    dump(l2_table_offset, '%s L2[%d]=0x%016x' % (prefix, i, l2_table[i]), 0 <= offset < self.fsize)
		    if l1_index != -1:
		        SECTOR_SIZE = 512
		        pos = (l1_index << (self.cluster_bits+self.l2_bits)) + (i << self.cluster_bits)
		        length = 1<<self.cluster_bits
                print "l1: %ld, l2: %ld, pos: %ld, length:%ld" % (l1_index, i<<self.l2_bits, pos, length/SECTOR_SIZE)
                print "sector : %ld" % (pos/SECTOR_SIZE)
                '''
                for index in xrange(length/SECTOR_SIZE):
                    print "sector : %ld\n" % (pos/SECTOR_SIZE*index)
                '''

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
			raise NotImplemented('Qcow2 encryption')

		self.update_bits(cluster_bits, size)

		if pos >= size:
			raise EOFError()
		cluster_index = pos & self.cluster_mask
		l2_index = self.offset_to_l2(pos)
		l1_index = self.offset_to_l1(pos)
		print "l1_index:%ld, l2_index:%ld" % (l1_index, l2_index)

		l1_table = self.load_L1(l1_table_offset, l1_size)
		l2_table_offset = l1_table[l1_index]
		if not l2_table_offset:
		    print "No l2 table offset"
		    return None # '\0' * min(self.cluster_size * self.l2_size, size - pos)
		if l2_table_offset & (1L << 62):
			raise NotImplemented('Qcow2 compression')
		l2_table_offset &= ~(3L << 62)

		l2_table = self.load_L2(l2_table_offset)
		cluster_offset = l2_table[l2_index]
		if not cluster_offset:
		    print "No Cluster offset"
		    return None # '\0' * min(self.cluster_size, size - pos)
		if cluster_offset & (1L << 62):
			raise NotImplemented('Qcow2 compression')
		cluster_offset &= ~(3L << 62)
		print "cluster_offset : 0x%x" % cluster_offset

		self.f.seek(cluster_offset)
		d = self.f.read(self.cluster_size)
		print "len of data : %d" % len(d)
		return d[cluster_index]

if __name__ == '__main__':
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
