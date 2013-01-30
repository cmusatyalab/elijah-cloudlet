#!/bin/bash
# sync daily DB update from http://www.hostip.info/dl/index.html

rsync -avz --progress hostip.info::hostip ./hostip_db/
