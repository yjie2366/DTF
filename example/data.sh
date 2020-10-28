#!/bin/sh

cur_dir=$(cd $(dirname ${BASH_SOURCE[0]}) > /dev/null && pwd)
log_dir=${cur_dir}/output

for np in 32 64 128 256 512 1024; do
exist="`find ${log_dir} -name "${np}-*" -type d`"
if [ -n "${exist}" ]; then
	echo "DIR: $exist"
	times=$(echo $exist | wc -w)
	times=$((times*5))
	for n in "writer" "reader"; do
		grep "^${n}.*avg transfer time" ${log_dir}/${np}-*/batch.sh.*.out | awk -v t=${times} '{total+=$10} END{print total/t}'
	done
fi
done
