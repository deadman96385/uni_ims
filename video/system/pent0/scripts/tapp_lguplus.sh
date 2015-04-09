#!/bin/sh
# remove the old results
rm -rf tapp_malibu_lguplus*result.*
rm smd0

rm -r queues
mkdir queues
# copy lgu+ gapp xml
cp gapp_tapp_lguplus.xml gapp.xml
# malibu LGU+ testsuite
./csm_ut_tapp ./queues tapp_lguplus.xml ../csm/ut/tapp/testsuites/malibu_extdial/provider/lguplus/tc_all.xml

echo "\n***** TEST RESULT *****"
echo "malibu LGU+ test result : tapp_malibu_lguplus_result.txt"
tail -n3 tapp_malibu_lguplus_result.txt
