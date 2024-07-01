#!/bin/bash
# update configs: ip, block size and packet size

if [ "$#" != "2" ]; then
	echo "Usage: $0 block_size (Byte) packet_size (Byte)" >&2
    exit 1
fi

block_size=$1
packet_size=$2

home_dir=$(echo ~)
proj_dir=${home_dir}/openec-lrctradeoff
config_dir=${proj_dir}/conf
config_filename=sysSetting.xml
hdfs_config_dir=${proj_dir}/hdfs3.3.4-integration/conf
hadoop_home_dir=$(echo $HADOOP_HOME)


# update configs
bash copy_dist.sh ${config_dir}/${config_filename} ${config_dir}
cp -r ${hdfs_config_dir}/* ${hadoop_home_dir}/etc/hadoop
chmod 777 ${hadoop_home_dir}/etc/hadoop/rack_topology.sh
bash copy_dist.sh ${hadoop_home_dir}/etc/hadoop ${hadoop_home_dir}/etc
bash run_script_dist.sh update_ip.sh
bash run_script_dist.sh update_sizes.sh $block_size $packet_size
