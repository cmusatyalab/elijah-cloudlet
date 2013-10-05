#
# vmnetx.package - Handling of .nxpk files
#
# Copyright (C) 2012-2013 Carnegie Mellon University
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of version 2 of the GNU General Public License as published
# by the Free Software Foundation.  A copy of the GNU General Public License
# should have been distributed along with this program in the file
# COPYING.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.
#

from cookielib import Cookie
from datetime import datetime
import dateutil.parser
from dateutil.tz import tzutc
import os
import re
import requests
import struct
import shutil
from lxml import etree
from tempfile import mkdtemp
from urlparse import urlsplit
import zipfile
from lxml.builder import ElementMaker
import sys
import subprocess

from cloudlet.Configuration import Const
from cloudlet import log as logging
from cloudlet.db.api import DBConnector
from cloudlet.db.table_def import BaseVM

LOG = logging.getLogger(__name__)


# We want this to be a public attribute
# pylint: disable=C0103
# pylint: enable=C0103
class DetailException(Exception):
    def __init__(self, msg, detail=None):
        Exception.__init__(self, msg)
        if detail:
            self.detail = detail


class BadPackageError(DetailException):
    pass


class NeedAuthentication(Exception):
    def __init__(self, host, realm, scheme):
        Exception.__init__(self, 'Authentication required')
        self.host = host
        self.realm = realm
        self.scheme = scheme


class _HttpError(Exception):
    '''_HttpFile would like to raise IOError on errors, but ZipFile swallows
    the error message.  So it raises this instead.'''
    pass


