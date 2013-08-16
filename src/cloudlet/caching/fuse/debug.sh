cd ..
python CloudletCache.py http://localhost/
cd fuse
gdb --args ./cachefs /tmp/cloudlet_cache/ localhost localhost 6379 redis_req redis_res
