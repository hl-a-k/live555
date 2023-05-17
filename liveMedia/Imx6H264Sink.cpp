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
// File sinks
// Implementation

#if (defined(__WIN32__) || defined(_WIN32)) && !defined(_WIN32_WCE)
#include <io.h>
#include <fcntl.h>
#endif
#include "Imx6H264Sink.hh"
#include "GroupsockHelper.hh"
#include "H264VideoRTPSource.hh" // for "parseSPropParameterSets()"

#include <fcntl.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <errno.h>

struct nethdr {
	int seqno;
	int iframe;
	int len;
};

////////// Imx6H264Sink //////////

Imx6H264Sink::Imx6H264Sink(UsageEnvironment& env, unsigned bufferSize,
                        char const* sPropParameterSetsStr1,
                        char const* sPropParameterSetsStr2,
                        char const* sPropParameterSetsStr3
           )
  : MediaSink(env),  fBufferSize(bufferSize), fSamePresentationTimeCounter(0),seqno(0),
   fHaveWrittenFirstFrame(False) , sps_length(0), pps_length(0)
   {

  fSPropParameterSetsStr[0] = strDup(sPropParameterSetsStr1);
  fSPropParameterSetsStr[1] = strDup(sPropParameterSetsStr2);
  fSPropParameterSetsStr[2] = strDup(sPropParameterSetsStr3);


  fBuffer = new unsigned char[bufferSize];
  fPrevPresentationTime.tv_sec = ~0; fPrevPresentationTime.tv_usec = 0;

  s=socket(AF_UNIX,SOCK_DGRAM,0);

    memset(&serveraddr,0,sizeof(serveraddr));
    serveraddr.sun_family = AF_UNIX;
    strcpy(serveraddr.sun_path, kPORT_VIDEO_IPC_LIVE_UDP);


   slen=sizeof(serveraddr);

   frameid = 0;
   shm_fd = shm_open(IPC_DATA_SHM, O_RDWR , 0666);
    if (shm_fd == -1) {
        env<< "cons: Shared memory failed: " << strerror(errno) << "\n";
        exit(1);
    }

    shm_base = (unsigned char*) mmap(0, IPC_DATA_BLOCK_SIZE * IPC_DATA_BLOCK_NUM + 1, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm_base == MAP_FAILED) {
        env << "cons: Map failed:" << strerror(errno) << "\n";
        // close and unlink?
        exit(1);
    }

}

Imx6H264Sink::~Imx6H264Sink() {
    for (unsigned j = 0; j < 3; ++j) delete[] (char*)fSPropParameterSetsStr[j];
    delete[] fBuffer;
    ::closeSocket(s);

    if(sps_length > 0) {
         delete[] sps_arr;;
    }
    if(pps_length > 0) {
         delete[] pps_arr;;
    }

    /* remove the mapped memory segment from the address space of the process */
    if (munmap(shm_base, IPC_DATA_BLOCK_SIZE * IPC_DATA_BLOCK_NUM + 1) == -1) {
        envir() << "prod: Unmap failed: " << strerror(errno) << "\n";
    }
    /* close the shared memory segment as if it was a file */
    else if (::close(shm_fd) == -1) {
        envir() << "prod: Close failed: %s" << strerror(errno) << "\n";
    }

}

Imx6H264Sink* Imx6H264Sink::createNew(UsageEnvironment& env,
                    char const* sPropParameterSetsStr1,
                    char const* sPropParameterSetsStr2,
                    char const* sPropParameterSetsStr3,
			        unsigned bufferSize) {
  do {
    return new Imx6H264Sink(env, bufferSize, sPropParameterSetsStr1, sPropParameterSetsStr2, sPropParameterSetsStr3);
  } while (0);

  return NULL;
}

Boolean Imx6H264Sink::continuePlaying() {
  if (fSource == NULL) return False;

  fSource->getNextFrame(fBuffer, fBufferSize,
			afterGettingFrame, this,
			onSourceClosure, this);

  return True;
}

void Imx6H264Sink::afterGettingFrame(void* clientData, unsigned frameSize,
				 unsigned numTruncatedBytes,
				 struct timeval presentationTime,
				 unsigned /*durationInMicroseconds*/) {
  Imx6H264Sink* sink = (Imx6H264Sink*)clientData;
  sink->afterGettingFrame(frameSize, numTruncatedBytes, presentationTime);
}

