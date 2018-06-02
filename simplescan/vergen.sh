ver=$(date)
echo "// auto generated data version" > version.h
echo "const char* version_date = \"$ver\";" >> version.h
cp -u -v version.h ~/workspace/low-powered-doorbell-fw/ambarella/ring/ble
cp -u -v version.h ~/workspace/chime_pro-firmware/app/ble/



