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
// Copyright (c) 1996-2022 Live Networks, Inc.  All rights reserved.
// Usage Environment
// Implementation

#include "UsageEnvironment.hh"
#include <stdio.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/ioctl.h>


int LocalSocket_UDPSendMsg(const char *pName, unsigned char *pHeader, int iHeaderLen, unsigned char *pDataBuf, int iDataLen)
{
    struct sockaddr_un stAddr;
    int iSockfd      = -1;
    int iAddrLen     = 0;
    unsigned char *pSendbuf   = NULL;
    int  iSendBufLen = 0;
    int iBytes       = 0;


    if(pName == NULL)
    {
        printf("%s:%d pName should not be NULL!\n", __FUNCTION__, __LINE__);
        return -1;
    }


   if((pHeader != NULL && iHeaderLen != 0)
    &&(pDataBuf == NULL || iDataLen == 0))
    {
        iSendBufLen = iHeaderLen;
        pSendbuf = pHeader;
    }
    else if((pHeader == NULL || iHeaderLen == 0)
          &&(pDataBuf != NULL && iDataLen != 0))
    {
        iSendBufLen = iDataLen;
        pSendbuf = pDataBuf;
    }
    else if((pHeader != NULL && iHeaderLen != 0)
          &&(pDataBuf != NULL && iDataLen != 0))
    {
        iSendBufLen = iHeaderLen+iDataLen;
        pSendbuf = (unsigned char *)malloc(iSendBufLen);
        if(NULL == pSendbuf)
        {
            printf("%s:%d request buffer failed(buf len=%d)\n", __FUNCTION__, __LINE__, iSendBufLen);
            return -1;
        }
        memcpy(pSendbuf, pHeader, iHeaderLen);
        memcpy(pSendbuf + iHeaderLen, pDataBuf, iDataLen);
    }
    else/*if both header and data are invalid, there must be an error*/
    {
        printf("%s:%d both header and data are invalid.\n", __FUNCTION__, __LINE__);
        return -1;
    }


    iSockfd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (iSockfd < 0)
    {
        printf("%s:%d open socket (%s) failed\n", __FUNCTION__, __LINE__, pName);
        if((pSendbuf != pDataBuf) && (pSendbuf != pHeader) && (pSendbuf != NULL))
        {
            free(pSendbuf);
        }
        return -1;
    }

    fcntl(iSockfd, F_SETFD, FD_CLOEXEC);
    fcntl(iSockfd, F_SETFL, O_NONBLOCK);

    stAddr.sun_family = AF_UNIX;
    strcpy (stAddr.sun_path, pName);
    iAddrLen = sizeof(stAddr);

    iBytes = sendto(iSockfd, pSendbuf, iSendBufLen, 0, (struct sockaddr *)&stAddr, iAddrLen);
    if(iBytes != iSendBufLen)
    {
        printf("%s:%d data send to (%s) failed(retval=%d)\n", __FUNCTION__, __LINE__, pName, iBytes);
        if((pSendbuf != pDataBuf) && (pSendbuf != pHeader) && (pSendbuf != NULL))
        {
            free(pSendbuf);
        }
        close(iSockfd);
        return -1;
    }

    if((pSendbuf != pDataBuf) && (pSendbuf != pHeader) && (pSendbuf != NULL))
    {
        free(pSendbuf);
    }
    close(iSockfd);
        return 0;
}

typedef unsigned char       BYTE;
#define MAX_ID_SIZE             6
#define MAX_SLICE_NUM         (4 * 1024)

typedef struct ___Packet___Msg__
{
    int order_type;   //  参见ORDER_TYPE， 用来区分内部消息的类型。
    int sockfd;
    int datalen; // 消息数据长度
}s_PacketMsg;
#define INTER_PACKET_SIZE 12

typedef struct tag_SockDataPacket
{
    BYTE dstId[MAX_ID_SIZE];             //  目的ID
    BYTE srcId[MAX_ID_SIZE];             //  源ID
    BYTE funcCode;                       //  功能码
    BYTE operCode;                       //  操作码
    BYTE dataLen[4];                     //  数据长度
}SOCK_DATA_PACKET_T;

const unsigned int threshold_package = 1024;//slicing threshold is 1024
#define kPORT_IPGW "/tmp/UDN_Msg2IPGW"
#define SOCKET_MSG_ORDER_FROM_DESMAIN_LONGF 5
#define SOCKET_MSG_ORDER_FROM_DESMAIN 4

Boolean UsageEnvironment::reclaim() {
  // We delete ourselves only if we have no remainining state:
  if (liveMediaPriv == NULL && groupsockPriv == NULL) {
    delete this;
    return True;
  }

  return False;
}

UsageEnvironment::UsageEnvironment(TaskScheduler& scheduler)
  : liveMediaPriv(NULL), groupsockPriv(NULL), fScheduler(scheduler) {
}

UsageEnvironment::~UsageEnvironment() {
}

// By default, we handle 'should not occur'-type library errors by calling abort().  Subclasses can redefine this, if desired.
// (If your runtime library doesn't define the "abort()" function, then define your own (e.g., that does nothing).)
void UsageEnvironment::internalError() {
  abort();
}

