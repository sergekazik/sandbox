cd ~/workspace/sandbox/simplescan
make all clean_obj
cp *.cpp *.c *.h *.hh -v $HOME/workspace/low-powered-doorbell-fw/ambarella/ring/ble/.

#cd ~/workspace/low-powered-doorbell-fw
#source build.env
#protoc --version | awk '{print $2}'
#PROTOBUF_VER=$( protoc --version | awk '{print $2}' )
#protoc --version
#pkg-config --modversion protobuf
#@echo $(PROTOBUF_VER)
#make RING_NAME=hpcam2 BUILD_VER=133_0_1


