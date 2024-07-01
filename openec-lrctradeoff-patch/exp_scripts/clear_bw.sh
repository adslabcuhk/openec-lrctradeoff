#!/bin/bash
# usage: clear bandwidth setting (on the gateway node)

passwd=lrctradeoff
home_dir=$(echo ~)
wondershaper_dir=$home_dir/wondershaper

cur_ip=$(ifconfig | grep '192.168.0' | head -1 | sed "s/ *inet [addr:]*\([^ ]*\).*/\1/")
cur_dev=$(ifconfig | grep -B1 $cur_ip | grep -o "^\w*")

cd $wondershaper_dir && echo $passwd | sudo -S ./wondershaper -c -a $cur_dev

echo clear bandwidth setting [ $cur_ip $cur_dev ]
