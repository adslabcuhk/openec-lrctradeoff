# Harmonizing Repair and Maintenance in LRC-Coded Storage

This repository contains the implementation of our configurable data placement
scheme for LRC-coded storage that operates along the trade-off between repair
and maintenance operations.

## Paper

Please refer to our SRDS 2024 paper for more details.

## Overview

We provide the source code for both simulation and prototype experiments as
described in our paper. For simulation, we implement an ILP solver to obtain
optimal data placment schemes based on Gurobi. Gurobi is a highly optimized
solver to address general ILP problems. Our prototype builds atop Hadoop HDFS
with OpenEC, and is organized as a patch to OpenEC. OpenEC is a middleware
system realizing erasure coding schemes in the form of direct acyclic graphs
(called ECDAG). The coding operations are implemented based on ISA-L.

We implement our configurable data placement schemes for LRCs in the form of
ECDAGs. We integrate our implementation to OpenEC v1.0.0 atop Hadoop HDFS
3.3.4. We provide instructions to run both simulations and our prototype
experiments with different data placement schemes.

Useful Links:
- Gurobi: [link](https://www.gurobi.com/)
- OpenEC: [project website](http://adslab.cse.cuhk.edu.hk/software/openec/),
  [paper](https://www.usenix.org/conference/fast19/presentation/li) (USENIX
  FAST'19)
- Hadoop 3.3.4: [link](https://hadoop.apache.org/docs/r3.3.4/)
- ISA-L: [link](https://github.com/intel/isa-l)


## Folder Structure

The simulation source code is in ```analysis/```.

```
// ILP solver to obtain optimal ADC and AMC (based on Gurobi)
- analysis/lrc_opt.py
// Calculate ADC and AMC for our data placement scheme
- analysis/lrc_tradeoff.py
```

The source code patch for OpenEC is in ```openec-lrctradeoff-patch/```. The
ECDAG implementation of data placement schemes is in
```openec-lrctradeoff-patch/src/ec/```.

```
- src/ec/AzureLRCFlat.* // Flat data placement
- src/ec/AzureLRCOptR* //  Optimal placement with minimum ADC
- src/ec/AzureLRCOptM* // Optimal data placement with minimum AMC
- src/ec/AzureLRCTradeoff.* // Our configurable data placement
```

Note that ```AzureLRCOptR*``` and ```AzureLRCOptM*``` are implemented based on
the optimal solution obtained by the ILP solver (i.e., from ```lrc_opt.py```).


We provide sample configurations in
```openec-lrctradeoff-patch/conf/sysSetting.xml```. For more details of the
configuration file, please refer to OpenEC's [documentation](https://github.com/ukulililixl/openec/blob/master/doc/doc.pdf).

We list different data placement schemes for Azure-LRC(k=10,l=2,g=2) as below:

| Code | ClassName | Parameters *(k,l,g)* |
| ------ | ------ | ------ |
| Flat | AF_14_10 | (10,2,2) |
| Opt-R | AOR_14_10_r, AOR_14_10_m | (10,2,2) |
| Opt-M | AOM_14_10_r, AOM_14_10_m | (10,2,2) |
| Trade-off-0 | AT_14_10_0_r, AT_14_10_0_m | (10,2,2), eta=0 |
| Trade-off-1 | AT_14_10_1_r, AT_14_10_1_m | (10,2,2), eta=1 |
| Trade-off-2 | AT_14_10_2_r, AT_14_10_2_m | (10,2,2), eta=2 |

Note that the ClassName ends with "_r" represents the settings for
**regular mode**, while the ClassName ends with "_m" represents the settings
for **maintenance mode**.

## Deployment

Please follow the below steps:

* [Cluster setup](#cluster-setup)

* [Download HDFS and OpenEC](#download-hdfs-and-openec)

* [Add patch to OpenEC](#add-patch-to-openec)

* [Deploy HDFS with patched
  OpenEC](#deploy-hdfs-with-patched-openec)

* [Start HDFS with OpenEC](#start-hdfs-with-openec)

* [Run simulation](#run-simulation)

* [Run prototype](#run-prototype)


### Cluster Setup

We setup a local storage cluster of 17 machines (one for network core, one for
HDFS NameNode, 15 for DataNode). 

| Role | Number | IP |
| ------ | ------ | ------ |
| Network core | 1 | 192.168.0.1 |
| HDFS NameNode and OpenEC Controller | 1 | 192.168.0.2 |
| HDFS DataNode, OpenEC Agent and Client | 15 | 192.168.0.3-192.168.0.17 | 

On each machine, we create an account with the default username
```lrctradeoff```. Please change the IP addresses in the configuration files
of HDFS and OpenEC for your cluster accordingly.

### Download HDFS and OpenEC

On NameNode:

* Download the source code of HDFS-3.3.4 in ```/home/lrctradeoff/hadoop-3.3.4-src```

* Download the source code of OpenEC v1.0.0 in ```/home/lrctradeoff/openec-v1.0.0```

* Download the source code patch in ```/home/lrctradeoff/openec-lrctradeoff```

### Add patch to OpenEC

Run the following commands to add the patch to OpenEC:

```
$ cp -r /home/lrctradeoff/openec-v1.0.0/* /home/lrctradeoff/openec-lrctradeoff
$ cp -r /home/lrctradeoff/openec-lrctradeoff/openec-lrctradeoff-patch/* /home/lrctradeoff/openec-lrctradeoff
```

### Deploy HDFS with patched OpenEC

Please follow the OpenEC documentation
([Link](https://github.com/ukulililixl/openec/blob/master/doc/doc.pdf)) for
deploying HDFS-3.3.4 and OpenEC in the cluster.

Notes:

* the sample configuration files for HDFS-3.3.4 are in
```openec-lrctradeoff-patch/hdfs3-integration/conf```.

We set the default block size as 64MiB and packet size as 1MiB. For other
configurations, we follow the defaults in OpenEC documentation.


In ```hdfs-site.xml``` of HDFS:

| Parameter | Description | Example |
| ------ | ------ | ------|
| dfs.block.size | The size of a block in bytes. | 67108864 for 64 MiB block size. |
| oec.pktsize | The size of a packet in bytes. | 1048576 for 1 MiB packet size. |


In ```conf/sysSetting.xml``` of OpenEC:

| Parameter | Description | Example |
| ------ | ------ | ------ |
| packet.size | The size of a packet in bytes. | 1048576 for 1MiB. |


### Run Simulation

Please install Gurobi on your machine first, and ensure that you
have a valid account to run the ILP solver (e.g., [a free academic license](https://www.gurobi.com/academia/academic-program-and-licenses/)).

* Reference: [Link](https://www.gurobi.com/downloads/gurobi-software/)

To obtain the optimal data placement schemes for Azure-LRC(10,2,2), please run the following commands:

```
// Opt-R
python3 lrc_opt.py -eck 10 -ecl 2 -ecg 2 -problem adc
// Opt-M
python3 lrc_opt.py -eck 10 -ecl 2 -ecg 2 -problem amc
```

To calculate the ADC and AMC for our trade-off data placement schemes for
Azure-LRC(10,2,2), please run the following command:

```
python3 lrc_tradeoff.py -eck 10 -ecl 2 -ecg 2
```

### Run Prototype

Below is an example of running degraded read operations in both regular mode
and maintenance mode for Azure-LRC(10,2,2) for our trade-off data placement
with eta=1 (i.e., Trade-off-1).

We also provide a script ```exp_scripts/evaluation.py``` for
testbed experiments. The experiment configurations are listed in
```exp_scripts/eval_settings.ini```.

#### Configure Rack Topology

We configure the rack topology to run the data placement. We use Wondershaper
to configure the inner-rack and cross-rack network bandwidth. Please put the
folder of Wondershaper under ```/home/openec-lrctradeoff```.

```
git clone https://github.com/magnific0/wondershaper.git
```

#### Start HDFS with OpenEC

* Start HDFS

```
$ hdfs namenode -format
$ start-dfs.sh
```

* Start OpenEC

```
$ cd openec-lrctradeoff
$ python script/start.py
```

For more details, please follow the documentation of OpenEC.

#### Write Blocks

We use **degraded read in regular mode** as an example. The code ClassName is
```AT_14_10_1_r```. For maintenance mode, you can run the same sets of
commands by changing the corresponding ClassName to maintenance mode (i.e.,
```AT_14_10_1_m```).

On one of the HDFS DataNode:

- Use OpenEC Client to issue a file write (online write). Assume the filename
  is ```640MiB``` and the file size is: 640 MiB = *k = 10* HDFS blocks. The
  file is encoded into *k+l+g = 14* HDFS blocks and randomly distributed to 15
  storage nodes.

```
cd ~/openec-lrctradeoff
./OECClient write 640MiB /repair_AT AT_14_10_1_r online 640
```

We can check the data placement of blocks with:

```
hdfs fsck / -files -blocks -locations
```

For each block *i* (out of *n*), the filename format in HDFS is
```/repair_AT_oecobj_<i>```; the physical HDFS block name format is
```blk_<blkid>```.

e.g., the first block (or block 0) is stored in HDFS as
```/repair_AT_oecobj_0```. We can also get the IP address of DataNode that
stores block 0.

A sample log message copied from ```hdfs fsck``` are shown as below. The IP
address of the DataNode that stores block 0 is ```192.168.0.3```; the
physical block name stored in HDFS is named ```blk_1073741836```.

```
/repair_AT_oecobj_0 67108864 bytes, replicated: replication=1, 1 block(s):  OK
0. BP-1220575476-192.168.0.3-1672996056729:blk_1073741836_1011 len=67108864 Live_repl=1  [DatanodeInfoWithStorage[192.168.0.3:9866,DS-89dc6e26-7219-
412d-a7fa-e33dbbb14cfe,DISK]]
```

#### Manually Fail a Block

Now we manually remove a block from the storage nodes. For example, we
manually fail block 0 (named ```/repair_AT_oecobj_0``` in HDFS). **On the
DataNode that stores block 0**, we first enter the folder that physically
stores the block in HDFS:

```
cd ${HADOOP_HOME}/dfs/data/current/BP-*/current/finalized/subdir0/subdir0/
```

Only one block ```blk_<blkid>``` is stored together with its corresponding
metadata file ```blk_<blkid>.meta```. We manually remove the block (e.g.,
block 0):

```
cp blk_<blkid> ~/openec-lrctradeoff
rm blk_<blkid>
```

HDFS will automatically detect and report the lost block shortly after the
block manual removal. We can verify with ```hdfs fsck``` again.

```
hdfs fsck / -list-corruptfileblocks
```

#### Repair a Failed Block

After the failure is detected, we issue a degraded read to block 0 (named
```/repair_AT_0```, **without "\_oecobj\_"**) **in a data node that is not
storing the stripe** with the OpenEC Client's read request, and store it as
```repair_AT_0```.

```
cd ~/openec-lrctradeoff
./OECClient read /repair_AT_0 repair_AT_0
```

We can verify that the repaired block is the same as the manually failed block
stored in ```~/openec-lrctradeoff/```.

```
cd ~/openec-lrctradeoff
cmp repair_AT_0 blk_<blkid>
```
