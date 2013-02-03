drop table if exists base_vm;
create table base_vm(
    sha256_id string promary key,
	base_disk_path string not null,
	UNIQUE(base_disk_path)
);

