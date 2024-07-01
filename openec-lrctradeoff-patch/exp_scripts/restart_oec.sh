#!/bin/bash

source "./config.sh"

home_dir=$(echo ~)
proj_dir=${home_dir}/openec-lrctradeoff
oec_scripts_dir=${proj_dir}/script
exp_scripts_dir=${proj_dir}/exp_scripts

# stop OEC
cd ${oec_scripts_dir}
python2.7 stop.py

# clear metadata
cd ${proj_dir}
rm -f coor_output
rm -f entryStore
rm -f poolStore

# clear logs
rm -f coor_output
rm -f hs_err*
for idx in $(seq 0 $((num_nodes-1))); do
    ip=${ip_list[$idx]}
    user=${user_list[$idx]}
    passwd=${passwd_list[$idx]}
    
    ssh -n $user@$ip "cd ${proj_dir}; rm -f hs_err*"
    ssh -n $user@$ip "cd ${proj_dir}; rm -f agent_output"
done

# restart HDFS
cd ${exp_scripts_dir}
bash restart_hdfs.sh

sleep 2

cd ${oec_scripts_dir}
python2.7 start.py
