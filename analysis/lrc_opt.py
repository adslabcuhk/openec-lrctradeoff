#!/usr/bin/env python3
import argparse
import os
import sys
import subprocess
from pathlib import Path
import numpy as np
import gurobipy as gp
from gurobipy import GRB


def parse_args(cmd_args):
    arg_parser = argparse.ArgumentParser(description="optimal average degraded-read cost (ADC) and average degraded-read-under-maintenance cost (AMC) for LRC") 

    # Input parameters: (k,l,g)
    arg_parser.add_argument("-eck", type=int, required=True, help="eck")
    arg_parser.add_argument("-ecl", type=int, required=True, help="ecl")
    arg_parser.add_argument("-ecg", type=int, required=True, help="ecg")
    arg_parser.add_argument("-problem", type=str, required=True, help="optimization problem [adc/amc] (adc: optimal ADC; amc: optimal AMC)")
    
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
    max_num_racks = ecn # at most n racks

    print("Input parameters: Azure-LRC(k,l,g) = ({},{},{})".format(eck, ecl, ecg))
    print("Derived parameters: (n,b,max_num_racks): ({}, {}, {})".format(ecn, ecb, max_num_racks))

    try:
        # Create a new model
        model = gp.Model("lrc-optimization")

        ########################## Input Variables ##########################

        # alpha(i,j): number of data blocks stored in rack R_i from the local
        # group G_j. Type: non-negative integer
        alpha = model.addVars(max_num_racks, ecl, vtype=GRB.INTEGER, lb=0, name="alpha")

        # beta(i,j): number of local parity blocks stored in rack R_i from the
        # local group G_j.  Type: binary
        beta = model.addVars(max_num_racks, ecl, vtype=GRB.BINARY, name="beta")

        # gamma(i): number of global parity blocks stored in rack R_i. Type:
        # non-negative integer
        gamma = model.addVars(max_num_racks, vtype=GRB.INTEGER, lb=0, name="gamma")

        #####################################################################

        ####################### Constraints #################################

        # Constraint 1: each local group has b data blocks spanned over all racks
        for lg_id in range(ecl):
            model.addConstr(alpha.sum('*', lg_id) == ecb)

        # Constraint 2: each local group has one local parity block spanned over all racks
        for lg_id in range(ecl):
            model.addConstr(beta.sum('*', lg_id) == 1)

        # Constraint 3: each stripe has g global parity blocks spanned over all racks
        model.addConstr(gamma.sum('*') == ecg)

        # Constraint 4: each rack stores no more than g+i blocks spanned by i
        # local groups, subject to single-rack fault tolerance
        
        # R_blk_num(i): the number of blocks stored in rack R_i
        R_blk_num = model.addVars(max_num_racks, vtype=GRB.INTEGER, name="R_blk_num")
        for rack_id in range(max_num_racks):
            model.addConstr(R_blk_num[rack_id] == alpha.sum(rack_id, '*') + beta.sum(rack_id, '*') + gamma[rack_id])

        # I_blk_lg(i,j): Indicator variable; check whether rack R_i stores any
        # block from local group G_j
        I_blk_lg = model.addVars(max_num_racks, ecl, vtype=GRB.BINARY, name="I_blk_lg")
        for rack_id in range(max_num_racks):
            for lg_id in range(ecl):
                model.addGenConstrIndicator(I_blk_lg[rack_id, lg_id], True, alpha[rack_id, lg_id] + beta[rack_id, lg_id] >= 1)
                model.addGenConstrIndicator(I_blk_lg[rack_id, lg_id], False, alpha[rack_id, lg_id] + beta[rack_id, lg_id] == 0)

        for rack_id in range(max_num_racks):
            model.addConstr(R_blk_num[rack_id] <= ecg + I_blk_lg.sum(rack_id, '*'))

        #################### Auxiliary variables ############################

        # I_R(i): Indicator variable; check whether rack R_i stores any blocks
        I_R = model.addVars(max_num_racks, vtype=GRB.BINARY, name="I_R")
        for rack_id in range(max_num_racks):
            model.addGenConstrIndicator(I_R[rack_id], True, R_blk_num[rack_id] >= 1)
            model.addGenConstrIndicator(I_R[rack_id], False, R_blk_num[rack_id] == 0)

        # z: number of racks spanned by the LRC stripe
        z = model.addVar(vtype=GRB.INTEGER, name="z")
        model.addConstr(z == I_R.sum('*'))

        #####################################################################

        #################### Auxiliary constraints ###########################
          
        # Symmetry constraint (the latter racks stores no more blocks than the
        # former racks)
        for rack_id in range(max_num_racks - 1):
            model.addConstr(R_blk_num[rack_id] >= R_blk_num[rack_id + 1])

        #####################################################################

        #####################################################################

        if args.problem == "adc":
            ########### Optimization goal: minimize ADC #############

            # # I_alpha(): Indicator variable; check whether rack R_i stores any
            # # data block from local group G_j (I_alpha(i,j) = 1 when
            # # alpha(i,j) >= 1)
            # I_alpha = model.addVars(max_num_racks, ecl, vtype=GRB.BINARY, name="I_alpha")
            # for rack_id in range(max_num_racks):
            #     for lg_id in range(ecl):
            #         model.addGenConstrIndicator(I_alpha[rack_id, lg_id], True, alpha[rack_id, lg_id] >= 1)
            #         model.addGenConstrIndicator(I_alpha[rack_id, lg_id], False, alpha[rack_id, lg_id] == 0)

            # delta(j): number of racks spanned by local group G_j
            delta = model.addVars(ecl, vtype=GRB.INTEGER, name="delta")
            for lg_id in range(ecl):
                model.addConstr(delta[lg_id] == I_blk_lg.sum('*', lg_id))

            # r_cost(i,j): repair cost of data blocks of rack R_i from the local
            # group G_j

            # version 1 (the blocks in each local group have the same r_cost)
            r_cost_lg = model.addVars(ecl, vtype=GRB.INTEGER, name="r_cost_lg")
            for lg_id in range(ecl):
                model.addConstr(r_cost_lg[lg_id] == ecb * (delta[lg_id] - 1))
            
            # ADC: average degraded-read cost of the LRC stripe (of all data
            # blocks)
            ADC = model.addVar(vtype=GRB.CONTINUOUS, name="ADC")
            model.addConstr(ADC == r_cost_lg.sum('*') / eck)

            # # version 2
            # r_cost = model.addVars(max_num_racks, ecl, vtype=GRB.INTEGER, name="r_cost")
            # for rack_id in range(max_num_racks):
            #     for lg_id in range(ecl):
            #         model.addConstr((I_alpha[rack_id, lg_id] == 0) >> (r_cost[rack_id, lg_id] == 0))
            #         model.addConstr((I_alpha[rack_id, lg_id] == 1) >> (r_cost[rack_id, lg_id] == delta[lg_id] - 1))

            # # weighted r_cost(i,j): r_cost_weighted(i,j) = alpha(i,j) * r_cost(i,j)
            # r_cost_weighted = model.addVars(max_num_racks, ecl, vtype=GRB.INTEGER, name="r_cost_weighted")
            # for rack_id in range(max_num_racks):
            #     for lg_id in range(ecl):
            #         model.addConstr(r_cost_weighted[rack_id, lg_id] == alpha[rack_id, lg_id] * r_cost[rack_id, lg_id])
            
            # # ADC: average repair cost of the LRC stripe (of all data blocks)
            # ADC = model.addVar(vtype=GRB.CONTINUOUS, name="ADC")
            # model.addConstr(ADC == r_cost_weighted.sum('*', '*') / eck)

            # Set objective
            model.setObjective(ADC, GRB.MINIMIZE)

            # Start from an initial solution (combined locality) Reminded
            # about the symmetric constraint that the number of blocks per
            # rack is in descending order
            for rack_id in range(max_num_racks):
                for lg_id in range(ecl):
                    alpha[rack_id, lg_id].Start = 0
                    beta[ecb, lg_id].Start = 0
                gamma[rack_id].Start = 0

            max_blocks_per_lg = ecg + 1 # g+1 blocks per rack
            num_racks_lg = (ecb + 1) // max_blocks_per_lg
            remainder_blocks_lg = (ecb + 1) % max_blocks_per_lg
            cur_rack_id = 0
            for lg_id in range(ecl):
                for i in range(num_racks_lg):
                    # put g+1 data blocks
                    alpha[cur_rack_id, lg_id].Start = max_blocks_per_lg
                    # check if the last rack can store g+1 blocks, including g
                    # data blocks and one local parity block
                    if i == num_racks_lg - 1 and remainder_blocks_lg == 0:
                        alpha[cur_rack_id, lg_id].Start = max_blocks_per_lg - 1
                        beta[cur_rack_id, lg_id].Start = 1
                    cur_rack_id += 1
            # Check if the global parity blocks should be put in front of the
            # remaining blocks per local group
            if ecg > remainder_blocks_lg:
                gamma[cur_rack_id].Start = ecg
                cur_rack_id += 1
            # Put the remaining blocks per local group
            if remainder_blocks_lg > 0:
                for lg_id in range(ecl):
                    alpha[cur_rack_id, lg_id].Start = remainder_blocks_lg - 1
                    beta[cur_rack_id, lg_id].Start = 1
                    cur_rack_id += 1
                if ecg <= remainder_blocks_lg:
                    gamma[cur_rack_id].Start = ecg
                    cur_rack_id += 1
                
            #####################################################################
        elif args.problem == "amc":
            ######### Optimization goal: minimize AMC ###########

            # Definition of sigma (version 1)
            # I_sigma(i): Indicator variable; check whether rack R_i stores
            # any data block or global parity
            I_sigma = model.addVars(max_num_racks, vtype=GRB.BINARY, name="I_sigma")
            for rack_id in range(max_num_racks):
                model.addGenConstrIndicator(I_sigma[rack_id], True, alpha.sum(rack_id, '*') >= 1)
                model.addGenConstrIndicator(I_sigma[rack_id], False, alpha.sum(rack_id, '*') == 0)
            
            # sigma: number of racks spanned by the data blocks and global
            # parity blocks
            sigma = model.addVar(vtype=GRB.INTEGER, name="sigma")
            model.addConstr(sigma == I_sigma.sum('*') + 1)

            # # Definition of sigma (version 2)
            # # I_sigma(i): Indicator variable; check whether rack R_i stores
            # # any data block or global parity
            # I_sigma = model.addVars(max_num_racks, vtype=GRB.BINARY, name="I_sigma")
            # for rack_id in range(max_num_racks):
            #     model.addGenConstrIndicator(I_sigma[rack_id], True, alpha.sum(rack_id, '*') + gamma[rack_id] >= 1)
            #     model.addGenConstrIndicator(I_sigma[rack_id], False, alpha.sum(rack_id, '*') + gamma[rack_id] == 0)

            # # sigma: number of racks spanned by the data blocks and global
            # # parity blocks
            # sigma = model.addVar(vtype=GRB.INTEGER, name="sigma")
            # model.addConstr(sigma == I_sigma.sum('*'))

            # I_alpha(): Indicator variable; check whether rack R_i stores any
            # data block from local group G_j (I_alpha(i,j) = 1 when
            # alpha(i,j) >= 1)
            I_alpha = model.addVars(max_num_racks, ecl, vtype=GRB.BINARY, name="I_alpha")
            for rack_id in range(max_num_racks):
                for lg_id in range(ecl):
                    model.addGenConstrIndicator(I_alpha[rack_id, lg_id], True, alpha[rack_id, lg_id] >= 1)
                    model.addGenConstrIndicator(I_alpha[rack_id, lg_id], False, alpha[rack_id, lg_id] == 0)

            # delta(j): number of racks spanned by local group G_j
            delta = model.addVars(ecl, vtype=GRB.INTEGER, name="delta")
            for lg_id in range(ecl):
                model.addConstr(delta[lg_id] == I_blk_lg.sum('*', lg_id))

            # m_cost_global_lg_tmp(i,j): tmp variable (as Gurobi doesn't
            # support multiplying consecutive three integers in one variable)
            m_cost_global_lg_tmp = model.addVars(max_num_racks, ecl, vtype=GRB.INTEGER, name="m_cost_global_lg_tmp")
            for rack_id in range(max_num_racks):
                for lg_id in range(ecl):
                    model.addConstr(m_cost_global_lg_tmp[rack_id, lg_id] == I_alpha[rack_id, lg_id] * (1 - beta[rack_id, lg_id]))

            # m_cost_global_lg(i,j): representing the maintenance cost to
            # relate all the data blocks of local group G_j in rack R_i
            m_cost_global_lg = model.addVars(max_num_racks, ecl, vtype=GRB.INTEGER, name="m_cost_global_lg")
            for rack_id in range(max_num_racks):
                for lg_id in range(ecl):
                    model.addConstr(m_cost_global_lg[rack_id, lg_id] == alpha[rack_id, lg_id] * (sigma - 1) + m_cost_global_lg_tmp[rack_id, lg_id] * (delta[lg_id] - sigma))

            # m_cost_global(i): maintenance cost of the data blocks in rack
            # R_i (global repair)
            m_cost_global = model.addVars(max_num_racks, vtype=GRB.INTEGER, name="m_cost_global")
            for rack_id in range(max_num_racks):
                model.addConstr(m_cost_global[rack_id] == m_cost_global_lg.sum(rack_id, '*'))

            # I_local(i): Indicator variable that whether the data blocks in
            # j-th local group can be locally repaired in the i-th rack. It
            # requires that alpha[rack_id, lg_id] >= 1
            I_local = model.addVars(max_num_racks, ecl, vtype=GRB.BINARY, name="I_local")
            for rack_id in range(max_num_racks):
                for lg_id in range(ecl):
                    model.addGenConstrIndicator(I_local[rack_id, lg_id], True, alpha[rack_id, lg_id] + beta[rack_id, lg_id] <= 1)
                    model.addGenConstrIndicator(I_local[rack_id, lg_id], False, alpha[rack_id, lg_id] + beta[rack_id, lg_id] >= 2)
            
            # m_cost(i,j): maintenance cost for the data block from local
            # group G_j in rack R_i (combining local and global repair)
            m_cost = model.addVars(max_num_racks, ecl, vtype=GRB.INTEGER,
            lb=0, ub=eck, name="m_cost")
            for rack_id in range(max_num_racks):
                for lg_id in range(ecl):
                    model.addConstr(m_cost[rack_id, lg_id] ==
                    (I_local[rack_id, lg_id] * (delta[lg_id] - 1) + (1 -
                    I_local[rack_id, lg_id]) * m_cost_global[rack_id]))

            # m_cost_sum(i,j): sum of maintenance costs for all the data
            # blocks for local group G_j in rack R_i
            m_cost_lg_sum = model.addVars(max_num_racks, ecl, vtype=GRB.INTEGER, lb=0, ub=eck, name="m_cost_lg_sum")
            for rack_id in range(max_num_racks):
                for lg_id in range(ecl):
                    model.addConstr(m_cost_lg_sum[rack_id, lg_id] == alpha[rack_id, lg_id] * m_cost[rack_id, lg_id])

            # AMC: average maintenance cost of the LRC stripe (continuous)
            AMC = model.addVar(vtype=GRB.CONTINUOUS, name="AMC")
            model.addConstr(AMC == m_cost_lg_sum.sum('*', '*') / eck)

            # Set objective
            model.setObjective(AMC, GRB.MINIMIZE)


            # Start from an initial solution (maintenance-robust-efficient deployment)
            for rack_id in range(max_num_racks):
                for lg_id in range(ecl):
                    alpha[rack_id, lg_id].Start = 0
                    beta[ecb, lg_id].Start = 0
                gamma[rack_id].Start = 0

            for lg_id in range(ecl):
                for rack_id in range(ecb):
                    alpha[rack_id, lg_id].Start = 1
                beta[ecb, lg_id].Start = 1
            gamma[ecb + 1].Start = ecg

            #################################################################

        ############### Gurobi Parameter tuning ##############################
        
        model.setParam('MIPFocus', 3) # more focus on proving the objective bound
        model.setParam('Heuristics', 0) # only focus on proving the objective bound, instead of finding multiple feasible solutions
        model.setParam('NonConvex', 2) # Non-convex model

        ######################################################################


        # Optimize model
        model.optimize()

        for v in model.getVars():
            print(f"{v.VarName} {v.X:g}")

        print(f"Obj: {model.ObjVal:g}")


    except gp.GurobiError as e:
        print(f"Error code {e.errno}: {e}")
    # except AttributeError:
        # print("Encountered an attribute error")



if __name__ == '__main__':
    main()