# -*- coding: utf-8 -*-

import sys
import os
import requests
from BeautifulSoup import BeautifulSoup
from Queue import Queue, Empty
import threading 

class URIParser(threading.Thread):
    CHARSET = "utf-8"

    def __init__(self, visited_set, uri_queue, cache_dir=None):
        self.visited_set = visited_set
        self.uri_queue = uri_queue
        self.compiled_list = list()
        self.cache_dir = cache_dir
        threading.Thread.__init__(self, target=self.parse)

    def _is_valid_uri(self, uri):
        parse_ret = requests.utils.urlparse(uri)
        if len(parse_ret.query) > 0:
            return False
        return True

    def _is_file(self, header):
        content_type = header.get('content-type', None)
        if content_type == None:
            return True
        if content_type.find('html') != -1:
            return False
        return True

    def parse(self):
        while True:
            try:
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

            if self.cache_dir != None:
                parse_ret = requests.utils.urlparse(url)
                url_path = parse_ret.path[1:] # remove "/"
                diskpath = os.path.join(self.cache_dir, parse_ret.netloc, url_path)
                # save to disk
                if self._is_file(header) == True:
                    dirpath = os.path.dirname(diskpath)
                    if os.path.exists(dirpath) == False:
                        os.makedirs(dirpath)
                    r = requests.get(url, stream=True)
                    if r.ok: 
                        diskfile = open(diskpath, "w+b")
                        print "Saving %s --> %s" % (url, diskpath)
                        while True:
                            raw_data = r.raw.read(1024*1024*5)
                            if raw_data == None or len(raw_data) == 0:
                                break
                            diskfile.write(raw_data)
                        diskfile.close()
                else:
                    if os.path.exists(diskpath) == False:
                        os.makedirs(diskpath)

            self.compiled_list.append(url)
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


def get_compiled_list(host, cache_dir):
    visited = set()
    uri_queue = Queue()
    uri_queue.put(host)

    thread_list = []
    for index in xrange(3):
        parser = URIParser(visited, uri_queue, cache_dir=cache_dir)
        thread_list.append(parser)
        parser.start()

    compiled_list = list()
    for t in thread_list:
        t.join()
        compiled_list.extend(t.compiled_list)

    return compiled_list


def caching_all(uri_list):
    visited = set()
    uri_queue = Queue()
    uri_queue.put(host)

    thread_list = []
    for index in xrange(3):
        parser = URIParser(visited, uri_queue)
        thread_list.append(parser)
        parser.start()

    compiled_list = list()
    for t in thread_list:
        t.join()
        compiled_list.extend(t.compiled_list)

    return compiled_list


def print_usage():
    print "> $ prog [root_uri] [cache_dir]"


if __name__ == '__main__':
    if len(sys.argv) != 3:
        print_usage()
        sys.exit(1)

    host = sys.argv[1]
    cache_dir = sys.argv[2]
    ret_list = get_compiled_list(host, cache_dir)
    import pprint
    pprint.pprint(ret_list)

