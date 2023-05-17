/**********
This library is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the
Free Software Foundation; either version 3 of the License, or (at your
option) any later version. (See <http://www.gnu.org/copyleft/lesser.html>.)

This library is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
more details.

You should have received a copy of the GNU Lesser General Public License
along with this library; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
**********/
// "liveMedia"
// Copyright (c) 1996-2022 Live Networks, Inc.  All rights reserved.
// IMX6 Sinks
// C++ header

#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <sys/un.h>
#include <unistd.h>

#ifndef _IMX6_SINK_HH
#define _IMX6_SINK_HH

#ifndef _MEDIA_SINK_HH
#include "MediaSink.hh"
#endif

#define IPC_DATA_SHM "/ipc_data_temp"
#define kPORT_VIDEO_IPC_LIVE_UDP  "/tmp/UDN_UDP_MSG2VIDEO_LIVE"
#define IPC_DATA_BLOCK_SIZE  1024000
#define IPC_DATA_BLOCK_NUM 20


class Imx6H264Sink: public MediaSink {
public:
  static Imx6H264Sink* createNew(UsageEnvironment& env,
                        char const* sPropParameterSetsStr1,
                        char const* sPropParameterSetsStr2,
                        char const* sPropParameterSetsStr3,
			            unsigned bufferSize = 20000);


protected:
  Imx6H264Sink(UsageEnvironment& env,unsigned bufferSize,
                        char const* sPropParameterSetsStr1,
                        char const* sPropParameterSetsStr2,
                        char const* sPropParameterSetsStr3);
      // called only by createNew()
  virtual ~Imx6H264Sink();

protected: // redefined virtual functions:
  virtual Boolean continuePlaying();

protected:
  static void afterGettingFrame(void* clientData, unsigned frameSize,
				unsigned numTruncatedBytes,
				struct timeval presentationTime,
				unsigned durationInMicroseconds);
  virtual void afterGettingFrame(unsigned frameSize,
				 unsigned numTruncatedBytes,
				 struct timeval presentationTime);

  void forwordRtp(unsigned frameSize, unsigned long long rtpTimestamp);
  void cus_memcpy(unsigned char const** base, unsigned *_dataSize,unsigned char* buff, int length);

  unsigned char* fBuffer;
  unsigned fBufferSize;
  struct timeval fPrevPresentationTime;
  unsigned fSamePresentationTimeCounter;

private:
  char const* fSPropParameterSetsStr[3];
  Boolean fHaveWrittenFirstFrame;
  int  s;
  int slen;
  struct sockaddr_un serveraddr;
  int seqno;
  unsigned frameid;
 
  unsigned sps_length = 0;
  unsigned pps_length = 0;
  unsigned char *sps_arr;
  unsigned char *pps_arr;
  int shm_fd;        // file descriptor, from shm_open()
  unsigned char *shm_base;    // base address, from mmap()
};

#endif
