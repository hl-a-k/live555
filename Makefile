##### Change the following for your environment:
CROSS_COMPILE?=		/opt/fsl-imx-fb/4.14-sumo/sysroots/x86_64-pokysdk-linux/usr/bin/arm-poky-linux-gnueabi/arm-poky-linux-gnueabi-
COMPILE_OPTS =		$(INCLUDES)  -I. -O2 -DSOCKLEN_T=socklen_t -DNO_SSTREAM=1 -D_LARGEFILE_SOURCE=1 -D_FILE_OFFSET_BITS=64 -march=armv7-a -mfpu=neon -mfloat-abi=hard -mcpu=cortex-a9  -I ../../third_party/src/library/openssl/openssl-1.1.1j/include --sysroot=/opt/fsl-imx-fb/4.14-sumo/sysroots/cortexa9hf-neon-poky-linux-gnueabi
C =			c
C_COMPILER =		$(CROSS_COMPILE)gcc
C_FLAGS =		$(COMPILE_OPTS)
CPP =			cpp
CPLUSPLUS_COMPILER =	$(CROSS_COMPILE)g++
CPLUSPLUS_FLAGS =	$(COMPILE_OPTS) -Wall -DBSD=1
OBJ =			o
LINK =			$(CROSS_COMPILE)g++ -march=armv7-a -mfpu=neon -mfloat-abi=hard -mcpu=cortex-a9 -L../../third_party/src/library/openssl/openssl-1.1.1j --sysroot=/opt/fsl-imx-fb/4.14-sumo/sysroots/cortexa9hf-neon-poky-linux-gnueabi  -o 
LINK_OPTS =		
CONSOLE_LINK_OPTS =	$(LINK_OPTS)
LIBRARY_LINK =		$(CROSS_COMPILE)ar cr 
LIBRARY_LINK_OPTS =	$(LINK_OPTS)
LIB_SUFFIX =			a
LIBS_FOR_CONSOLE_APPLICATION = -lssl -lcrypto
LIBS_FOR_GUI_APPLICATION =
EXE =
##### End of variables to change

LIVEMEDIA_DIR = liveMedia
GROUPSOCK_DIR = groupsock
USAGE_ENVIRONMENT_DIR = UsageEnvironment
BASIC_USAGE_ENVIRONMENT_DIR = BasicUsageEnvironment

TESTPROGS_DIR = testProgs

MEDIA_SERVER_DIR = mediaServer

PROXY_SERVER_DIR = proxyServer

HLS_PROXY_DIR = hlsProxy
TARGET=testRTSPClient

all:
	cd $(LIVEMEDIA_DIR) ; $(MAKE)
	cd $(GROUPSOCK_DIR) ; $(MAKE)
	cd $(USAGE_ENVIRONMENT_DIR) ; $(MAKE)
	cd $(BASIC_USAGE_ENVIRONMENT_DIR) ; $(MAKE)
	cd $(TESTPROGS_DIR) ; $(MAKE)
	cd $(MEDIA_SERVER_DIR) ; $(MAKE)
	cd $(PROXY_SERVER_DIR) ; $(MAKE)
	cd $(HLS_PROXY_DIR) ; $(MAKE)
	@echo
	@echo "For more information about this source code (including your obligations under the LGPL), please see our FAQ at http://live555.com/liveMedia/faq.html"

install:
	cd $(LIVEMEDIA_DIR) ; $(MAKE) install
	cd $(GROUPSOCK_DIR) ; $(MAKE) install
	cd $(USAGE_ENVIRONMENT_DIR) ; $(MAKE) install
	cd $(BASIC_USAGE_ENVIRONMENT_DIR) ; $(MAKE) install
	cd $(TESTPROGS_DIR) ; $(MAKE) install
	cd $(MEDIA_SERVER_DIR) ; $(MAKE) install
	cd $(PROXY_SERVER_DIR) ; $(MAKE) install
	cd $(HLS_PROXY_DIR) ; $(MAKE) install

clean:
	cd $(LIVEMEDIA_DIR) ; $(MAKE) clean
	cd $(GROUPSOCK_DIR) ; $(MAKE) clean
	cd $(USAGE_ENVIRONMENT_DIR) ; $(MAKE) clean
	cd $(BASIC_USAGE_ENVIRONMENT_DIR) ; $(MAKE) clean
	cd $(TESTPROGS_DIR) ; $(MAKE) clean
	cd $(MEDIA_SERVER_DIR) ; $(MAKE) clean
	cd $(PROXY_SERVER_DIR) ; $(MAKE) clean
	cd $(HLS_PROXY_DIR) ; $(MAKE) clean

distclean: clean
	-rm -f $(LIVEMEDIA_DIR)/Makefile $(GROUPSOCK_DIR)/Makefile \
	  $(USAGE_ENVIRONMENT_DIR)/Makefile $(BASIC_USAGE_ENVIRONMENT_DIR)/Makefile \
	  $(TESTPROGS_DIR)/Makefile $(MEDIA_SERVER_DIR)/Makefile \
	  $(PROXY_SERVER_DIR)/Makefile \
	  $(HLS_PROXY_DIR)/Makefile \
	  Makefile


interface:
	@echo no interface needed for $(TARGET)

interface-clean:
	@echo no interface-clean needed for $(TARGET)

romfs: all
	@echo install $(TARGET) to $(ROOTFS_DIR)/usr/app/
	$(MKDIR) $(ROOTFS_DIR)/usr/app/
	$(CP) testProgs/$(TARGET) $(ROOTFS_DIR)/usr/app/

.PHONY: all clean interface interface-clean romfs