class _HttpFile(object):
    '''A read-only file-like object backed by HTTP Range requests.'''

    # pylint doesn't understand named tuples
    # pylint: disable=E1103
    def __init__(self, url, scheme=None, username=None, password=None,
            buffer_size=64 << 10):
        if scheme == 'Basic':
            self._auth = (username, password)
        elif scheme == 'Digest':
            self._auth = requests.auth.HTTPDigestAuth(username, password)
        elif scheme is None:
            self._auth = None
        else:
            raise ValueError('Unknown authentication scheme')

        self.url = url
        self._offset = 0
        self._closed = False
        self._buffer = ''
        self._buffer_offset = 0
        self._buffer_size = buffer_size
        self._session = requests.Session()
        if hasattr(requests.utils, 'default_user_agent'):
            self._session.headers['User-Agent'] = 'cloudlet/%s %s' % (
                    Const.VERSION, requests.utils.default_user_agent())
        else:
            # requests < 0.13.3
            self._session.headers['User-Agent'] = \
                    'vmnetx/%s python-requests/%s' % (
                    Const.VERSION, requests.__version__)

        # Debugging
        self._last_case = None
        self._last_network = None

        # Perform HEAD request
        try:
            resp = self._session.head(self.url, auth=self._auth)

            # Check for missing credentials
            if resp.status_code == 401:
                # Assumes a single challenge.
                scheme, parameters = resp.headers['WWW-Authenticate'].split(
                        None, 1)
                if scheme != 'Basic' and scheme != 'Digest':
                    raise _HttpError('Server requested unknown ' +
                            'authentication scheme: %s' % scheme)
                host = urlsplit(self.url).netloc
                for param in parameters.split(', '):
                    match = re.match('^realm=\"([^"]*)\"$', param)
                    if match:
                        raise NeedAuthentication(host, match.group(1), scheme)
                raise _HttpError('Unknown authentication realm')

            # Check for other errors
            resp.raise_for_status()
            # 2xx codes other than 200 are unexpected
            if resp.status_code != 200:
                raise _HttpError('Unexpected status code %d' %
                        resp.status_code)

            # Store object length
            try:
                self.length = int(resp.headers['Content-Length'])
            except (IndexError, ValueError):
                raise _HttpError('Server did not provide Content-Length')

            # Store validators
            self.etag = self._get_etag(resp)
            self.last_modified = self._get_last_modified(resp)

            # Record cookies
            if hasattr(self._session.cookies, 'extract_cookies'):
                # CookieJar
                self.cookies = tuple(c for c in self._session.cookies)
            else:
                # dict (requests < 0.12.0)
                parsed = urlsplit(self.url)
                self.cookies = tuple(Cookie(version=0,
                        name=name, value='"%s"' % value,
                        port=None, port_specified=False,
                        domain=parsed.netloc, domain_specified=False,
                        domain_initial_dot=False,
                        path=parsed.path, path_specified=True,
                        secure=False, expires=None, discard=True,
                        comment=None, comment_url=None, rest={})
                        for name, value in self._session.cookies.iteritems())
        except requests.exceptions.RequestException, e:
            raise _HttpError(str(e))
    # pylint: enable=E1103

    def __enter__(self):
        return self

    def __exit__(self, _type, _value, _traceback):
        self.close()

    @property
    def name(self):
        return '<%s>' % self.url

    def _get_etag(self, resp):
        etag = resp.headers.get('ETag')
        if etag is None or etag.startswith('W/'):
            return None
        return etag

    def _get_last_modified(self, resp):
        last_modified = resp.headers.get('Last-Modified')
        if last_modified is None:
            return None
        try:
            return dateutil.parser.parse(last_modified)
        except ValueError:
            return None

    def _get(self, offset, size):
        range = '%d-%d' % (offset, offset + size - 1)
        self._last_network = range
        range = 'bytes=' + range

        try:
            resp = self._session.get(self.url, auth=self._auth, headers={
                'Range': range,
            })
            resp.raise_for_status()
            if resp.status_code != 206:
                raise _HttpError('Server ignored range request')
            if (self._get_etag(resp) != self.etag or
                    self._get_last_modified(resp) != self.last_modified):
                raise _HttpError('Resource changed on server')
            return resp.content
        except requests.exceptions.RequestException, e:
            raise _HttpError(str(e))

    def read(self, size=None):
        if self.closed:
            raise _HttpError('File is closed')
        if size is None:
            size = self.length - self._offset
        buf_start = self._buffer_offset
        buf_end = self._buffer_offset + len(self._buffer)
        if self._offset >= buf_start and self._offset + size <= buf_end:
            # Case B: Satisfy entirely from buffer
            self._last_case = 'B'
            start = self._offset - self._buffer_offset
            ret = self._buffer[start:start + size]
        elif self._offset >= buf_start and self._offset < buf_end:
            # Case C: Satisfy head from buffer
            # Buffer becomes _buffer_size bytes after requested region
            self._last_case = 'C'
            ret = self._buffer[self._offset - buf_start:]
            remaining = size - len(ret)
            data = self._get(self._offset + len(ret), remaining +
                    self._buffer_size)
            ret += data[:remaining]
            self._buffer = data[remaining:]
            self._buffer_offset = self._offset + size
        elif (self._offset < buf_start and
                self._offset + size >= buf_start):
            # Case D: Satisfy tail from buffer
            # Buffer becomes _buffer_size bytes before requested region
            # plus requested region
            self._last_case = 'D'
            tail = self._buffer[:self._offset + size - buf_start]
            start = max(self._offset - self._buffer_size, 0)
            data = self._get(start, buf_start - start)
            self._buffer = data + tail
            self._buffer_offset = start
            ret = self._buffer[self._offset - start:]
        else:
            # Buffer is useless
            if self._offset + size >= self.length:
                # Case E: Reading at the end of the file.
                # Assume zipfile is probing for the central directory.
                # Buffer becomes _buffer_size bytes before requested
                # region plus requested region
                self._last_case = 'E'
                start = max(self._offset - self._buffer_size, 0)
                self._buffer = self._get(start,
                        self._offset + size - start)
                self._buffer_offset = start
                ret = self._buffer[self._offset - start:]
            else:
                # Case F: Read unrelated to previous reads.
                # Buffer becomes _buffer_size bytes after requested region
                self._last_case = 'F'
                data = self._get(self._offset, size + self._buffer_size)
                ret = data[:size]
                self._buffer = data[size:]
                self._buffer_offset = self._offset + size
        self._offset += len(ret)
        return ret

    def seek(self, offset, whence=0):
        if self.closed:
            raise _HttpError('File is closed')
        if whence == 0:
            self._offset = offset
        elif whence == 1:
            self._offset += offset
        elif whence == 2:
            self._offset = self.length + offset
        self._offset = max(self._offset, 0)

    def tell(self):
        if self.closed:
            raise _HttpError('File is closed')
        return self._offset

    def close(self):
        self._closed = True
        self._buffer = ''
        self._session.close()

    @property
    def closed(self):
        return self._closed


