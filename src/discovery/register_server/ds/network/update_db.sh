#!/bin/bash
# sync daily DB update from http://www.hostip.info/dl/index.html

# DB from hostip
rsync -avz --progress hostip.info::hostip ./db/

