#!/bin/bash
# usage: run script on all nodes

if [ "$#" -lt "1" ]; then
	echo "Usage: $0 <script>.sh <args>" >&2
    exit 1
fi

source "./config.sh"

cur_dir=`pwd`
script=$1
args=${@:2}

if [ ! -f "$cur_dir/$script" ]; then
    echo "script $script does not exist"
    exit 1
fi

# run script
for idx in $(seq 0 $((num_nodes-1))); do
    ip=${ip_list[$idx]}
    user=${user_list[$idx]}
    passwd=${passwd_list[$idx]}
    
    echo ssh -n $user@$ip "cd $cur_dir && bash $script $args"
    ssh -n $user@$ip "cd $cur_dir && bash $script $args"
done