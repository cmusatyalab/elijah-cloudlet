#!/usr/bin/env python
#
# Elijah: Cloudlet Infrastructure for Mobile Computing
# Copyright (C) 2011-2012 Carnegie Mellon University
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of version 2 of the GNU General Public License as published
# by the Free Software Foundation.  A copy of the GNU General Public License
# should have been distributed along with this program in the file
# LICENSE.GPL.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.
#

import os
import stat
import sys
import threading 
import requests
import redis
from BeautifulSoup import BeautifulSoup
from Queue import Queue, Empty
from CacheFuse import CacheFS
from CacheFuse import CacheFuseError
import dateutil.parser
import time


# URL fetching thread
# python has multitheading issue
FETCHING_THREAD_NUM = 1

class CachingError(Exception):
    pass

class URIItem(object):
    URI             = "uri"
    NAME            = "cache_filename"
    SIZE            = "size"
    MODIFIED_TIME   = "mtime"
    IS_DIR          = "is_dir"
    IS_CACHED       = "is_cached"

    def __init__(self, uri, cache_filename, filesize, modified_time, 
            is_directory, is_cached):
        setattr(self, URIItem.URI, str(uri))
        setattr(self, URIItem.NAME, str(cache_filename))

        date_time = dateutil.parser.parse(modified_time)
        unix_time = time.mktime(date_time.timetuple())
        setattr(self, URIItem.MODIFIED_TIME, long(unix_time))
        setattr(self, URIItem.IS_DIR, bool(is_directory))
        setattr(self, URIItem.IS_CACHED, bool(is_cached))
        if is_directory == True:
            setattr(self, URIItem.SIZE, 4096) # page size
        else:
            setattr(self, URIItem.SIZE, filesize)
        self.child_list = list()

    def get_uri(self):
        return self.__dict__.get(URIItem.URI, None)

    def get_childlist(self):
        return self.child_list

    def append_child(self, child_uriinfo):
        if type(child_uriinfo) != URIItem:
            raise CachingError("Need URIItem object")
        self.child_list.append(child_uriinfo)

    def __getitem__(self, item):
        return self.__dict__[item]

    def __repr__(self):
        return "name: %s, children: %d" % (getattr(self, URIItem.NAME), len(self.child_list))


