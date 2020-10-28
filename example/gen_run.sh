#!/bin/sh
cur_dir=$(cd $(dirname ${BASH_SOURCE[0]}) > /dev/null && pwd)
batch_script=${cur_dir}/batch.sh
log_dir=${cur_dir}/output

grp_id=(`id -nG`)
name=$(hostname)
if [[ "${name}" == *"ofp"* ]]; then
	rscgrp="regular-flat"
else
	rscgrp="eap-large"
fi

if [ ! -d ${log_dir} ]; then
	mkdir -p ${log_dir}
fi

px=$1
py=$2
pz=$3

psx=$4
psy=$5
psz=$6

nproc=$(($psx*$psy*$psz))

cat <<- EOF > ${batch_script}
#!/bin/sh

#PJM -L "node=$((nproc*2))"
#PJM -L "rscgrp=${rscgrp}"
#PJM -L "elapse=00:60:00"
#PJM -g ${grp_id[-1]}
#PJM -S
#PJM --spath ${log_dir}/%n.%j.stat
#PJM -o ${log_dir}/%n.%j.out
#PJM -e ${log_dir}/%n.%j.err
#	PJM --mpi "proc=${nproc}"
#PJM --mpi "max-proc-per-node=1"

sh ${cur_dir}/run.sh $px $py $pz $psx $psy $psz

EOF

pjsub ${batch_script}
