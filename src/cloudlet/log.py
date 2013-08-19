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

import os
import logging
import sys
import stat
from cloudlet.Configuration import Const as Const

loggers = dict()
DEFAULT_FORMATTER = '%(asctime)s %(name)s %(levelname)s %(message)s'

def getLogger(name='unknown'):
    if loggers.get(name, None) == None:
        # default file logging
        log_filepath = "/var/tmp/cloudlet/log-synthesis"
        if hasattr(Const, "LOG_PATH") == True:
            log_filepath = Const.LOG_PATH
        if os.path.exists(os.path.dirname(log_filepath)) == False:
            os.makedirs(os.path.dirname(log_filepath))
            os.chmod(os.path.dirname(log_filepath), 0o777)
            open(log_filepath, "w+").close()
            os.chmod(log_filepath, 0o666)
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


