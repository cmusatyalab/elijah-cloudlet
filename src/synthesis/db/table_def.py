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
DB Table definition
"""
from sqlalchemy import create_engine, ForeignKey
from sqlalchemy import Column, DateTime, Integer, String, BigInteger
from sqlalchemy.ext.declarative import declarative_base
from sqlalchemy.orm import relation, backref
import random
import datetime


Base = declarative_base()


class BaseVM(Base):
    """
    """
    __tablename__ = "base_vm"

    disk_path = Column(String, primary_key=True)
    hash_value = Column(String, unique=True, nullable=False)

    def __init__(self, disk_path, hash_value):
        self.disk_path = disk_path
        self.hash_value = hash_value


class Session(Base):
    """ """
    __tablename__ = "session"

    DIGIT = len(str(2**64))-3
    STATUS_RUNNING          = 1
    STATUS_CLOSE            = 2     # successfully closed by client
    STATUS_UNEXPECT_CLOSE   = 3     # force closed either from
                                    # 1) error handler
                                    # 2) server termination

    session_id = Column(BigInteger, primary_key=True)
    associated_time = Column(DateTime, nullable=False)
    disassociated_time = Column(DateTime, nullable=True)
    status = Column(Integer)

    def __init__(self):
        # generate DIGIT length long
        random.seed(datetime.datetime.now())
        self.session_id = long(random.randint(10**(Session.DIGIT-1), 10**(Session.DIGIT)-1))
        self.associated_time = datetime.datetime.now()
        self.disassociated_time = None
        self.status = Session.STATUS_RUNNING

    def terminate(self, status=STATUS_CLOSE):
        self.disassociated_time = datetime.datetime.now()
        self.status = status
        for overlay_vm in self.overlay_vms:
            if overlay_vm.status == OverlayVM.STATUS_RUNNING:
                overlay_vm.terminate(status=status)

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


class OverlayVM(Base):
    """
    """
    __tablename__ = "overlay_vm"

    STATUS_RUNNING          = 1
    STATUS_CLOSE            = 2     # successfully closed by client
    STATUS_UNEXPECT_CLOSE   = 3     # force closed due to unexpected reasons

    id = Column(Integer, primary_key=True, autoincrement=True)
    session_id = Column(String, ForeignKey(Session.session_id))
    basevm_path = Column(String, ForeignKey(BaseVM.disk_path))
    create_time = Column(DateTime, nullable=False)
    terminate_time = Column(DateTime, nullable=True)
    session = relation(Session, backref=backref('overlay_vms', order_by=id))
    status = Column(Integer)

    def __init__(self, session_id, basevm_path):
        self.session_id = session_id
        self.basevm_path = basevm_path
        self.create_time = datetime.datetime.now()
        self.status = OverlayVM.STATUS_RUNNING

    def terminate(self, status=STATUS_CLOSE):
        self.terminate_time = datetime.datetime.now()
        self.status = status

    def _print_status(self, status):
        if status == Session.STATUS_RUNNING:
            return "RUNNING"
        elif status == Session.STATUS_CLOSE:
            return "CLOSED"
        elif status == Session.STATUS_UNEXPECT_CLOSE:
            return "FORCE_CLOSED"

    def __str__(self):
        base_path = self.basevm_path
        if len(base_path) > 50:
            base_path = "...%s" % base_path[-47:]
        ret_str = "%s\t%s\t%s\t%s" % (str(self.id),
                str(self.session_id), str(base_path),
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

