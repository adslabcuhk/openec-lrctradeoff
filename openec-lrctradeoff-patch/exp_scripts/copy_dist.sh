#!/bin/bash
# usage: copy file/dir to all nodes

if [ "$#" != "2" ]; then
	echo "Usage: $0 src_file/src_dir dst_dir" >&2
    exit 1
fi

source "./config.sh"

src_file=$1
dst_dir=$2

# copy file/dir
for idx in $(seq 0 $((num_nodes-1))); do
    ip=${ip_list[$idx]}
    user=${user_list[$idx]}
    passwd=${passwd_list[$idx]}
    
    echo rsync -av --delete --recursive $src_file $user@$ip:$dst_dir
    rsync -av --delete --recursive $src_file $user@$ip:$dst_dir
done
