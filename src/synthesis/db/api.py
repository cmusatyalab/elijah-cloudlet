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
DB wrapper for cloudlet
"""

import os
import sqlalchemy
import sys
from synthesis.Configuration import Const
from sqlalchemy.orm import sessionmaker

from table_def import create_db
from table_def import BaseVM, OverlayVM, User, Session


class DBConnector(object):
    def __init__(self, log=sys.stdout):

        # create DB file if it does not exist
        if not os.path.exists(Const.CLOUDLET_DB):
            log.write("[DB] Create new database\n")
            create_db(Const.CLOUDLET_DB)

        # mapping existing DB to class
        self.engine = sqlalchemy.create_engine('sqlite:///%s' % Const.CLOUDLET_DB, echo=False)
        session_maker = sessionmaker(bind=self.engine)
        self.session = session_maker()

    def add_item(self, entry):
        self.session.add(entry)
        self.session.commit()

    def del_item(self, entry):
        self.session.delete(entry)
        self.session.commit()

    def list_item(self, entry):
        ret = self.session.query(entry)
        return ret