void UsageEnvironment::IMXMPI_TskBufToLocalSocket(unsigned char const* Dataaddr, unsigned Datalen, unsigned long long pts) {
    
	int 					pos = 0;
	static unsigned char tmpSendBuf[1024*100] = {0};//use 100K for sending data, make sure not be exceeded.
    static unsigned char buf_[4] = {0, 0, 0, 1};
	s_PacketMsg packet = {0, 0, 0};
	SOCK_DATA_PACKET_T stHead = { { 0 }, { 0 }, 0, 0, { 0 } };

	stHead.dstId[0] = 0x30;
	stHead.srcId[0] = 0x10;

	stHead.funcCode = 0x0;
	stHead.operCode = 0x1;

	memcpy(tmpSendBuf, &stHead, sizeof(SOCK_DATA_PACKET_T));

    int splitNum = (Datalen / threshold_package);
    if(Datalen % threshold_package > 0) {
        splitNum++;
    }

    if(splitNum == 1) {
        int isKeyFrame = 0;
        int                     sliceNum = 1;    /* slice number in one frame */
        int                     sliceLen[MAX_SLICE_NUM] = {0};
        sliceLen[0] = Datalen + 4; 

        memcpy(tmpSendBuf + sizeof(SOCK_DATA_PACKET_T) + pos,(void*)&Datalen, 4);
        pos+=4;

        memcpy(tmpSendBuf + sizeof(SOCK_DATA_PACKET_T) + pos,(void*)&isKeyFrame, 4);
        pos+=4;
        memcpy(tmpSendBuf + sizeof(SOCK_DATA_PACKET_T) + pos,(void*)&pts, 4);
        pos+=4;
        memcpy(tmpSendBuf + sizeof(SOCK_DATA_PACKET_T) + pos,(void*)&sliceNum, 4);
        pos+=4;
        memcpy(tmpSendBuf + sizeof(SOCK_DATA_PACKET_T) + pos,(void*)&sliceLen[0], 4*64);
        pos+=4*64;

        memcpy(tmpSendBuf + sizeof(SOCK_DATA_PACKET_T) + pos, buf_, 4);
        pos+=4;
        memcpy(tmpSendBuf + sizeof(SOCK_DATA_PACKET_T) + pos,(void*)Dataaddr, Datalen);
        pos+=Datalen;
        memset(&packet, 0, sizeof(s_PacketMsg));
        packet.order_type = SOCKET_MSG_ORDER_FROM_DESMAIN;
        packet.datalen = sizeof(SOCK_DATA_PACKET_T) + pos;
        //printf("%d=======%d",packet.datalen ,pos);

        LocalSocket_UDPSendMsg(kPORT_IPGW,(BYTE *)&packet, INTER_PACKET_SIZE, tmpSendBuf, packet.datalen);
    } else {
        unsigned char naul_type = Dataaddr[0];
        Dataaddr++;
        Datalen--;

        for(int i=0; i<splitNum; i++) {
            pos=0;
            int _Datalen = Datalen / splitNum;
            if(i==splitNum-1) {
                _Datalen += Datalen%splitNum;
            } 
            int isKeyFrame = 0;
            int                     sliceNum = 1;    /* slice number in one frame */
            int                     sliceLen[MAX_SLICE_NUM] = {0};
            sliceLen[0] = _Datalen + 6; 

            memcpy(tmpSendBuf + sizeof(SOCK_DATA_PACKET_T) + pos,(void*)&_Datalen, 4);
            pos+=4;

            memcpy(tmpSendBuf + sizeof(SOCK_DATA_PACKET_T) + pos,(void*)&isKeyFrame, 4);
            pos+=4;
            memcpy(tmpSendBuf + sizeof(SOCK_DATA_PACKET_T) + pos,(void*)&pts, 4);
            pos+=4;
            memcpy(tmpSendBuf + sizeof(SOCK_DATA_PACKET_T) + pos,(void*)&sliceNum, 4);
            pos+=4;
            memcpy(tmpSendBuf + sizeof(SOCK_DATA_PACKET_T) + pos,(void*)&sliceLen[0], 4*64);
            pos+=4*64;

            memcpy(tmpSendBuf + sizeof(SOCK_DATA_PACKET_T) + pos, buf_, 4);
            pos+=4;

            tmpSendBuf[pos + sizeof(SOCK_DATA_PACKET_T)] = (naul_type & (7<<5)) + 28;
            if(i == 0) {
                tmpSendBuf[pos + sizeof(SOCK_DATA_PACKET_T) + 1] = (naul_type & 0x1f) + (4<<5);
            } else if(i == splitNum - 1) {
                tmpSendBuf[pos + sizeof(SOCK_DATA_PACKET_T) + 1] = (naul_type & 0x1f) + (2<<5);
            } else {
                tmpSendBuf[pos + sizeof(SOCK_DATA_PACKET_T) + 1] = (naul_type & 0x1f);
            }
            


            pos += 2;
            memcpy(tmpSendBuf + sizeof(SOCK_DATA_PACKET_T) + pos,(void*)Dataaddr, _Datalen);

            Dataaddr += _Datalen;
            pos+=_Datalen;
            memset(&packet, 0, sizeof(s_PacketMsg));
            packet.order_type = SOCKET_MSG_ORDER_FROM_DESMAIN;
            packet.datalen = sizeof(SOCK_DATA_PACKET_T) + pos;
            //printf("%d=======%d",packet.datalen ,pos);

            LocalSocket_UDPSendMsg(kPORT_IPGW,(BYTE *)&packet, INTER_PACKET_SIZE, tmpSendBuf, packet.datalen);
        }

    }
}


TaskScheduler::TaskScheduler() {
}

TaskScheduler::~TaskScheduler() {
}

void TaskScheduler::rescheduleDelayedTask(TaskToken& task,
					  int64_t microseconds, TaskFunc* proc,
					  void* clientData) {
  unscheduleDelayedTask(task);
  task = scheduleDelayedTask(microseconds, proc, clientData);
}

// By default, we handle 'should not occur'-type library errors by calling abort().  Subclasses can redefine this, if desired.
void TaskScheduler::internalError() {
  abort();
}
