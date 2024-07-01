#!/usr/bin/env python3
import argparse
import os
import sys
import subprocess
from pathlib import Path
import numpy as np
import math


def parse_args(cmd_args):
    arg_parser = argparse.ArgumentParser(description="Trade-off between average degraded-read cost (ARC) and average degraded-read-under-maintenance cost (AMC) for LRC") 

    # Input parameters: (k,l,g)
    arg_parser.add_argument("-eck", type=int, required=True, help="eck")
    arg_parser.add_argument("-ecl", type=int, required=True, help="ecl")
    arg_parser.add_argument("-ecg", type=int, required=True, help="ecg")
    
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

def check_params(eck, ecl, ecg):
    if eck % ecl != 0:
        print("Error: k is not a multiple of l")
        return False
    return True

def main():
    args = parse_args(sys.argv[1:])
    if not args:
        exit()

    # Input parameters: (k,l,g)
    eck = args.eck 
    ecl = args.ecl 
    ecg = args.ecg 

    # check parameters
    if check_params(eck, ecl, ecg) == False:
        return

    ecb = int(eck / ecl)
    ecn = eck + ecl + ecg

    eta_min = 0
    eta_max = math.ceil(ecb / (ecg + 1))

    print("Input parameters: Azure-LRC(k,l,g) = ({},{},{})".format(eck, ecl, ecg))
    print("Derived parameters: (n,b): ({}, {})".format(ecn, ecb))

    for eta in range(eta_min, eta_max+1):
        delta_j = -ecg * eta + ecb + 1

        # calculate repair cost
        ADC = delta_j - 1

        # calculate maintenance cost
        AMC = (ecl - ecg - 1) * (ecg + 1) * ecg / ecb * eta * eta + ecg * ecg * eta + ecb

        if eta == eta_max and ecb % (ecg + 1) != 0:
            # Special handling
            # print("Handling special case for eta (b % (g+1) != 0")
            delta_j = eta_max
            sigma = delta_j * ecl
            ADC = delta_j - 1
            AMC =  1.0 * (((eta_max - 1) * (ecg + 1)) * (sigma * ecg + delta_j - 1) + ((ecb % (ecg+1)) * (sigma * (ecb % (ecg+1))))) / ecb

        print("eta = {}, ADC = {}, AMC = {}".format(eta, ADC, AMC))

if __name__ == '__main__':
    main()