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

from Configuration import Const
from contextlib import closing
import sqlite3
import os


def create_basevm(hash_value, base_disk_path):
    with closing(_connect_db()) as conn:
        base_disk_path = os.path.abspath(base_disk_path)
        cur = conn.cursor()

        # delete duplicated data
        query = "select sha256_id from base_vm where base_disk_path='%s'" % (base_disk_path)
        cur.execute(query)
        ret_hash = cur.fetchone()
        if ret_hash and (len(ret_hash) > 0):
            query = "delete from base_vm where base_disk_path='%s'" % (base_disk_path)
            cur.execute(query)

        # insert new
        query = "insert into base_vm values ('%s', '%s')" % (hash_value, base_disk_path)
        cur.execute(query)
        conn.commit()


def gethash_basevm(base_disk_path):
    with closing(_connect_db()) as conn:
        base_disk_path = os.path.abspath(base_disk_path)
        cur = conn.cursor()
        query = "SELECT sha256_id from base_vm where base_disk_path == '%s'" % base_disk_path
        cur.execute(query)
        hashvalue = cur.fetchone()
        if hashvalue and (len(hashvalue) == 1):
            return str(hashvalue[0])
        else:
            return None


def list_basevm(log=open("/dev/null", "w+b")):
    with closing(_connect_db()) as conn:
        cur = conn.cursor()
        cur.execute('SELECT sha256_id, base_disk_path from base_vm')
        base_vm_list = cur.fetchall()
        log.write("-"*80)
        log.write("\n")
        log.write("hash_value\t\t\t\t\t\tpath\n")
        log.write("-"*80)
        log.write("\n")
        for base_vm_item in base_vm_list:
            log.write("%s\t%s\n" % (base_vm_item[0], base_vm_item[1]))
        log.write("\n")
        return base_vm_list


def delete_basevm(base_disk_path):
    pass


'''
DB handler
'''
def _connect_db():
    if os.path.exists(Const.CLOUDLET_DB) == False:
        _init_db()
    return sqlite3.connect(Const.CLOUDLET_DB)


def _init_db():
    def _connect():
        return sqlite3.connect(Const.CLOUDLET_DB)

    with closing(_connect()) as db:
        with open(Const.CLOUDLET_DB_SCHEMA) as f:
            db.cursor().executescript(f.read())
        db.commit()

