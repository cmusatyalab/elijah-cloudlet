#!/usr/bin/env python 
#
# Cloudlet Infrastructure for Mobile Computing
#
#   Author: Kiryong Ha <krha@cmu.edu>
#
#   Copyright (C) 2011-2013 Carnegie Mellon University
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
#

"""
DB wrapper for cloudlet registration
"""

import os
import sys
import heapq
from operator import itemgetter

from sqlalchemy import create_engine
#from sqlalchemy.orm import mapper, sessionmaker

DJANGO_PROJECT_PATH = os.path.abspath("../register_server/")
sys.path.append(os.path.join(DJANGO_PROJECT_PATH, "cloudlets"))
from network import ip_location


class DBConnector(object):
    def __init__(self):
        self.cost = ip_location.IPLocation()
        self.db_engine = self._load_engine(os.path.join(DJANGO_PROJECT_PATH, "mysql.conf"))

    def search_nearby_cloudlet(self, client_ip, max_count=10):
        client_location = self.cost.ip2location(client_ip)
        if not client_location:
            return list()

        lat1, lon1 = client_location.latitude, client_location.longitude
        search_field = ["latitude", "longitude", "ip_address"]
        sql_query = "select %s from ds_cloudlet where status = 'RUN'" % (", ".join(search_field))
        ret_list = self.db_engine.execute(sql_query)
        cloudlet_list = list()
        for item in ret_list:
            cloudlet_machine = CloudletMachine(search_field, item)
            lat2, lon2 = float(cloudlet_machine.latitude), float(cloudlet_machine.longitude)
            geo_distance = ip_location.geo_distance(lat1, lon1, lat2, lon2)
            cloudlet_machine.cost = geo_distance
            cloudlet_list.append(cloudlet_machine)

        top_cloudlets = heapq.nlargest(max_count, cloudlet_list, key=itemgetter('cost'))
        return top_cloudlets

    def _load_engine(self, conf_file):
        def _parse_db_file(conf_filename):
            db_name = db_user = db_pass = None
            with open(conf_filename, 'r') as f:
                lines = f.read().split("\n")
                for line in lines:
                    if line.find("=") == -1:
                        continue
                    key, value = line.split("=", 2)
                    key = key.strip(); value = value.strip()
                    if key == 'database':
                        db_name = value
                    elif key == 'user':
                        db_user = value
                    elif key == 'password':
                        db_pass = value
            return db_name, db_user, db_pass

        if os.path.exists(conf_file) == False:
            sys.stderr.write("Cannot find mysql configuration file at %s\n" % \
                    os.path.exists(conf_file))
            sys.stderr.write("Please make a file following README\n")
            sys.exit(1)
        db_name, user, password = _parse_db_file(conf_file)
        engine = create_engine('mysql://%s:%s@localhost/%s' % (user, password, db_name), echo=False)
        #metadata = MetaData(engine)
        #moz_cloudlet = Table('ds_cloudlet', metadata, autoload=True)
        #mapper(Cloudlet, moz_cloudlet) 

        # This session maker make a problem at RHEL 6, which has 0.5.5 version
        #Session = sessionmaker(bind=engine)
        #session = Session()

        return engine


class CloudletMachine(object):
    def __init__(self, keys, values):
        for index, each_key in enumerate(keys):
            self.__dict__[each_key] = values[index]

    def __getitem__(self, key):
        return self.__dict__[key]

    def __repr__(self):
        from pprint import pformat
        return pformat(self.__dict__)

def main(argv):
    db = DBConnector()
    cloudlet_list = db.search_nearby_cloudlet("1.1.1.1")
    for each in cloudlet_list:
        print cloudlet_list

    sys.exit(0)


if __name__ == "__main__":
    status = main(sys.argv[1:])
