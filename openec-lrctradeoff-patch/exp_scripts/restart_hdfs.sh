#!/bin/bash

source "./config.sh"

stop-dfs.sh

# namenode
rm -rf $HADOOP_HOME/tmp
rm -rf $HADOOP_HOME/logs

# execute command
for idx in $(seq 0 $((num_nodes-1))); do
    ip=${ip_list[$idx]}
    user=${user_list[$idx]}
    passwd=${passwd_list[$idx]}
     
    ssh -n $user@$ip "rm -rf $HADOOP_HOME/tmp"
    ssh -n $user@$ip "rm -rf $HADOOP_HOME/logs"
    ssh -n $user@$ip "rm -rf $HADOOP_HOME/dfs"
done

hdfs namenode -format -force && start-dfs.sh
