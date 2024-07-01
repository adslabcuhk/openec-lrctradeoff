#!/bin/bash
# usage: clear route setting

if [ "$#" != "2" ]; then
	echo "Usage: $0 dst_ip gw_ip" >&2
    exit 1
fi

passwd=lrctradeoff
dst_ip=$1
gw_ip=$2

cur_ip=$(ifconfig | grep '192.168.0' | head -1 | sed "s/ *inet [addr:]*\([^ ]*\).*/\1/")
cur_dev=$(ifconfig | grep -B1 $cur_ip | grep -o "^\w*")

echo $passwd | sudo -S route del -net $dst_ip netmask 255.255.255.255 gw $gw_ip

echo disable route setting [ $cur_ip - $gw_ip - $dst_ip ]