class _FileFile(file):
    '''An _HttpFile-compatible file-like object for local files.'''

    # pylint doesn't understand named tuples
    # pylint: disable=E1103
    def __init__(self, url):
        # Process URL
        parsed = urlsplit(url)
        if parsed.scheme != 'file':
            raise ValueError('Invalid URL scheme')
        self.url = url
        self.cookies = ()

        file.__init__(self, parsed.path)

        # Set length
        self.seek(0, 2)
        self.length = self.tell()
        self.seek(0)

        # Set validators.  We could synthesize an ETag from st_dev and
        # st_ino, but this would confuse vmnetfs since libcurl doesn't do
        # the same.
        self.etag = None
        self.last_modified = datetime.fromtimestamp(
                int(os.fstat(self.fileno()).st_mtime), tzutc())
    # pylint: enable=E1103


class _PackageObject(object):
    def __init__(self, zip, path, load_data=False):
        self._fh = zip.fp
        self.url = self._fh.url
        self.etag = self._fh.etag
        self.last_modified = self._fh.last_modified
        self.cookies = self._fh.cookies

        # Calculate file offset and length
        try:
            info = zip.getinfo(path)
        except KeyError:
            raise BadPackageError('Path "%s" missing from package' % path)
        # ZipInfo.extra is the extra field from the central directory file
        # header, which may be different from the extra field in the local
        # file header.  So we need to read the local file header to determine
        # its size.
        header_fmt = '<4s5H3I2H'
        header_len = struct.calcsize(header_fmt)
        self._fh.seek(info.header_offset)
        magic, _, flags, compression, _, _, _, _, _, name_len, extra_len = \
                struct.unpack(header_fmt, self._fh.read(header_len))
        if magic != zipfile.stringFileHeader:
            raise BadPackageError('Member "%s" has invalid header' % path)
        if compression != zipfile.ZIP_STORED:
            raise BadPackageError('Member "%s" is compressed' % path)
        if flags & 0x1:
            raise BadPackageError('Member "%s" is encrypted' % path)
        self.offset = info.header_offset + header_len + name_len + extra_len
        self.size = info.file_size

        if load_data:
            # Eagerly read file data into memory, since _HttpFile likely has
            # it in cache.
            self._fh.seek(self.offset)
            self.data = self._fh.read(self.size)
        else:
            self.data = None

    def write_to_file(self, fh, buf_size=1 << 20):
        if self.data is not None:
            fh.write(self.data)
        else:
            self._fh.seek(self.offset)
            count = self.size
            while count > 0:
                cur = min(count, buf_size)
                buf = self._fh.read(cur)
                fh.write(buf)
                count -= len(buf)


class VMOverlayPackage(object):
    # pylint doesn't understand named tuples
    # pylint: disable=E1103
    def __init__(self, url, scheme=None, username=None, password=None):
        self.url = url

        # Open URL
        parsed = urlsplit(url)
        if parsed.scheme == 'http' or parsed.scheme == 'https':
            fh = _HttpFile(url, scheme=scheme, username=username,
                    password=password)
        elif parsed.scheme == 'file':
            fh = _FileFile(url)
        else:
            raise ValueError('%s: URLs not supported' % parsed.scheme)

        # Read Zip
        try:
            self.zip_overlay = zipfile.ZipFile(fh, 'r')

            if Const.OVERLAY_META not in self.zip_overlay.namelist():
                msg = "Does not have meta file named %s" % Const.OVERLAY_META
                raise DetailException(msg)
            
            self.metafile = Const.OVERLAY_META
            self.blobfiles = list()
            for each_file in self.zip_overlay.namelist():
                if (each_file != Const.OVERLAY_META):
                    self.blobfiles.append(each_file)
            
        except (zipfile.BadZipfile, _HttpError), e:
            raise BadPackageError(str(e))
    # pylint: enable=E1103

    def read_meta(self):
        self.metadata = self.zip_overlay.read(self.metafile)
        return self.metadata

    def read_blob(self, blobname):
        self.blobdata = self.zip_overlay.read(blobname)
        return self.blobdata

    @classmethod
    def create(cls, outfilename, metafile, blobfiles):
        # Write package
        zip = zipfile.ZipFile(outfilename, 'w', zipfile.ZIP_STORED, True)
        zip.comment = 'Cloudlet VM overlay'
        zip.write(metafile, os.path.basename(metafile))
        for index, blobfile in enumerate(blobfiles):
            zip.write(blobfile, os.path.basename(blobfile))
        zip.close()


