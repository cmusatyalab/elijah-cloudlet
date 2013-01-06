#!/bin/bash

git clone git@bitbucket.org:krha/cloudlet.git
rm -r ./cloudlet/src/app
rm -r ./cloudlet/src/ec2
rm -r ./cloudlet/src/ISR
rm -r ./cloudlet/src/measurement
rm -r ./cloudlet/src/nova
rm -r ./cloudlet/deploy
rm -r ./cloudlet/tmp
rm -r ./cloudlet/TODO.md
rm -r ./cloudlet/.git/
rm -r ./cloudlet/.gitignore

tar cvfz cloudlet.tar.gz cloudlet/
rm -rf ./cloudlet
