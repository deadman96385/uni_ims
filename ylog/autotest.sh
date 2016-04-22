#!/bin/bash
# Copyright (C) 2016 Spreadtrum
# Created on Mar 25, 2016

rfuncs=(
"ylog_check_ylog_cli_print2kernel"
"ylog_check_ylog_service"
"ylog_check_kernel_log"
"ylog_check_android_log"
"ylog_check_wcn_log"
"ylog_check_wcdma_log"
"ylog_check_td-scdma_log"
"ylog_check_5mode_log"
"ylog_check_4mode_log"
"ylog_check_3mode_log"
"ylog_check_ylog_cli_at"
)

# 函数：ylog_loop_start_stop()
# 参数：无参数
# 功能：压力测试ylog_cli接口, 可以打开多个terminal, 并行执行该操作
# 历史：
# 1. 创建函数 - 2016.04.21 by luther
function ylog_loop_start_stop() #                                  #start & stop ylog one by one to have a stress test
{
    local y=""
    local ylog_service=(
    "android_main"
    "android_system"
    "android_radio"
    "android_events"
    "android_crash"
    "tcpdump"
    "hcidump"
    )
    while [ 1 ]; do
        for y in ${ylog_service[@]}; do
            $ADB_SHELL ylog_cli ylog ${y} start
            $ADB_SHELL ylog_cli ylog ${y} stop
        done
    done
}

# 函数：ylog_check_ylog_cli_print2kernel()
# 参数：无参数
# 功能：检查ylog_cli print2kernel 8888888 命令,dmesg |grep 'print2kerne 8888888' 返回内容应包含print2kerne 8888888
# 历史：
# 1. 创建函数 - 2016.03.26 by yanli
function ylog_check_ylog_cli_print2kernel() #                      #check ylog_cli print2kernel cmd
{
    $ADB_SHELL ylog_cli print2kernel 8888888
    [ "$($ADB_SHELL dmesg | grep 'print2kernel 8888888')" ] && result="pass"
}

# 函数：ylog_check_ylog_service()
# 参数：无参数
# 功能：检查at命令发送
# 历史：
# 1. 创建函数 - 2016.03.25 by luther
function ylog_check_ylog_service() #                               #check ylog service running or not
{
    [ "$($ADB_SHELL ps |grep /system/bin/ylog)" ] && result="pass"
}

# 函数：ylog_check_kernel_log()
# 参数：无参数
# 功能：检查kernel log是否正常捕获
# 历史：
# 1. 创建函数 - 2016.03.25 by luther
function ylog_check_kernel_log() #                                 #check kernel log capture
{
#timeout 5 bash -x <<__EOF
timeout 5 bash <<__EOF
    prev_line='$($ADB_SHELL tail -n 1 ${rootdir_ylog}/kernel/000)'
    while [ 1 ]; do
        $ADB_SHELL ylog_cli print2kernel autotest >/dev/null
        $ADB_SHELL ylog_cli flush >/dev/null
        curr_line="\$($ADB_SHELL tail -n 1 ${rootdir_ylog}/kernel/000)"
        if [ "\${prev_line}" != "\${curr_line}" ]; then
            #echo "1111=\${prev_line}"
            #echo "2222=\${curr_line}"
            exit 0
        fi
        sleep 0.2
    done
__EOF
    [ "$?" == "0" ] && result="pass"
}

# 函数：ylog_check_android_log()
# 参数：无参数
# 功能：检查andorid log是否正常捕获
# 历史：
# 1. 创建函数 - 2016.03.25 by luther
function ylog_check_android_log() #                                #check android log capture
{
#timeout 5 bash -x <<__EOF
timeout 5 bash <<__EOF
    prev_line='$($ADB_SHELL tail -n 1 ${rootdir_ylog}/android/000)'
    while [ 1 ]; do
        $ADB_SHELL ylog_cli print2android autotest >/dev/null
        $ADB_SHELL ylog_cli flush >/dev/null >/dev/null
        curr_line="\$($ADB_SHELL tail -n 1 ${rootdir_ylog}/android/000)"
        if [ "\${prev_line}" != "\${curr_line}" ]; then
            #echo "1111=\${prev_line}"
            #echo "2222=\${curr_line}"
            exit 0
        fi
        sleep 0.2
    done
__EOF
    [ "$?" == "0" ] && result="pass"
}

