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

"""
DB Table definition
"""
from sqlalchemy import create_engine, ForeignKey
from sqlalchemy import Column, DateTime, Integer, String, BigInteger
from sqlalchemy.ext.declarative import declarative_base
from sqlalchemy.orm import relationship, backref
from uuid import uuid1
import random
import sys
    
import datetime


Base = declarative_base()


class BaseVM(Base):
    """
    """
    __tablename__ = "base_vm"

    disk_path = Column(String, primary_key=True)
    hash_value = Column(String)

    def __init__(self, disk_path, hash_value):
        self.disk_path = disk_path
        self.hash_value = hash_value

class OverlayVM(Base):
    """
    """
    __tablename__ = "overlay_vm"

    disk_path = Column(String, primary_key=True)
    memory_path = Column(String, primary_key=True)
    basevm_diskpath = Column(String, ForeignKey("base_vm.disk_path"))


class Session(Base):
    """ """
    __tablename__ = "session"

    STATUS_RUNNING          = 1
    STATUS_CLOSE            = 2     # successfully closed by client
    STATUS_UNEXPECT_CLOSE   = 3     # force closed either from
                                    # 1) error handler
                                    # 2) server termination

    session_id = Column(BigInteger, primary_key=True)
    associated_time = Column(DateTime)
    disassociated_time = Column(DateTime)
    status = Column(Integer)

    def __init__(self):
        self.session_id = long(random.random()*sys.maxint)
        self.associated_time = datetime.datetime.now()
        self.disassociated_time = datetime.datetime.now()
        self.status = Session.STATUS_RUNNING

    def terminate(self, status=STATUS_CLOSE):
        self.disassociated_time = datetime.datetime.now()
        self.status = status

    def _print_status(self, status):
        if status == Session.STATUS_RUNNING:
            return "RUNNING"
        elif status == Session.STATUS_CLOSE:
            return "CLOSED"
        elif status == Session.STATUS_UNEXPECT_CLOSE:
            return "FORCE_CLOSED"

    def __str__(self):
        ret_str = "%s\t%s\t%s\t%s" % (str(self.session_id),
                str(self.associated_time), str(self.disassociated_time),
                self._print_status(self.status))
        return ret_str


class User(Base):
    """
    """
    __tablename__ = "user"

    user_id = Column(String, primary_key=True)
    user_password = Column(String)


def create_db(db_path):
    engine = create_engine('sqlite:///%s' % db_path, echo=False)
    Base.metadata.create_all(engine)

