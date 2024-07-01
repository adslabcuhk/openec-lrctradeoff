#!/bin/bash

exp_scripts_dir=/home/lrctradeoff/openec-lrctradeoff/exp_scripts

# num of runs
num_runs=1


##################################################################
# exp_configs
approach=tradeoff
eta=2
block_size_byte=67108864
cr_bw_Kbps=1048576

sed -i "s%\(num_runs = \)[^<]*%\1${num_runs}%" $exp_scripts_dir/eval_settings.ini
sed -i "s%\(approach = \)[^<]*%\1${approach}%" $exp_scripts_dir/eval_settings.ini
sed -i "s%\(eta = \)[^<]*%\1${eta}%" $exp_scripts_dir/eval_settings.ini
sed -i "s%\(block_size_byte = \)[^<]*%\1${block_size_byte}%" $exp_scripts_dir/eval_settings.ini
sed -i "s%\(cr_bw_Kbps = \)[^<]*%\1${cr_bw_Kbps}%" $exp_scripts_dir/eval_settings.ini

cd $exp_scripts_dir
python3 evaluation.py -eval_settings_file eval_settings.ini

##################################################################
