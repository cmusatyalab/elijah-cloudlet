Latest Changes
-------------------
- master branch is merged with dev branch
	Between commit 2870f23b15ea20b177a879e981fc1532e7502fe9 and a9aedfdc483db7645b4982fdfcc6356f845617c5
	1. Use database to manage *base vm* and *overlay vm*
	2. Use simple authentication to keep track of VM usage

- Since we have newly instroduced DB, it does not provide lower level
compatibility. To fix this problem, 

	1. Install python DB binding code as in README.md file
		$ sudo pip install sqlalchymy
	2. Migrate current *base vm* information to new DB file
		>> // record current base vm information
		>> $ ./bin/cluodlet list_base
		>> // Delete previsou DB file
		>> $ rm ./src/server/config/cloudlet.db
		>> // add DB information to using cloudlet tool
		>> $ ./bin/cloudlet add_base [/path/to/base.img] [hash_value]


