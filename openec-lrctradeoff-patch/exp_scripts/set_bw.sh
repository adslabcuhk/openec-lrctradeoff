#!/bin/bash
# usage: enable bandwidth setting (on the gateway node)

if [ "$#" != "1" ]; then
	echo "Usage: $0 upload_bw (Kbps)" >&2
    exit 1
fi

upload_bw_Kbps=$1

passwd=lrctradeoff
home_dir=$(echo ~)
wondershaper_dir=$home_dir/wondershaper

cur_ip=$(ifconfig | grep '192.168.0' | head -1 | sed "s/ *inet [addr:]*\([^ ]*\).*/\1/")
cur_dev=$(ifconfig | grep -B1 $cur_ip | grep -o "^\w*")

cd $wondershaper_dir
echo $passwd | sudo -S ./wondershaper -a $cur_dev -u $upload_bw_Kbps
cd -

echo enable bandwidth setting [ $cur_ip $cur_dev $upload_bw_Kbps Kbps]
