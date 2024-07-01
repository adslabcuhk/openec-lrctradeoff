#!/bin/bash
# update sizes: block size and packet size

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
hadoop_home_dir=${home_dir}/hadoop-3.3.4

echo $hadoop_home_dir

# block size
block_size_MiB=`expr $block_size / 1024 / 1024`
sed -i "s%\(</ecid><base>\)[^<]*%\1${block_size_MiB}%" ${config_dir}/${config_filename}
sed -i "s%\(<property><name>dfs.blocksize</name><value>\)[^<]*%\1${block_size}%" ${hadoop_home_dir}/etc/hadoop/hdfs-site.xml

# packet size
sed -i "s%\(<attribute><name>packet.size</name><value>\)[^<]*%\1${packet_size}%" ${config_dir}/${config_filename}
sed -i "s%\(<property><name>oec.pktsize</name><value>\)[^<]*%\1${packet_size}%" ${hadoop_home_dir}/etc/hadoop/hdfs-site.xml
