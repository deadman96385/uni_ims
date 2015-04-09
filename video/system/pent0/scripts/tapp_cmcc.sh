#!/bin/sh
# remove the old results
rm -rf tapp_malibu_cmcc*result.*
rm smd0

rm -r queues
mkdir queues
# copy cmcc gapp xml
cp gapp_tapp_cmcc.xml gapp.xml

# malibu cmcc testsuite
./csm_ut_tapp ./queues tapp_cmcc.xml ../csm/ut/tapp/testsuites/malibu_extdial/provider/cmcc/tc_all.xml

echo "\n***** TEST RESULT *****"
echo "malibu cmcc test result : tapp_malibu_cmcc_result.txt"
tail -n3 tapp_malibu_cmcc_result.txt
