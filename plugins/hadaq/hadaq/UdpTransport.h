/********************************************************************
 * The Data Acquisition Backbone Core (DABC)
 ********************************************************************
 * Copyright (C) 2009-
 * GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 * Planckstr. 1
 * 64291 Darmstadt
 * Germany
 * Contact:  http://dabc.gsi.de
 ********************************************************************
 * This software can be used under the GPL license agreements as stated
 * in LICENSE.txt file which is part of the distribution.
 ********************************************************************/

#ifndef HADAQ_UDPTRANSPORT_H
#define HADAQ_UDPTRANSPORT_H

#ifndef DABC_SocketThread
#include "dabc/SocketThread.h"
#endif

#ifndef DABC_Transport
#include "dabc/Transport.h"
#endif

#ifndef DABC_BuffersQueue
#include "dabc/BuffersQueue.h"
#endif

#ifndef DABC_MemoryPool
#include "dabc/MemoryPool.h"
#endif

#include <netinet/in.h>
#include <sys/socket.h>


#include "hadaq/HadaqTypeDefs.h"



#define DEFAULT_MTU 63 * 1024

namespace hadaq {


   class UdpDataSocket : public dabc::SocketWorker,
                         public dabc::Transport,
                         protected dabc::MemoryPoolRequester {

      DABC_TRANSPORT(dabc::SocketWorker)

      protected:

         struct sockaddr_in    fSockAddr;
         size_t             fMTU;
         dabc::Mutex        fQueueMutex;
         dabc::BuffersQueue fQueue;


         /*
          * Schema of the buffer pointer meanings:
          *   fTgtBuf.SegmentPtr()  - begin of dabc buffer segment
          *                                  ^
          *                            previous events length
          *                                  v
          *    fTgtPtr        - begin of current HadTu message
          *                                  ^
          *                              fTgtShift
          *                                  v
          *    (fTgtPtr +  fTgtShift) - next position to write data
          *                                  ^
          *                            fTgtBuf.SegmentSize() -  (fTgtPtr +  fTgtShift)
          *                                  v
          *    fEndPtr                - end of buffer segment
          */


         dabc::Buffer       fTgtBuf;   // pointer on buffer, where next portion can be received, use pointer while it is buffer from the queue
         unsigned           fTgtShift; // current shift in the buffer
         char*              fTgtPtr;   // location of next subevent header data to be received
         char*              fEndPtr;   // end of current buffer
         char*              fTempBuf; // buffer to recv packet when no regular place is available

         unsigned           fBufferSize;

         dabc::Buffer       fEvtBuf; // buffer for output event/subevent data
         char*              fEvtPtr;   // cursor pointer in EvtBuf
         char*              fEvtEndPtr;   // end of current event buffer

         dabc::MemoryPoolRef fPool;  // reference on the pool, reference should help us to preserve pool as long as we are using it


         uint64_t           fTotalRecvPacket;
         uint64_t           fTotalDiscardPacket;
         uint64_t           fTotalRecvMsg;
         uint64_t           fTotalDiscardMsg;
         uint64_t           fTotalRecvBytes;
         uint64_t           fTotalRecvEvents;
         uint64_t           fTotalRecvBuffers;
         uint64_t           fTotalDroppedBuffers;

         double             fFlushTimeout;  // timeout for controls parameter update

         /* if true, we will produce full hadaq events with subevent for direct use.
          * otherwise, produce subevent stream for consecutive event builder module.
          */
         bool fBuildFullEvent;

         /* switch to export some counters to shmem observer*/
         bool fWithObserver;
         
         double fLastUpdateTime;

         /* run id from timeofday for eventbuilding*/
         RunId fRunNumber;

         /* port id number from transport name*/
         unsigned int fIdNumber;

         /* actually opened udp port number*/
         int fNPort;

         std::string                fDataRateName;

         //virtual bool ReplyCommand(dabc::Command cmd);

         virtual void ProcessPoolChanged(dabc::MemoryPool* pool) {}

         virtual bool ProcessPoolRequest();

         virtual double ProcessTimeout(double lastdiff);

         void ConfigureFor(dabc::Port* port, dabc::Command cmd);

         void setFlushTimeout(double tmout) { fFlushTimeout = tmout; }
         double getFlushTimeout() const {  return fFlushTimeout; }

         int OpenUdp(int& portnum, int portmin, int portmax, int & rcvBufLenReq);

         /* use recvfrom() as in hadaq nettrans::recvGeneric*/
         ssize_t DoUdpRecvFrom(void* buf, size_t len, int flags=0);

         /* copy next received unit from udp buffer to dabc buffer*/
         void ReadUdp();

         /* Switch to next dabc buffer, put old one into receive queue
          * copyspanning will copy a spanning hadtu fragment from old to new buffers*/
         void NewReceiveBuffer(bool copyspanning=false);

         /*
          * Do simple eventbuilding into output buffer if enabled.
          */
         void FillEventBuffer();

         /*
          * Export counters to control system here:
          * */
         std::string GetNetmemParName(const std::string& name);
         void CreateNetmemPar(const std::string& name);
         void SetNetmemPar(const std::string& name, unsigned value);


         void RegisterExportedCounters();
         bool UpdateExportedCounters();
         void ClearExportedCounters();

      public:
         UdpDataSocket(dabc::Reference port, dabc::Command cmd);
         virtual ~UdpDataSocket();

         virtual void ProcessEvent(const dabc::EventId&);

         virtual bool ProvidesInput() { return true; }
         virtual bool ProvidesOutput() { return false; }

         virtual bool Recv(dabc::Buffer& buf);
         virtual unsigned  RecvQueueSize() const;
         virtual dabc::Buffer& RecvBuffer(unsigned indx) const;
         virtual bool Send(const dabc::Buffer& buf) { return false; }
         virtual unsigned SendQueueSize() { return 0; }
         virtual unsigned MaxSendSegments() { return 0; }

         virtual void StartTransport();
         virtual void StopTransport();
   };

}

#endif