# vmnetx specific package
# create xml file for demand fetching

#from lxml.builder import ElementMaker
#from lxml import etree
#MANIFEST_FILENAME = 'vmnetx-package.xml'
#DOMAIN_FILENAME = 'domain.xml'
#DISK_FILENAME = 'disk.img'
#MEMORY_FILENAME = 'memory.img'
#
#NS = 'http://olivearchive.org/xmlns/vmnetx/package'
#NSP = '{' + NS + '}'
#
#SCHEMA_PATH = os.path.join(os.path.dirname(__file__), 'schema', 'package.xsd')
#schema = etree.XMLSchema(etree.parse(SCHEMA_PATH))
#
#class Package(object):
#    # pylint doesn't understand named tuples
#    # pylint: disable=E1103
#    def __init__(self, url, scheme=None, username=None, password=None):
#        self.url = url
#
#        # Open URL
#        parsed = urlsplit(url)
#        if parsed.scheme == 'http' or parsed.scheme == 'https':
#            fh = _HttpFile(url, scheme=scheme, username=username,
#                    password=password)
#        elif parsed.scheme == 'file':
#            fh = _FileFile(url)
#        else:
#            raise ValueError('%s: URLs not supported' % parsed.scheme)
#
#        # Read Zip
#        try:
#            zip = zipfile.ZipFile(fh, 'r')
#
#            # Parse manifest
#            if MANIFEST_FILENAME not in zip.namelist():
#                raise BadPackageError('Package does not contain manifest')
#            xml = zip.read(MANIFEST_FILENAME)
#            tree = etree.fromstring(xml, etree.XMLParser(schema=schema))
#
#            # Create attributes
#            self.name = tree.get('name')
#            self.domain = _PackageObject(zip,
#                    tree.find(NSP + 'domain').get('path'), True)
#            self.disk = _PackageObject(zip,
#                    tree.find(NSP + 'disk').get('path'))
#            memory = tree.find(NSP + 'memory')
#            if memory is not None:
#                self.memory = _PackageObject(zip, memory.get('path'))
#            else:
#                self.memory = None
#        except etree.XMLSyntaxError, e:
#            raise BadPackageError('Manifest XML does not validate', str(e))
#        except (zipfile.BadZipfile, _HttpError), e:
#            raise BadPackageError(str(e))
#    # pylint: enable=E1103
#
#    @classmethod
#    def create(cls, out, name, domain_xml, disk_path, memory_path=None):
#        # Generate manifest XML
#        e = ElementMaker(namespace=NS, nsmap={None: NS})
#        tree = e.image(
#            e.domain(path=DOMAIN_FILENAME),
#            e.disk(path=DISK_FILENAME),
#            name=name,
#        )
#        if memory_path:
#            tree.append(e.memory(path=MEMORY_FILENAME))
#        schema.assertValid(tree)
#        xml = etree.tostring(tree, encoding='UTF-8', pretty_print=True,
#                xml_declaration=True)
#
#        # Write package
#        zip = zipfile.ZipFile(out, 'w', zipfile.ZIP_STORED, True)
#        zip.comment = 'VMNetX package'
#        zip.writestr(MANIFEST_FILENAME, xml)
#        zip.writestr(DOMAIN_FILENAME, domain_xml)
#        if memory_path is not None:
#            zip.write(memory_path, MEMORY_FILENAME)
#        zip.write(disk_path, DISK_FILENAME)
#        zip.close()
#

