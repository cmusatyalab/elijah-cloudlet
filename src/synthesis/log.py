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

import logging
import sys
from synthesis.Configuration import Const as Const

loggers = dict()
DEFAULT_FORMATTER = '%(asctime)s %(name)s %(levelname)s %(message)s'

def getLogger(name='unknown'):
    if loggers.get(name, None) == None:
        # default file logging
        log_filepath = "synthesis-log"
        if hasattr(Const, "LOG_PATH") == True:
            log_filepath = Const.LOG_PATH
        logging.basicConfig(level=logging.DEBUG,
                format='%(asctime)s %(name)-12s %(levelname)-8s %(message)s',
                datefmt='%m-%d %H:%M',
                filename=log_filepath,
                filemode='a')
        logger = logging.getLogger(name)
        hdlr = logging.FileHandler(log_filepath)
        hdlr.setLevel(logging.DEBUG)
        formatter = logging.Formatter(DEFAULT_FORMATTER)
        hdlr.setFormatter(formatter)

        # add stdout logging with INFO level
        console = logging.StreamHandler(sys.stdout)
        console.setLevel(logging.INFO)
        formatter = logging.Formatter('%(levelname)-8s %(message)s')
        console.setFormatter(formatter)
        logger.addHandler(console)
        

        loggers[name] = logger

    return loggers.get(name)


