#!/bin/bash
# usage: run command on all nodes

source "./config.sh"

cur_dir=`pwd`

# execute command
for idx in $(seq 0 $((num_nodes-1))); do
    ip=${ip_list[$idx]}
    user=${user_list[$idx]}
    passwd=${passwd_list[$idx]}
    
    echo ssh -n $user@$ip "echo $passwd | sudo -S apt-get -y install expect"
    ssh -n $user@$ip "echo $passwd | sudo -S apt-get -y install expect"
done
