#!/bin/bash

rm -rf ./cloudlet
git clone git@bitbucket.org:krha/cloudlet.git
rm -rf ./cloudlet/src/app
rm -rf ./cloudlet/src/ec2
rm -rf ./cloudlet/src/ISR
rm -rf ./cloudlet/src/measurement
rm -rf ./cloudlet/src/nova
rm -rf ./cloudlet/deploy
rm -rf ./cloudlet/tmp
rm -rf ./cloudlet/TODO.md
rm -rf ./cloudlet/.git/
rm -rf ./cloudlet/.gitignore

tar cvfz cloudlet.tar.gz cloudlet/
rm -rf ./cloudlet
