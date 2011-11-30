#
# vmm.py - Helper code for OpenISR (R) VMM drivers written in Python
#
# Copyright (C) 2008-2009 Carnegie Mellon University
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

import os
import sys
import errno
import signal
import subprocess
import stat
import traceback
import __main__

__all__ = 'VmmError', 'main', 'find_program', 'run_program'

VMNAME = "UnknownVMM"
USES_ROOT = "no"
DEBUG = False

class VmmError(Exception):
	pass

def _init():
	global OPTIONS
	for var in 'NAME', 'CFGDIR', 'UUID', 'SECTORS', 'MEM', \
				'FULLSCREEN', 'SUSPENDED', 'COMMAND', \
				'VERBOSE':
		if os.environ.has_key(var):
			exec('global ' + var + ';' + var + ' = "' + \
						os.environ[var] + '"')
	OPTIONS = dict()
	for opt in os.environ.get('OPTIONS', '').split(','):
		split = opt.split('=', 1) + ['1']
		if split[0] != '':
			OPTIONS.update([split[0:2]])

def _exception_msg(inst):
	if DEBUG:
		traceback.print_exc()
	if inst.__class__ == VmmError:
		return str(inst)
	else:
		return "%s: %s" % (inst.__class__.__name__, inst)

def main():
	if len(sys.argv) <= 1:
		print >>sys.stderr, "No mode specified"
		sys.exit(1)
	elif sys.argv[1] == "info":
		try:
			__main__.info()
		except Exception, inst:
			print "VMM=%s" % VMNAME
			print "RUNNABLE=no"
			print "RUNNABLE_REASON=%s" % _exception_msg(inst)
		else:
			print "VMM=%s" % VMNAME
			print "USES_ROOT=%s" % USES_ROOT
			print "RUNNABLE=yes"
	elif sys.argv[1] == "run":
		try:
			__main__.run()
		except Exception, inst:
			print "SUSPENDED=%s" % SUSPENDED
			print "SUCCESS=no"
			print "ERROR=%s" % _exception_msg(inst)
		else:
			print "SUSPENDED=%s" % SUSPENDED
			print "SUCCESS=yes"
	elif sys.argv[1] == "poweroff":
		try:
			__main__.poweroff()
		except Exception, inst:
			print "SUSPENDED=%s" % SUSPENDED
			print "SUCCESS=no"
			print "ERROR=%s" % _exception_msg(inst)
		else:
			print "SUSPENDED=%s" % SUSPENDED
			print "SUCCESS=yes"
	elif sys.argv[1] == "cleanup":
		try:
			if hasattr(__main__, 'cleanup'):
				__main__.cleanup()
		except Exception, inst:
			print "SUCCESS=no"
			print "ERROR=%s" % _exception_msg(inst)
		else:
			print "SUCCESS=yes"
	else:
		print >>sys.stderr, "Unknown mode specified"
		sys.exit(1)
	sys.exit(0)

def _executable(path):
	try:
		st = os.stat(path)
	except OSError:
		return False
	return st[stat.ST_MODE] & stat.S_IXUSR|stat.S_IXGRP|stat.S_IXOTH > 0

# If prog is an absolute path and executable, return it.  If it is a relative
# path and executable via the specified search path (defaulting to PATH),
# return the absolute path to the executable.  If no executable is found,
# return false.
def find_program(prog, search_path = os.environ['PATH'].split(':')):
	if prog[0] == '/':
		if _executable(prog):
			return prog
		else:
			return False
	for dirname in search_path:
		path = dirname + '/' + prog
		if _executable(path):
			return path
	return False

# Run a program and wait for it to exit, redirecting its stdout to stderr so
# that the child can't write key-value pairs back to our calling process.
# sigint_handler specifies a function to be called if we receive SIGINT;
# it takes the pid of the child as its first argument.  If sigint_handler is
# not specified, we ignore SIGINT while the child is running.  If new_pgrp
# is True, we run the child in its own process group and redirect its stdin/
# stdout/stderr to /dev/null.
def run_program(args, sigint_handler = lambda pid: None, new_pgrp = False):
	restore = None
	proc = None
	def handle_signal(sig, frame):
		if proc is not None:
			sigint_handler(proc.pid)
	try:
		restore = signal.signal(signal.SIGINT, handle_signal)
		if new_pgrp:
			proc = subprocess.Popen(args, \
					stdin = file("/dev/null", "r"), \
					stdout = file("/dev/null", "w"), \
					stderr = file("/dev/null", "w"), \
					preexec_fn = os.setpgrp)
		else:
			fd = os.dup(sys.stderr.fileno())
			proc = subprocess.Popen(args, stdout = fd)
			os.close(fd)
		while True:
			try:
				proc.wait()
			except OSError, e:
				if e.errno == errno.EINTR:
					continue
				raise
			else:
				break
	finally:
		if restore is not None:
			signal.signal(signal.SIGINT, restore)
	return proc.returncode

_init()