# 函数：ylog_check_wcn_log()
# 参数：无参数
# 功能：检查wcn log是否正常捕获
# 历史：
# 1. 创建函数 - 2016.03.25 by luther
function ylog_check_wcn_log() #                                    #check wcn log capture
{
    [ "`$ADB_SHELL \"[ -e ${rootdir_ylog}/cp/wcn/000 ] && echo 1\"`" ] && result="pass"
}

# 函数：ylog_check_wcdma_log()
# 参数：无参数
# 功能：检查wcdma log是否正常捕获
# 历史：
# 1. 创建函数 - 2016.03.25 by luther
function ylog_check_wcdma_log() #                                  #check wcdma log capture
{
    [ "`$ADB_SHELL \"[ -e ${rootdir_ylog}/cp/wcdma/000 ] && echo 1\"`" ] && result="pass"
}

# 函数：ylog_check_td-scdma_log()
# 参数：无参数
# 功能：检查td-scdma log是否正常捕获
# 历史：
# 1. 创建函数 - 2016.03.25 by luther
function ylog_check_td-scdma_log() #                               #check td-scdma log capture
{
    [ "`$ADB_SHELL \"[ -e ${rootdir_ylog}/cp/td-scdma/000 ] && echo 1\"`" ] && result="pass"
}

# 函数：ylog_check_5mode_log()
# 参数：无参数
# 功能：检查5mode log是否正常捕获
# 历史：
# 1. 创建函数 - 2016.03.25 by luther
function ylog_check_5mode_log() #                                  #check 5mode log capture
{
    [ "`$ADB_SHELL \"[ -e ${rootdir_ylog}/cp/5mode/000 ] && echo 1\"`" ] && result="pass"
}

# 函数：ylog_check_4mode_log()
# 参数：无参数
# 功能：检查4mode log是否正常捕获
# 历史：
# 1. 创建函数 - 2016.03.25 by luther
function ylog_check_4mode_log() #                                  #check 4mode log capture
{
    [ "`$ADB_SHELL \"[ -e ${rootdir_ylog}/cp/4mode/000 ] && echo 1\"`" ] && result="pass"
}

# 函数：ylog_check_3mode_log()
# 参数：无参数
# 功能：检查3mode log是否正常捕获
# 历史：
# 1. 创建函数 - 2016.03.25 by luther
function ylog_check_3mode_log() #                                  #check 3mode log capture
{
    [ "`$ADB_SHELL \"[ -e ${rootdir_ylog}/cp/3mode/000 ] && echo 1\"`" ] && result="pass"
}

# 函数：ylog_check_ylog_cli_at()
# 参数：无参数
# 功能：检查at命令发送
# 历史：
# 1. 创建函数 - 2016.03.25 by luther
function ylog_check_ylog_cli_at() #                                #check at using ylog_cli at AT+CGMI
{
    [ "$($ADB_SHELL ylog_cli at AT+CGMI | grep -i fail)" ] || result="pass"
}

if [ -x /system/bin/sh ]; then
    android=1
    BUSYBOX='busybox '
    ADB='eval #'
    ADB_SHELL=eval
    AWK=gawk
else
    android=0
    BUSYBOX=
    [ "`which gawk`" ] || sudo apt-get install gawk
    AWK=gawk
    PYTHON=python
fi