class _URIParser(threading.Thread):
    CHARSET = "utf-8"

    def __init__(self, visited_set, uri_queue, cache_root=None, 
            fetch_data=False, print_out=None):
        self.visited_set = visited_set
        self.uri_queue = uri_queue
        self.cache_root = cache_root
        self.is_fetch_data = bool(fetch_data)
        self.compiled_list = list()
        self.print_out = print_out
        if self.print_out == None:
            self.print_out = open("/dev/null", "w+b")
        self.stop = threading.Event()

        threading.Thread.__init__(self, target=self.parse)

    def _is_file(self, header):
        content_type = header.get('content-type', None)
        if content_type == None:
            return True
        if content_type.find('html') != -1:
            return False
        return True

    def parse(self):
        is_first_time_access = True
        while(not self.stop.wait(0.0001)):
            try:
                if is_first_time_access:
                    url = self.uri_queue.get(True, 1) # block for 1 second
                    is_first_time_access = False
                else:
                    url = self.uri_queue.get_nowait()
                header_ret = requests.head(url)
                if header_ret.ok == True:
                    header = header_ret.headers
                else:
                    continue
            except Empty:
                break
            except UnicodeDecodeError:
                continue

            parse_ret = requests.utils.urlparse(url)
            url_path = parse_ret.path[1:] # remove "/"
            cache_filepath = os.path.join(self.cache_root, parse_ret.netloc, url_path)
            if cache_filepath.endswith("/"):
                cache_filepath = cache_filepath[0:-1]

            if self._is_file(header) == True:
                if self.is_fetch_data == True:
                    # save to disk
                    dirpath = os.path.dirname(cache_filepath)
                    if os.path.exists(dirpath) == False:
                        os.makedirs(dirpath)
                    r = requests.get(url, stream=True)
                    if r.ok: 
                        diskfile = open(cache_filepath, "w+b")
                        #self.print_out.write("%s --> %s\n" % (url, cache_filepath))
                        diskfile.write(r.content)
                        diskfile.close()
                # save information to compiled list
                self.compiled_list.append(URIItem(url, cache_filepath, \
                        header.get('content-length', None),
                        header.get('last-modified', None),
                        is_directory=False, is_cached=self.is_fetch_data))
            else:
                # save information to compiled list
                modified_time = header.get('last-modified', None)
                if modified_time == None:
                    modified_time = header.get('date', None)
                if self.is_fetch_data == True:
                    if os.path.exists(cache_filepath) == False:
                        os.makedirs(cache_filepath)

                self.compiled_list.append(URIItem(url, cache_filepath, \
                        header.get('content-length', None),
                        modified_time,
                        is_directory=True, is_cached=self.is_fetch_data))

            if self._is_file(header) == True:
                # leaf node
                pass
            else:
                try:
                    r = requests.get(url)
                except UnicodeDecodeError:
                    continue
                for link in BeautifulSoup(r.text).findAll('a'):
                    try:
                        href = link['href']
                    except KeyError:
                        continue
                    if not href.startswith('http://'):
                        if href[0] == '/':
                            href = href[1:]
                        parse_ret = requests.utils.urlparse('%s%s' % (r.url, href))
                        new_uri = "%s://%s%s" % (parse_ret.scheme, parse_ret.netloc, parse_ret.path)
                    else:
                        new_uri = href

                    if new_uri not in self.visited_set:
                        self.visited_set.add(new_uri)
                        self.uri_queue.put(new_uri)

    def terminate(self):
        self.stop.set()


class Util(object):
    @staticmethod
    def is_valid_uri(uri, is_source_uri=False):
        parse_ret = requests.utils.urlparse(uri)
        if len(parse_ret.scheme) == 0:
            return False
        if len(parse_ret.scheme) == 0:
            return False
        if len(parse_ret.netloc) == 0:
            return False
        if not is_source_uri:
            if len(parse_ret.query) > 0:
                return False
        return True

    @staticmethod
    def get_compiled_URIs(cache_root, sourceURI):
        """Return list of _UIRInfo
        """
        if not Util.is_valid_uri(sourceURI, is_source_uri=True):
            msg = "Invalid URI: %s" % sourceURI
            raise CachingError(msg)
        uri_queue = Queue()
        visited = set()
        uri_queue.put(sourceURI)
        visited.add(sourceURI)
        thread_list = []
        for index in xrange(FETCHING_THREAD_NUM):
            parser = _URIParser(visited, uri_queue, \
                    cache_root=cache_root, fetch_data=False)
            thread_list.append(parser)
            parser.start()

        compiled_list = list()
        try:
            while len(thread_list) > 0:
                t = thread_list[0]
                t.join(timeout=1.0)
                if not t.is_alive():
                    compiled_list.extend(t.compiled_list) 
                    thread_list.remove(t)
        except KeyboardInterrupt, e:
            for t in thread_list:
                t.terminate()
                t.join()
            sys.stderr.write("Keyboard Interrupt")
        
        Util.organize_compiled_list(compiled_list)
        return compiled_list

    @staticmethod
    def organize_compiled_list(compiled_list):
        """Construct file-system like tree structure 
        """
        directory_dict = dict()
        for uri_info in compiled_list:
            # contruct hash table of (key: pathname, value: uri)
            if getattr(uri_info, URIItem.IS_DIR) == True:
                if directory_dict.get(getattr(uri_info, URIItem.NAME), None) != None:
                    msg = "Do not expect duplicated elemenet in compiled list"
                    raise CachingError(msg)
                directory_dict[getattr(uri_info, URIItem.NAME)] = uri_info

        for uri_info in compiled_list:
            pathname = getattr(uri_info, URIItem.NAME)
            parentpath = os.path.dirname(pathname)
            if parentpath.endswith('/'):
                parentpath = parentpath[0:-1]
            parent_uri_info = directory_dict.get(parentpath, None)
            if parent_uri_info != None:
                parent_uri_info.append_child(uri_info)

    @staticmethod
    def get_cache_score(compiledURI_list):
        cache_score = 0
        return cache_score


