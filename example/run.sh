#!/bin/bash

DTF_DIR=$HOME/dtf/DTF/libdtf #set path to where the DTF library is
PNETCDF_DIR=$HOME/dtf/install #set path to the installed PnetCDF library
SPLIT_WRAPPER=/home/g9300001/u93005/dtf/split_world_wrap

export LD_LIBRARY_PATH=$DTF_DIR:$PNETCDF_DIR/lib:$LD_LIBRARY_PATH
export DTF_VERBOSE_LEVEL=0
export MAX_WORKGROUP_SIZE=2
export DTF_GLOBAL_PATH=.
export LD_PRELOAD=${SPLIT_WRAPPER}/libsplitworld.so

px=40
py=40
pz=20

psx=2
psy=2
psz=1

#px=$1
#py=$2
#pz=$3
#
#psx=$4
#psy=$5
#psz=$6

nproc=$(($psx*$psy*$psz))
#mpiexec -n $nproc ./writer/writer $px $py $pz $psx $psy $psz 1 F . &
#mpiexec -n $nproc ./reader/reader $px $py $pz $psx $psy $psz 1 T .
mpiexec -n $nproc ./writer/writer $px $py $pz $psx $psy $psz 1 F . &
mpiexec -n $nproc ./reader/reader $px $py $pz $psx $psy $psz 1 T .
wait