BASH=/bin/bash
SED=${BUSYBOX}sed
GREP=${BUSYBOX}grep
EGREP=${BUSYBOX}egrep
EXPR=${BUSYBOX}expr
BASENAME=${BUSYBOX}basename
CUT=${BUSYBOX}cut
SORT=${BUSYBOX}sort
UNIQ=${BUSYBOX}uniq
TR=${BUSYBOX}tr
MKTEMP=${BUSYBOX}mktemp
TEE=${BUSYBOX}tee
FIND=${BUSYBOX}find
STRINGS=${BUSYBOX}strings
XARGS=${BUSYBOX}xargs

program="$0"

COLOR_START_RED='\E[1;31m'
COLOR_START_GREEN='\E[1;32m'
COLOR_START_BLUE='\E[1;34m'
COLOR_START_PINK='\E[1;35m'
COLOR_START_YELOW='\E[1;33m'
COLOR_END='\E[0m'

 function my_echo_normal() {
echo "$@"
}
 function my_echo_red() {
echo -e "${COLOR_START_RED}$@${COLOR_END}"
}

 function my_echo_yellow() {
echo -e "${COLOR_START_YELOW}$@${COLOR_END}"
}

 function my_echo_green() {
echo -e "${COLOR_START_GREEN}$@${COLOR_END}"
}

 function my_echo_pink() {
echo -e "${COLOR_START_PINK}$@${COLOR_END}"
}

if [ '1' ]; then
    if [ "$1" == "all" ]; then
        #rfuncs=(`${GREP} '^function ' ${program} | ${SED} 's/^function //;s/(.*//'`)
        echo > /dev/null
    else
        rfuncs=($@)
    fi
    if [ "${rfuncs[0]}" ]; then
        devices=($(adb devices -l | sed '/device:/!d;s/.*device \(.*\) product:.*/\1/g'))
        pcount=0
        for d in ${devices[@]}; do
            serial=$(adb devices -l | sed "/${d}/!d" | cut -d' ' -f 1)
            ((pcount++))
            echo "------- ${pcount} device ${d} , ${serial}-------"
            count=1
            export ADB="adb -s ${d}"
            export ADB_SHELL="${ADB} shell"
            export rootdir="$(${ADB} shell ylog_cli rootdir | tr -d '\r')"
            result="fail"
            ylog_check_ylog_service
            [ "${result}" != "pass" ] && rootdir="/"
            export rootdir_ylog="${rootdir}/ylog/ylog"
            for f in "${rfuncs[@]}"; do
                result="fail"
                eval ${f}
                if [ "${result}" != "pass" ]; then
                    printf "%03d. " ${count}
                    my_echo_red "${f} --> fail"
                else
                    printf "%03d. " ${count}
                    my_echo_normal "${f} --> pass"
                fi
                ((count++))
            done
        done
    else
        ${GREP} '^function ' ${program} | ${SED} 's/^function //;s/()//;s/#//' | ${AWK} '{printf "%03d. %s\n", NR, $0}'
    fi
else
    if [ "$1" ]; then
        funcs=(`${GREP} '^function ' ${program} | ${SED} 's/^function //;s/(.*//'`)
        func=()
        for i in ${funcs[@]}; do
            if [ "$1" == "${i}" ]; then
                func=${i}
                break
            fi
        done
        [ "${func[@]}" ] || {
            func=(`echo "${funcs[@]}" | ${SED} 's/ /\n/g' | ${EGREP} "$1"`)
        }
        if ((${#func[@]} != 1)); then
            if [ "${func[0]}" ]; then
                echo "Can't run ->"
                echo "${func[@]}" | ${AWK} 'BEGIN {RS=" "} {printf "%03d. %s\n", NR, $0}'
            else
                echo "Can't run -> $@"
            fi
            exit
        fi
        shift
        eval ${func[0]} $@
    else
        ${GREP} '^function ' ${program} | ${SED} 's/^function //;s/()//;s/#//' | ${AWK} '{printf "%03d. %s\n", NR, $0}'
    fi
fi