class BaseVMPackage(object):
    NS = 'http://opencloudlet.org/xmlns/vmsynthesis/package'
    NSP = '{' + NS + '}'
    SCHEMA_PATH = Const.BASEVM_PACKAGE_SCHEMA
    schema = etree.XMLSchema(etree.parse(SCHEMA_PATH))

    MANIFEST_FILENAME = 'basevm-package.xml'
    #DISK_FILENAME = 'disk.img'
    #MEMORY_FILENAME = 'memory.img'
    #DISK_HASH_FILENAME = 'disk-hash.img'
    #MEMORY_HASH_FILENAME = 'memory-hash.img'

    # pylint doesn't understand named tuples
    # pylint: disable=E1103
    def __init__(self, url, scheme=None, username=None, password=None):
        self.url = url

        # Open URL
        parsed = urlsplit(url)
        if parsed.scheme == 'http' or parsed.scheme == 'https':
            fh = _HttpFile(url, scheme=scheme, username=username,
                    password=password)
        elif parsed.scheme == 'file':
            fh = _FileFile(url)
        else:
            raise ValueError('%s: URLs not supported' % parsed.scheme)

        # Read Zip
        try:
            zip = zipfile.ZipFile(fh, 'r')

            # Parse manifest
            if self.MANIFEST_FILENAME not in zip.namelist():
                raise BadPackageError('Package does not contain manifest')
            xml = zip.read(self.MANIFEST_FILENAME)
            tree = etree.fromstring(xml, etree.XMLParser(schema=self.schema))

            # Create attributes
            self.base_hashvalue = tree.get('hash_value')
            self.disk = _PackageObject(zip,
                    tree.find(self.NSP + 'disk').get('path'))
            self.memory = _PackageObject(zip,
                    tree.find(self.NSP + 'memory').get('path'))
            self.disk_hash = _PackageObject(zip,
                    tree.find(self.NSP + 'disk_hash').get('path'))
            self.memory_hash = _PackageObject(zip,
                    tree.find(self.NSP + 'memory_hash').get('path'))
        except etree.XMLSyntaxError, e:
            raise BadPackageError('Manifest XML does not validate', str(e))
        except (zipfile.BadZipfile, _HttpError), e:
            raise BadPackageError(str(e))
    # pylint: enable=E1103

    def read_meta(self):
        self.metadata = self.zip_overlay.read(self.metafile)
        return self.metadata

    @classmethod
    def create(cls, outfile, basevm_hashvalue,
            base_disk, base_memory, disk_hash, memory_hash):
        # Generate manifest XML
        e = ElementMaker(namespace=cls.NS, nsmap={None: cls.NS})
        tree = e.image(
            e.disk(path=os.path.basename(base_disk)),
            e.memory(path=os.path.basename(base_memory)),
            e.disk_hash(path=os.path.basename(disk_hash)),
            e.memory_hash(path=os.path.basename(memory_hash)),
            hash_value=str(basevm_hashvalue),
        )
        cls.schema.assertValid(tree)
        xml = etree.tostring(tree, encoding='UTF-8', pretty_print=True,
                xml_declaration=True)
        zip = zipfile.ZipFile(outfile, 'w', zipfile.ZIP_DEFLATED, True)
        zip.comment = 'Cloudlet package for base VM'
        zip.writestr(cls.MANIFEST_FILENAME, xml)
        zip.close()

        # zip library bug at python 2.7.3
        # see more at http://bugs.python.org/issue9720

        #filelist = [base_disk, base_memory, disk_hash, memory_hash]
        #for filepath in filelist:
        #    basename = os.path.basename(filepath)
        #    filesize = os.path.getsize(filepath)
        #    LOG.info("Zipping %s (%ld bytes) into %s" % (basename, filesize, outfile))
        #    zip.write(filepath, basename)
        #zip.close()

        cmd = ['zip', '-j', '-9']
        cmd += ["%s" % outfile]
        cmd += [base_disk, base_memory, disk_hash, memory_hash]
        proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        LOG.info("Start compressing")
        LOG.info("%s" % ' '.join(cmd))
        for line in iter(proc.stdout.readline, ''):
            line = line.replace('\r', '')
            sys.stdout.write(line)
            sys.stdout.flush()


