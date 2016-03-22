#!/bin/bash

default_run=(1)
cmds=(
'report_summary'
'${ADB} shell ls -l ${rootdir}/ylog/ylog/'
'${ADB} shell ylog_cli space'
'${ADB} shell ylog_cli ylog'
'${ADB} shell tail ${rootdir}/ylog/ylog/ylog_journal_file'
'${ADB} shell tail -n 1 ${rootdir}/ylog/ylog/android/000'
'${ADB} shell tail -n 1 ${rootdir}/ylog/ylog/kernel/000'
'${ADB} shell cat ${rootdir}/ylog/ylog/ylog_debug | grep -E "Has run|killed" | tail -n 2'
)

function report_summary() {
    ylog_debug="${rootdir}/ylog/ylog/ylog_debug"
    ylog_android="${rootdir}/ylog/ylog/android/000"
    ylog_kernel="${rootdir}/ylog/ylog/kernel/000"
    ylog_traces="${rootdir}/ylog/ylog/traces/"

    eval '${ADB} shell "[ -d ${ylog_traces} ] && { echo \"ls -l ${ylog_traces}\"; ls -l ${ylog_traces}; }"'
    eval 'echo'
    eval '${ADB} shell ylog_cli space'
    eval 'echo'
    eval '${ADB} shell cat ${ylog_debug} | grep -E "Has run|killed" | tail -n 2 |
        sed "s/.*Has run \(.*\) avg_speed.*/\1/;s/.*] \[/[/" | paste -s' # | sed "s/\(.*\) \[\(.*\)/[\2 \1/"'
    eval '${ADB} shell ylog_cli speed | grep -E "Has run|killed" | tail -n 2 |
        sed "s/.*Has run \(.*\) avg_speed.*/\1/;s/.*] \[/[/" | paste -s' # | sed "s/\(.*\) \[\(.*\)/[\2 \1/"'
    eval '${ADB} shell tail -n 1 ${ylog_android} | cut -d" " -f1,2 | sed "s/..\(.*\)/      android - [\1]/"'
    eval '${ADB} shell tail -n 1 ${ylog_kernel} | cut -d" " -f1,2 | sed "s/^/       kernel - /"'
    eval 'echo'
}

function usage() {
cat <<__AEOF
ylog_verfiy.sh [command index1] [command index2] ...
-h for help
-l list all supported commands, the prefix is command index number
-c shell command, ex: -c 'ls /'
-C only execute shell command, ex: -C 'ls /'
__AEOF
}

while getopts hlr:c:C: op; do
    case $op in
        l)
            list_cmd=1; ;;
        r)
            runs=(${runs[@]} $OPTARG); ;;
        c)
            sub_cmds=("${sub_cmds[@]}" "\${ADB} shell $OPTARG"); ;;
        C)
            o_sub_cmds=("${o_sub_cmds[@]}" "\${ADB} shell $OPTARG"); ;;
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

adb devices -l | sed '/device:/!d'
devices=($(adb devices -l | sed '/device:/!d;s/.*device \(.*\) product:.*/\1/g'))

count=0
for d in ${devices[@]}; do
    serial=$(adb devices -l | sed "/${d}/!d" | cut -d' ' -f 1)
    ((count++))
    echo "------- device ${d} , ${serial}-------"
    for c in "${do_cmds[@]}"; do
        echo; echo;
        ADB="adb -s ${d}"
        rootdir="$(${ADB} shell ylog_cli rootdir | tr -d '\r')"
        eval "echo \"${count}. [ $(echo ${c} | sed 's/"/\\"/g') ] on device ${serial}\""
        eval "${c}"
    done
done
