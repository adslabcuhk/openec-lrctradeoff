#!/usr/bin/env python3
import argparse
import sys
import subprocess
from pathlib import Path
from collections import OrderedDict

home_dir = str(Path.home())
proj_dir = "{}/openec-lrctradeoff".format(home_dir)
exp_scripts_dir = "{}/exp_scripts".format(proj_dir)
hdfs_dir = "{}/hadoop-3.3.4".format(home_dir)
hdfs_config = "rack_topology.data"
ip_forward_script = "enable_ip_forward.sh"
clear_bw_script = "clear_bw.sh"
set_bw_script = "set_bw.sh"
clear_route_script = "clear_route.sh"
set_route_script = "set_route.sh"
user_name = "lrctradeoff"
user_passwd = "lrctradeoff"

def parse_args(cmd_args):
    arg_parser = argparse.ArgumentParser(description="set hierarchical network topology for the cluster (based on HDFS rack topology).")

    # input parameters
    arg_parser.add_argument("-option", type=str, required=True, help="option for bandwidth settings (set/clear)")
    arg_parser.add_argument("-cr_bw", type=int, required=True, help="cross-rack bandwidth (Kbps)")
    arg_parser.add_argument("-gw_ip", type=str, required=True, help="gateway node ip")
    
    args = arg_parser.parse_args(cmd_args)
    return args

def exec_cmd(cmd, exec=True):
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
    option = args.option
    cr_bw = args.cr_bw
    gw_ip = args.gw_ip

    # read rack topology file from hdfs
    node_to_rack = OrderedDict()
    rack_to_node = OrderedDict()
    hdfs_config_path = "{}/etc/hadoop/{}".format(hdfs_dir, hdfs_config)
    with open(hdfs_config_path, "r") as f:
        for line in f.readlines():
            if len(line.strip()) == 0:
                continue
            arr = line.strip().split(' ')
            node_ip = arr[0]
            rack_id = int(arr[1])
            node_to_rack[node_ip] = rack_id
            rack_to_node[rack_id] = rack_to_node.get(rack_id, []) + [node_ip]

    # Gateway
    if option == "clear":
        print("clear bandwidth for gateway {}".format(gw_ip))
        cmd = "ssh {}@{} \"cd {} && bash {}\"".format(user_name, gw_ip, exp_scripts_dir, clear_bw_script)
        exec_cmd(cmd, exec=True)
    elif option == "set":
        print("set bandwidth for gateway {}".format(gw_ip))
        cmd = "ssh {}@{} \"cd {} && bash {} {}\"".format(user_name, gw_ip, exp_scripts_dir, set_bw_script, cr_bw)
        exec_cmd(cmd, exec=True)
    else:
        print("unknown option: {}".format(option))
        return

    for cur_node_ip, cur_rack_id in node_to_rack.items():
        print("configure routing for node {} (rack {})".format(cur_node_ip, cur_rack_id))
        if option == "set":
            cmd = "ssh {}@{} \"cd {} && bash {}\"".format(user_name, cur_node_ip, exp_scripts_dir, ip_forward_script)
            exec_cmd(cmd, exec=True)

        for dst_node_ip, dst_rack_id in node_to_rack.items():
            # pass current node itself
            if cur_node_ip == dst_node_ip:
                continue
            # redirect to gateway for cross-rack traffic
            if cur_rack_id != dst_rack_id:
                if option == "clear":
                    cmd = "ssh {}@{} \"cd {} && bash {} {} {}\"".format(user_name, cur_node_ip, exp_scripts_dir, clear_route_script, dst_node_ip, gw_ip)
                    exec_cmd(cmd, exec=True)
                elif option == "set":
                    cmd = "ssh {}@{} \"cd {} && bash {} {} {}\"".format(user_name, cur_node_ip, exp_scripts_dir, set_route_script, dst_node_ip, gw_ip)
                    exec_cmd(cmd, exec=True)
                else:
                    print("unknown option: {}".format(option))
                    return

    print("Finished setting hierarchical network topology for the cluster")

if __name__ == '__main__':
    main()
