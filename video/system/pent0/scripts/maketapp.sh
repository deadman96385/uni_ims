#!/bin/sh

# Build vPort for generic provider
export PLATFORM=pent0
export PRODUCT=malibu
export PROVIDER=PROVIDER_GENERIC

# Build vPort for generic provider
echo "build tapp for provider:$PROVIDER" 

make clean
make TAPP=y PROVIDER=$PROVIDER

# Run test
cd build.pent0
./tapp_generic.sh
cd ..

# Build vPort for LGU+
export PLATFORM=pent0
export PRODUCT=malibu
export PROVIDER=PROVIDER_LGUPLUS
echo "build tapp for provider:$PROVIDER" 
make clean
make TAPP=y PROVIDER=$PROVIDER

# Run test
cd build.pent0
./tapp_lguplus.sh

# Build vPort for CMCC
export PLATFORM=pent0
export PRODUCT=malibu
export PROVIDER=PROVIDER_CMCC
echo "build tapp for provider:$PROVIDER" 
make clean
make TAPP=y PROVIDER=$PROVIDER

# Run test
cd build.pent0
./tapp_cmcc.sh

# Merge xml report
rm tapp_malibu_all_result.xml
echo '<?xml version="1.0" encoding = "UTF-8"?>'  >> tapp_malibu_all_result.xml
echo '<testsuites name="test">'  >> tapp_malibu_all_result.xml
echo '' >> tapp_malibu_all_result.xml
cat tapp_malibu_generic_result.xml >> tapp_malibu_all_result.xml
echo '' >> tapp_malibu_all_result.xml
cat tapp_malibu_config_1_result.xml >> tapp_malibu_all_result.xml
echo '' >> tapp_malibu_all_result.xml
cat tapp_malibu_default_provisioning_result.xml  >> tapp_malibu_all_result.xml
echo '' >> tapp_malibu_all_result.xml
cat tapp_malibu_config_1_provisioning_result.xml >> tapp_malibu_all_result.xml
echo '' >> tapp_malibu_all_result.xml
cat tapp_malibu_extdial_result.xml >> tapp_malibu_all_result.xml
echo '' >> tapp_malibu_all_result.xml
cat tapp_malibu_lguplus_result.xml >> tapp_malibu_all_result.xml
echo '' >> tapp_malibu_all_result.xml
cat tapp_malibu_cmcc_result.xml >> tapp_malibu_all_result.xml
echo '' >> tapp_malibu_all_result.xml
echo '</testsuites>' >> tapp_malibu_all_result.xml

# Show quick test result
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
echo ""
echo "malibu LGU+ test result : tapp_malibu_lguplus_result.txt"
tail -n3 tapp_malibu_lguplus_result.txt
echo ""
echo "malibu CMCC test result : tapp_malibu_cmcc_result.txt"
tail -n3 tapp_malibu_cmcc_result.txt

# go to previous directory
cd ..
