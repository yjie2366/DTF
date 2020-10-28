#!/bin/bash

DTF_DIR=/work/gi17/i17000/DTF/libdtf #set path to where the DTF library is
PNETCDF_DIR=/work/gi17/i17000/install #set path to the installed PnetCDF library
#SPLIT_WRAPPER=/home/g9300001/u93005/dtf/split_world_wrap

cur_dir=$(cd $(dirname ${BASH_SOURCE[0]}) > /dev/null && pwd)
log_dir=${cur_dir}/output
output_file=${log_dir}/output.log

px=$1
py=$2
pz=$3

psx=$4
psy=$5
psz=$6

nproc=$(($psx*$psy*$psz))
name=$(hostname)
run=5

if [ ${nproc} -gt 32 ]; then
	nmasters=64
else
	nmasters=32
fi

work_grp=$((nproc/nmasters))

export LD_LIBRARY_PATH=$DTF_DIR:$PNETCDF_DIR/lib:$LD_LIBRARY_PATH
export DTF_VERBOSE_LEVEL=0
export MAX_WORKGROUP_SIZE=${work_grp}
export DTF_GLOBAL_PATH=.
#export LD_PRELOAD=${SPLIT_WRAPPER}/libsplitworld.so

for r in `seq 1 ${run}`; do
	if [[ "$name" == *"ofp"* ]]; then
		mpiexec -n $nproc ./writer/writer $px $py $pz $psx $psy $psz 1 F . &
		mpiexec -n $nproc ./reader/reader $px $py $pz $psx $psy $psz 1 T .
	fi
	wait
done

folder="${log_dir}/${nproc}-${PJM_JOBID}"
if [ ! -d ${folder} ]; then
	mkdir -p ${folder}
fi

find ${log_dir} -maxdepth 1 -type f -exec mv {} ${folder} \;
