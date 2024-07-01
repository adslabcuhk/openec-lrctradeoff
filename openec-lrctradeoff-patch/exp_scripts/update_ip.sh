#!/bin/bash

home_dir=$(echo ~)
proj_dir=${home_dir}/openec-lrctradeoff
config_dir=${proj_dir}/conf
config_filename=sysSetting.xml
hadoop_home_dir=${home_dir}/hadoop-3.3.4

echo $hadoop_home_dir

cur_ip=$(ifconfig | grep 192.168.0 | head -1 | sed "s/ *inet [addr:]*\([^ ]*\).*/\1/")

sed -i "s%\(<attribute><name>local.addr</name><value>\)[^<]*%\1${cur_ip}%" ${config_dir}/${config_filename}
sed -i "s%\(<property><name>oec.local.addr</name><value>\)[^<]*%\1${cur_ip}%" ${hadoop_home_dir}/etc/hadoop/hdfs-site.xml