void Imx6H264Sink::forwordRtp(unsigned frameSize, unsigned long long rtpTimestamp){
    unsigned char naul_type = fBuffer[0];

     if((naul_type & 0x1f) == 5) { // 关键帧 需要传sps pps
        if(sps_length > 0 && pps_length > 0) {
           
            envir().IMXMPI_TskBufToLocalSocket(sps_arr , sps_length, rtpTimestamp);
            envir().IMXMPI_TskBufToLocalSocket(pps_arr , pps_length, rtpTimestamp);

        } else {
            for (unsigned j = 0; j < 3; ++j) {
                unsigned numSPropRecords;
                SPropRecord* sPropRecords
                = parseSPropParameterSets(fSPropParameterSetsStr[j], numSPropRecords);
                for (unsigned i = 0; i < numSPropRecords; ++i) {
                    if(sPropRecords[i].sPropLength > 0) {
                        envir().IMXMPI_TskBufToLocalSocket(sPropRecords[i].sPropBytes , sPropRecords[i].sPropLength, rtpTimestamp);
                    }
                }
                delete[] sPropRecords;
            }
        }  
    }
    envir().IMXMPI_TskBufToLocalSocket(fBuffer, frameSize, rtpTimestamp);
}

void Imx6H264Sink::cus_memcpy(unsigned char const** base, unsigned *_dataSize,unsigned char* buff, int length) {
    static unsigned char buf_[4] = {0, 0, 0, 1};
    memcpy((void *)(*base), buf_, 4);
    *base += 4;
    *_dataSize += 4;

    memcpy((void *)(*base), buff, length);
    *base += length;
    *_dataSize += length;
}

void Imx6H264Sink::afterGettingFrame(unsigned frameSize,
				 unsigned numTruncatedBytes,
				 struct timeval presentationTime) {

  if (numTruncatedBytes > 0) {
    envir() << "FileSink::afterGettingFrame(): The input frame data was too large for our buffer size ("
	    << fBufferSize << ").  "
            << numTruncatedBytes << " bytes of trailing data was dropped!  Correct this by increasing the \"bufferSize\" parameter in the \"createNew()\" call to at least "
            << fBufferSize + numTruncatedBytes << "\n";
  }
  unsigned char naul_type = fBuffer[0] & 0x1f;
  if(naul_type == 7 && sps_length == 0) { // sps
    sps_length = frameSize;
    sps_arr = new unsigned char[frameSize];
    memcpy((void *)sps_arr, fBuffer, frameSize);
  }
  if(naul_type == 8 && pps_length == 0) { // pps
    pps_length = frameSize;
    pps_arr = new unsigned char[frameSize];
    memcpy((void *)pps_arr, fBuffer, frameSize);
  }

  if(naul_type >= 0 && naul_type <= 5) {
    unsigned char const* base = shm_base + IPC_DATA_BLOCK_SIZE * (frameid % IPC_DATA_BLOCK_NUM);
    unsigned char * resend_sps_flag_addr = shm_base + IPC_DATA_BLOCK_SIZE * IPC_DATA_BLOCK_NUM;
    unsigned *_dataSize = (unsigned *)base;
    base += sizeof(unsigned);
    *_dataSize = frameSize;
    
    if ((naul_type & 0x1f) == 5) { // 关键帧 需要传sps pps
        // If we have NAL units encoded in "sprop parameter strings", prepend these to the file:
        if(sps_length > 0 && pps_length > 0) {
            cus_memcpy(&base, _dataSize, sps_arr, sps_length);
            cus_memcpy(&base, _dataSize, pps_arr, pps_length);
        } else {
            for (unsigned j = 0; j < 3; ++j) {
            unsigned numSPropRecords;
            SPropRecord* sPropRecords
            = parseSPropParameterSets(fSPropParameterSetsStr[j], numSPropRecords);
            for (unsigned i = 0; i < numSPropRecords; ++i) {
                if(sPropRecords[i].sPropLength > 0) {
                    cus_memcpy(&base, _dataSize, sPropRecords[i].sPropBytes, sPropRecords[i].sPropLength);
                }
            }
            delete[] sPropRecords;
            }
        }
        
        fHaveWrittenFirstFrame = True; // for next time
        *resend_sps_flag_addr = 0;
    }

    cus_memcpy(&base, _dataSize, fBuffer, frameSize);

    unsigned long long rtpTimestamp = (presentationTime.tv_sec * 1000000 + presentationTime.tv_usec) / 125;
    forwordRtp(frameSize, rtpTimestamp);
    
    sendto(s, &frameid, sizeof(unsigned) , 0 , (struct sockaddr const*) &serveraddr, slen);
    frameid++;

  }

  // Then try getting the next frame:
  continuePlaying();
}
