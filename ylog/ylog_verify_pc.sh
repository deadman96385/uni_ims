#!/bin/bash

default_run=(1)
cmds=(
'report_summary'
'[ -d ${y}/traces/ ] && ls -l ${y}/traces/'
'du -shc ${y}/* | sort -h'
'tail -n 1 ${y}/android/000'
'tail -n 1 ${y}/kernel/000'
'grep -E "Has run|killed" ${y}/ylog_debug | tail -n 2'
)

function report_summary() {
    eval '[ -d ${y}/traces/ ] && { echo "ls -l ${y}/traces/"; ls -l ${y}/traces/; }'
    eval 'echo'
    eval 'echo "du -shc ${y}/* | sort -h"; du -shc ${y}/* | sort -h'
    eval 'echo'
    eval 'cat ${y}/ylog_debug | grep -E "Has run|killed" | tail -n 2 |
        sed "s/.*Has run \(.*\) avg_speed.*/\1/;s/.*] \[/[/" | paste -s' # | sed "s/\(.*\) \[\(.*\)/[\2 \1/"'
    eval 'tail -n 1 ${y}/android/000 | cut -d" " -f1,2 | sed "s/..\(.*\)/      android - [\1]/"'
    eval 'tail -n 1 ${y}/kernel/000 | cut -d" " -f1,2 | sed "s/^/       kernel - /"'
    eval 'echo'
}

function usage() {
cat <<__AEOF
ylog_verfiy_pc.sh [command index1] [command index2] ...
-h for help
-l list all supported commands, the prefix is command index number
-c command, ex: -c 'ls -l \$y'
-C only execute command, ex: -C 'ls -l \$y'
__AEOF
}

while getopts hlr:c:C: op; do
    case $op in
        l)
            list_cmd=1; ;;
        r)
            runs=(${runs[@]} $OPTARG); ;;
        c)
            sub_cmds=("${sub_cmds[@]}" "$OPTARG"); ;;
        C)
            o_sub_cmds=("${o_sub_cmds[@]}" "$OPTARG"); ;;
        h)
            usage;
            exit 1;
        ;;
    esac
done

shift `expr $OPTIND - 1`
runs=($@)
[ "${runs[0]}" ] || {
    [ "${default_run[0]}" ] && runs=(${default_run[@]})
}

# [ "${runs[0]}${o_sub_cmds[0]}${sub_cmds[0]}" ] || list_cmd=1
[ "${list_cmd}" ] && {
    count=1
    for c in "${cmds[@]}"; do
        echo "${count}. ${c}"
        ((count++))
    done
    exit
}

if [ "${runs[0]}" ]; then
    do_cmds=()
    for r in ${runs[@]}; do
        count=1
        for c in "${cmds[@]}"; do
            if ((${r} == ${count})); then
                do_cmds=("${do_cmds[@]}" "${c}")
            fi
            ((count++))
        done
    done
else
    do_cmds=("${cmds[@]}")
fi

if [ "${o_sub_cmds[0]}" ]; then
    do_cmds=("${o_sub_cmds[@]}")
else
    do_cmds=("${do_cmds[@]}" "${sub_cmds[@]}")
fi

[ '' ] && {
    count=1
    echo "${do_cmds[@]}"
    for c in "${do_cmds[@]}"; do
        echo "${count}. [ ${c} ]"
        ((count++))
    done
}

ylog_log_folder=("`find -name ylog_debug | sort`")

count=1
for y in ${ylog_log_folder[@]}; do
    y=$(dirname ${y})
    echo "------- folder ${y} -------"
    for c in "${do_cmds[@]}"; do
        echo; echo;
        eval "echo \"${count}. [ $(echo ${c} | sed 's/"/\\"/g') ]\""
        eval "${c}"
    done
    ((count++))
done