# TODO: change thread to process
class CacheManager(threading.Thread):
    POST_FIX_ATTRIBUTE  = u'\u03b1'
    POST_FIX_LIST_DIR   = u'\u03b2'

    DEFAULT_GID = 1000 #65534 : nogroup
    DEFAULT_UID = 1000 #65534 : nobody

    def __init__(self, cache_dir, redis_addr, print_out=None):
        self.cache_dir = cache_dir
        self.print_out = print_out
        if self.print_out == None:
            self.print_out = open("/dev/null", "w+b")
        self.redis = self._init_redis(redis_addr)
        self.fuse_queue_list = list()
        self.stop = threading.Event()
        threading.Thread.__init__(self, target=self.monitor_fuse_request)

    def monitor_fuse_request(self):
        while(not self.stop.wait(0.0001)):
            for (fuse, queue) in self.fuse_queue_list:
                relpath = None
                try:
                    relpath = queue.get_nowait()
                except Empty:
                    pass
                if relpath == None:
                    continue;
                
                # fetch data
                cache_filepath = os.path.join(self.cache_dir, relpath)
                fetch_uri = "http://%s" % relpath
                ret = requests.get(fetch_uri, stream=True)
                if ret.ok: 
                    dirpath = os.path.dirname(cache_filepath)
                    if os.path.exists(dirpath) == False:
                        os.makedirs(dirpath)
                    diskfile = open(cache_filepath, "w+b")
                    diskfile.write(ret.content)
                    diskfile.close()
                else:
                    raise CachingError("Cannot cache from %s to %s" % \
                            fetch_uri, cache_filepath)

                # update redis
                relpath = os.path.relpath(cache_filepath, self.cache_dir)
                key = unicode(relpath, "utf-8") + CacheManager.POST_FIX_ATTRIBUTE
                value = self._get_file_attribute(cache_filepath)
                self.redis.set(key, unicode(value))

                # update fuse
                fuse.fuse_write("fetch:%s" % relpath)

    def _init_redis(self, redis_addr):
        """Initialize redis connection and 
        Upload current cache status
        """
        try:
            conn = redis.StrictRedis(host=str(redis_addr[0]), port=int(redis_addr[1]), db=0)
            conn.flushall()
        except redis.exceptions.ConnectionError, e:
            raise CachingError("Failed to connect to Redis")
        self.redis = conn

        for (root, dirs, files) in os.walk(self.cache_dir):
            relpath_cache_root = os.path.relpath(root, self.cache_dir)
            for each_file in files:
                abspath = os.path.join(root, each_file)
                relpath = os.path.relpath(abspath, self.cache_dir)
                # set attribute
                key = unicode(relpath, "utf-8") + CacheManager.POST_FIX_ATTRIBUTE
                value = self._get_file_attribute(abspath)
                self.redis.set(key, unicode(value))
                # set file list
                key = unicode(relpath_cache_root, "utf-8") + CacheManager.POST_FIX_LIST_DIR
                self.redis.rpush(key, unicode(each_file))
                #print "file : " + key + " --> " + each_file
                
            for each_dir in dirs:
                abspath = os.path.join(root, each_dir)
                relpath = os.path.relpath(abspath, self.cache_dir) 
                # set attribute
                key = unicode(relpath, "utf-8") + CacheManager.POST_FIX_ATTRIBUTE
                value = self._get_file_attribute(abspath)
                self.redis.set(key, unicode(value))
                # set file list
                key = unicode(relpath_cache_root, "utf-8") + CacheManager.POST_FIX_LIST_DIR
                self.redis.rpush(key, unicode(each_dir))
                #print "dir : " + key + " --> " + each_dir
        return conn

    def fetch_source_URI(self, sourceURI):
        if not Util.is_valid_uri(sourceURI, is_source_uri=True):
            raise CachingError("Invalid URI: %s" % sourceURI)
        visited = set()
        uri_queue = Queue()
        uri_queue.put(sourceURI)
        thread_list = []
        for index in xrange(FETCHING_THREAD_NUM):
            parser = _URIParser(visited, uri_queue, cache_root=self.cache_dir, 
                    fetch_data=True, print_out=self.print_out)
            thread_list.append(parser)
            parser.start()

        compiled_list = list()
        for t in thread_list:
            t.join()
            compiled_list.extend(t.compiled_list) 

        # update to REDIS since it saved to cache
        self.update_cachedfile_info(compiled_list)
        Util.organize_compiled_list(compiled_list)
        return compiled_list

    def update_cachedfile_info(self, compiled_list):
        for each_item in  compield_list:
            if type(each_item) != URIItem:
                raise CachingError("Expect URIItem")
            abspath = os.path.join(root, getattr(each_item, URIItem.NAME))
            relpath = os.path.relpath(abspath, self.cache_dir)
            # set attribute
            key = unicode(relpath, "utf-8") + CacheManager.POST_FIX_ATTRIBUTE
            value = self._get_file_attribute(abspath)
            self.redis.set(key, unicode(value))

    def fetch_compiled_URIs(self, URIItem_list):
        """ Fetch URIs and save is as cache
        Exception:
            CachingError if failed to fetching URI
        """
        if URIItem_list == None or len(URIItem_list) == 0:
            raise CachingError("No element in URI list")

        for each_info in URIItem_list:
            compiled_uri = getattr(each_info, URIItem.URI)
            if not Util.is_valid_uri(compiled_uri):
                raise CachingError("Invalid URI: %s" % compiled_uri)

        for each_info in URIItem_list:
            uri = getattr(each_info, URIItem.URI)
            parse_ret = requests.utils.urlparse(uri)
            fetch_root = os.path.join(self.cache_dir, parse_ret.netloc, ".")
            uri_path = parse_ret.path[1:] # remove "/" from path
            diskpath = os.path.join(fetch_root, uri_path)
            # save to disk
            if diskpath.endswith('/') == False and os.path.isdir(diskpath) == False:
                dirpath = os.path.dirname(diskpath)
                if os.path.exists(dirpath) == False:
                    import pdb;pdb.set_trace()
                    os.makedirs(dirpath)
                r = requests.get(uri, stream=True)
                if r.ok: 
                    diskfile = open(diskpath, "w+b")
                    #self.print_out.write("%s --> %s\n" % (uri, diskpath))
                    diskfile.write(r.content)
                    diskfile.close()
            else: # directory
                if os.path.exists(diskpath) == False:
                    import pdb;pdb.set_trace()
                    os.makedirs(diskpath)
        return fetch_root

    def launch_fuse(self, URIItem_list):
        """ Construct FUSE directory structure at give Samba directory
        Return:
            fuse object
        Exception:
            CachingError if element of input list is not URIItem
        """

        if len(URIItem_list) == 0 or (type(URIItem_list[0]) != URIItem):
            raise CachingError("Expect list of URIItem")

        for uri_info in URIItem_list:
            cache_filepath = getattr(uri_info, URIItem.NAME)
            abspath = os.path.abspath(cache_filepath)
            relpath = os.path.relpath(abspath, self.cache_dir)
            redis_ret = self.redis.get(unicode(relpath) + CacheManager.POST_FIX_ATTRIBUTE)
            if redis_ret != None:
                # cached
                # TODO: check expiration of the cache
                pass
            else:
                # not cached
                # update attributes
                key = unicode(relpath) + CacheManager.POST_FIX_ATTRIBUTE
                value = self._get_default_attribute(uri_info)
                self.redis.set(key, unicode(value))
                # update directory information
                child_list = uri_info.get_childlist()
                for child_file in child_list:
                    key = unicode(relpath) + CacheManager.POST_FIX_LIST_DIR
                    value = os.path.basename(getattr(child_file, URIItem.NAME))
                    #print "update : " + key + " --> " + value
                    self.redis.rpush(key, unicode(value))

        # launch FUSE
        parse_ret = requests.utils.urlparse(getattr(URIItem_list[0], URIItem.URI))
        url_root = parse_ret.netloc
        if url_root.endswith("/") == True:
            url_root = url_root[0:-1]
        args = "%s %s %s %s" % \
                (str(self.cache_dir), str(url_root), str(REDIS_ADDR[0]), str(REDIS_ADDR[1]))
        request_queue = Queue()
        fuse = CacheFS(CACHE_FUSE_BINPATH, args, request_queue, print_out=self.print_out)
        try:
            fuse.launch()
            fuse.start()
            self.add_fuse_request_queue(fuse, request_queue)
            return fuse
        except CacheFuseError, e:
            raise CacheFuseError(str(e))

    def add_fuse_request_queue(self, fuse, request_queue):
        self.fuse_queue_list.append((fuse, request_queue))
        pass

    def _get_file_attribute(self, filepath):
        st = os.lstat(filepath)
        value = "exists:1,atime:%ld,ctime:%ld,mtime:%ld,gid:%ld,uid:%ld,mode:%ld,size:%ld,nlink:%ld" % \
                ( getattr(st, 'st_atime'), getattr(st, 'st_ctime'),\
                getattr(st, 'st_mtime'), getattr(st, 'st_gid'),\
                getattr(st, 'st_uid'), getattr(st, 'st_mode'),\
                getattr(st, 'st_size'), getattr(st, 'st_nlink'))
        return value

    def _get_default_attribute(self, uri_info):
        mtime = getattr(uri_info, URIItem.MODIFIED_TIME)
        atime = ctime = mtime
        gid = CacheManager.DEFAULT_GID
        uid = CacheManager.DEFAULT_UID
        mode = stat.S_IRUSR | stat.S_IWUSR | stat.S_IRGRP | stat.S_IWGRP | stat.S_IROTH
        if getattr(uri_info, URIItem.IS_DIR) == True:
            mode |= stat.S_IFDIR
        else:
            mode |= stat.S_IFREG
        size = getattr(uri_info, URIItem.SIZE)
        nlink = len(uri_info.get_childlist())

        value = "exists:0,atime:%ld,ctime:%ld,mtime:%ld,gid:%ld,uid:%ld,mode:%ld,size:%ld,nlink:%ld" % \
                (atime, ctime, mtime, long(gid), long(uid), 
                        long(mode), long(size), long(nlink))
        return value

    def terminate(self):
        self.print_out.write("[INFO] CacheManager terminate\n")
        self.stop.set()

# Global
REDIS_ADDR = ('localhost', 6379)
CACHE_FUSE_BINPATH = "./fuse/cachefs"
CACHE_ROOT = '/tmp/cloudlet_cache/'

if __name__ == '__main__':
    if len(sys.argv) != 2:
        print "> $ prog [root_uri]"
        sys.exit(1)

    try:
        cache_manager = CacheManager(CACHE_ROOT, REDIS_ADDR, print_out=sys.stdout)
        cache_manager.start()
    except CachingError, e:
        sys.stderr.write(str(e) + "\n")
        sys.exit(1)

    compiled_list = Util.get_compiled_URIs(cache_manager.cache_dir, sys.argv[1])
    try:
        #cache_manager.fetch_compiled_URIs(compiled_list)
        cache_fuse = cache_manager.launch_fuse(compiled_list)
        print "mount : %s" % (cache_fuse.mountpoint)
        while True:
            time.sleep(100)

    except CachingError, e:
        print str(e)
    except KeyboardInterrupt,e :
        print "user exit"
    finally:
        if cache_fuse:
            cache_fuse.terminate()
            cache_fuse.join()
        if cache_manager:
            cache_manager.terminate()
            cache_manager.join()
