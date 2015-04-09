#!/bin/sh
# remove the old results
rm -rf tapp_malibu2_*_result.*
rm smd0

rm -r queues
mkdir queues
# default tapp test. malibu testsuite
cp ./gapp_tapp.xml ./gapp.xml
./csm_ut_tapp ./queues tapp.xml ../csm/ut/tapp/testsuites/malibu/tc_all.xml
#
# Test case with: 1. <parm name="NatUrlFmt" value="0"/>
#                    <parm name="IntUrlFmt" value="0"/>
#                 2. <parm name="psSignalling" value="SIPoUDP"/>
#                 3. <parm name="psRTMedia" value="SRTP"/>
./csm_ut_tapp ./queues tapp_config_1.xml ../csm/ut/tapp/testsuites/malibu/provisioning_setting/tc_ps_call_with_tel_udp_srtp.xml

# enabled proviosioning data with default value
./csm_ut_tapp ./queues tapp_enable_provisioning.xml ../csm/ut/tapp/testsuites/malibu/provisioning_setting/tc_enable_provisioning_all.xml

# enabled proviosioning data with : 1. <parm name="NatUrlFmt" value="0"/>
#                                      <parm name="IntUrlFmt" value="0"/>
#                                   2. <parm name="psSignalling" value="SIPoUDP"/>
#                                   3. <parm name="psRTMedia" value="SRTP"/>
./csm_ut_tapp ./queues tapp_enable_pv_with_config1.xml  ../csm/ut/tapp/testsuites/malibu/provisioning_setting/tc_enable_provisioning_all_with_config1.xml

# extdial testing
cp ./gapp_extdial.xml ./gapp.xml
./csm_ut_tapp ./queues tapp_extdial.xml ../csm/ut/tapp/testsuites/malibu_extdial/tc_all.xml

echo "\n***** TEST RESULT *****"
echo "malibu generic test result : tapp_malibu_generic_result.txt"
tail -n3 tapp_malibu_generic_result.txt
echo ""
echo "malibu generic configuration 1 test result : tapp_malibu_config_1_result.txt"
tail -n3 tapp_malibu_config_1_result.txt
echo ""
echo "Enable provisioning data with default setting test result : tapp_malibu_default_provisioning_result.txt"
tail -n3 tapp_malibu_default_provisioning_result.txt
echo ""
echo "Enable provisioning data with config1 setting test result : tapp_malibu_config_1_provisioning_result.txt"
tail -n3 tapp_malibu_config_1_provisioning_result.txt
echo ""
echo "malibu_extdial test result : tapp_malibu_extdial_result.txt"
tail -n3 tapp_malibu_extdial_result.txt 

