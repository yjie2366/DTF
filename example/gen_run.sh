#!/bin/sh
cur_dir=$(cd $(dirname ${BASH_SOURCE[0]}) > /dev/null && pwd)
batch_script=${cur_dir}/batch.sh
log_dir=${cur_dir}/output

if [ ! -d ${log_dir} ]; then
	mkdir -p ${log_dir}
fi

px=40
py=40
pz=20

psx=2
psy=2
psz=1
nproc=$(($psx*$psy*$psz))

cat <<- EOF > ${batch_script}
#!/bin/sh

#PJM -L "node=$((nproc*2))"
#PJM -L "rscgrp=eap-small"
#PJM -L "elapse=00:60:00"
#PJM -g g9300001
#PJM -S
#PJM --spath ${log_dir}/%n.%j.stat
#PJM -o ${log_dir}/%n.%j.out
#PJM -e ${log_dir}/%n.%j.err
#	PJM --mpi "proc=${nproc}"
#PJM --mpi "max-proc-per-node=1"

sh ${cur_dir}/run.sh $px $py $pz $psx $psy $psz

EOF

pjsub ${batch_script}
