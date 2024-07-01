#!/usr/bin/env python3
import argparse
import os
import sys
import subprocess
from pathlib import Path
import numpy as np
import math
import configparser
import re

home_dir = str(Path.home())
oec_dir = "{}/openec-lrctradeoff".format(home_dir)
placement_prog = "PlacementTest"
login_file = "login.txt"
oec_config = "sysSetting.xml"
hdfs_config = "rack_topology.data"

def parse_args(cmd_args):
    arg_parser = argparse.ArgumentParser(description="update rack topology from nodes (update rack ids in openec config file and HDFS config file).") 

    # input parameters
    arg_parser.add_argument("-oec_code_name", type=str, required=True, help="code name in OpenEC (must exist in sysSetting.xml)")
    
    arg_parser.add_argument("-dn_ids", type=int, nargs="+", required=True, help="data node ids in node login")
   
 
    args = arg_parser.parse_args(cmd_args)
    return args

def exec_cmd(cmd, exec=False):
    print("Execute Command: {}".format(cmd))
    msg = ""
    if exec == True:
        return_str, stderr = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE).communicate()
        msg = return_str.decode().strip()
        print(msg)
    return msg


def main():
    args = parse_args(sys.argv[1:])
    if not args:
        exit()

    # input parameters
    oec_code_name = args.oec_code_name
    dn_ids = args.dn_ids
    print(dn_ids)

    # read node login file
    node_logins = []
    with open(login_file, "r") as f:
        for line in f.readlines():
            if len(line.strip()) == 0:
                continue
            arr = line.strip().split(' ')
            node_logins.append(arr)

    data_nodes = [node_logins[item] for item in dn_ids]
    
    # run placement program to get rack info from oec_code
    cmd = "cd {} && ./{} {}".format(oec_dir, placement_prog, oec_code_name)
    msg = exec_cmd(cmd, exec=True)
    racks = {}
    node_to_rack = {}
    start_scan = False
    for line in msg.split("\n"):
        if "Hierarchical settings (start)" in line:
            start_scan = True
            continue
        if "Hierarchical settings (end)" in line:
            start_scan = False
            break
        
        if start_scan == True:
            # extract line
            # pattern: Group 0: 0 1 2
            ret = re.match(r"Group (\d+): (.*)", line)
            if ret:
                rack_id = int(ret.group(1))
                nodes = [int(item) for item in ret.group(2).split()]
                racks[rack_id] = nodes
                for node in nodes:
                    node_to_rack[node] = rack_id
    
    print("racks: {}".format(racks))
    
    # update rack ids in oec_config and hdfs_config
    oec_config_path = "{}/conf/{}".format(oec_dir, oec_config)
    hdfs_config_path = "{}/hdfs3.3.4-integration/conf/{}".format(oec_dir, hdfs_config)
    for node_id, node_login in enumerate(data_nodes):
        node_ip = node_login[0]

        # default rack id
        rack_id = 0
        if node_id in node_to_rack:
            rack_id = node_to_rack[node_id]
        else:
            rack_id = len(racks)
        
        print("processing node: {} {} (rack_id: {})".format(node_id, node_ip, rack_id))

        # oec_config pattern: <value>/rack0/192.168.0.1</value>
        cmd = "sed -i -E 's%<value>/rack[0-9]+/{}</value>%<value>/rack{}/{}</value>%g' {}".format(node_ip, rack_id, node_ip, oec_config_path)
        exec_cmd(cmd, exec=True)

        # hdfs_config pattern: 192.168.0.1 0
        cmd = "sed -i -E 's%{} [0-9]+%{} {}%g' {}".format(node_ip, node_ip, rack_id, hdfs_config_path)
        exec_cmd(cmd, exec=True)
    
    print("finished updating rack topology")
    

if __name__ == '__main__':
    main()
