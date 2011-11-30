#!/bin/sh

if [ -e configure ] ; then
	# If we've run before, do a fast refresh unless we're told otherwise.
	if [ "$1" = "-f" -o "$1" = "--force" ] ; then
		autoreconf --install --force
	else
		autoreconf
	fi
else
	autoreconf --install --force
fi