class PackagingUtil(object):
    @staticmethod
    def _get_matching_basevm(basedisk_path):
        dbconn = DBConnector()
        basedisk_path = os.path.abspath(basedisk_path)
        basevm_list = dbconn.list_item(BaseVM)
        ret_basevm = None
        for item in basevm_list:
            if basedisk_path == item.disk_path: 
                ret_basevm = item
                break
        return ret_basevm

    @staticmethod
    def export_basevm(name, basevm_path, basevm_hashvalue):
        (base_diskmeta, base_mempath, base_memmeta) = \
                Const.get_basepath(basevm_path)
        output_path = os.path.join(os.curdir, name)
        if output_path.endswith(".zip") == False:
            output_path += ".zip"
        if os.path.exists(output_path) == True:
            is_overwrite = raw_input("%s exists. Overwirte it? (y/N) " % output_path)
            if is_overwrite != 'y':
                return None

        BaseVMPackage.create(output_path, basevm_hashvalue, basevm_path, base_mempath, base_diskmeta, base_memmeta)
        #BaseVMPackage.create(output_path, name, base_diskmeta, base_memmeta, base_diskmeta, base_memmeta)
        return output_path

    @staticmethod
    def import_basevm(filename):
        # Parse manifest
        zip = zipfile.ZipFile(_FileFile("file:///%s" % filename), 'r')
        if BaseVMPackage.MANIFEST_FILENAME not in zip.namelist():
            raise BadPackageError('Package does not contain manifest')
        xml = zip.read(BaseVMPackage.MANIFEST_FILENAME)
        tree = etree.fromstring(xml, etree.XMLParser(schema=BaseVMPackage.schema))

        # Create attributes
        base_hashvalue = tree.get('hash_value')
        disk_name = tree.find(BaseVMPackage.NSP + 'disk').get('path')
        memory_name = tree.find(BaseVMPackage.NSP + 'memory').get('path')
        diskhash_name = tree.find(BaseVMPackage.NSP + 'disk_hash').get('path')
        memoryhash_name = tree.find(BaseVMPackage.NSP + 'memory_hash').get('path')

        # check directory
        base_vm_dir = os.path.join(os.path.dirname(Const.BASE_VM_DIR), base_hashvalue)
        temp_dir = mkdtemp(prefix="cloudlet-base-")
        disk_tmp_path = os.path.join(temp_dir, disk_name)
        disk_target_path = os.path.join(base_vm_dir, disk_name)
        matching_basevm = PackagingUtil._get_matching_basevm(disk_target_path)
        if matching_basevm != None:
            LOG.info("Base VM is already exists")
            LOG.info("Delete existing Base VM using command")
            LOG.info("See more 'cloudlet --help'")
            return None
        if not os.path.exists(base_vm_dir):
            LOG.info("create directory for base VM")
            os.makedirs(base_vm_dir)


        # decompress
        LOG.info("Decompressing Base VM to temp directory at %s" % temp_dir)
        zip.extractall(temp_dir)
        shutil.move(disk_tmp_path, disk_target_path)
        (target_diskhash, target_memory, target_memoryhash) = \
                Const.get_basepath(disk_target_path, check_exist=False)
        path_list = {
                os.path.join(temp_dir, memory_name): target_memory,
                os.path.join(temp_dir, diskhash_name): target_diskhash,
                os.path.join(temp_dir, memoryhash_name): target_memoryhash,
                }

        LOG.info("Place base VM to the right directory")
        for (src, dest) in path_list.iteritems():
            shutil.move(src, dest)

        # add to DB
        dbconn = DBConnector()
        LOG.info("Register New Base to DB")
        LOG.info("ID for the new Base VM: %s" % base_hashvalue)
        new_basevm = BaseVM(disk_target_path, base_hashvalue)
        LOG.info("Success")
        dbconn.add_item(new_basevm)
        return disk_target_path, base_hashvalue

