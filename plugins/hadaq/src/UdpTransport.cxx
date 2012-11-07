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

#include "hadaq/UdpTransport.h"
#include "hadaq/Iterator.h"

#include "dabc/timing.h"
#include "dabc/Port.h"
#include "dabc/version.h"
#include "dabc/Manager.h"

// #include <iostream>
#include <errno.h>

#include <math.h>


//#define UDP_PRINT_EVENTS 1

#define UDP_FILLEVENTS_WITHITERATOR 1


hadaq::UdpDataSocket::UdpDataSocket(dabc::Reference port, dabc::Command cmd) :
      dabc::SocketWorker(), dabc::Transport(port.Ref()), dabc::MemoryPoolRequester(), fQueueMutex(), fQueue(
            ((dabc::Port*) port())->InputQueueCapacity()), fTgtBuf()
{
   // we will react on all input packets
   SetDoingInput(true);

   fTgtShift = 0;
   fTgtPtr = 0;
   fEndPtr = 0;
   fBufferSize = 0;
   fTempBuf = 0;

   fTotalRecvPacket = 0;
   fTotalDiscardPacket = 0;
   fTotalRecvMsg = 0;
   fTotalDiscardMsg = 0;
   fTotalRecvBytes = 0;
   fTotalRecvEvents =0;
   fTotalRecvBuffers=0;
   fTotalDroppedBuffers=0;

   fLastUpdateTime = 0;
   fFlushTimeout = 0.2;
   fBufferSize = 2 * DEFAULT_MTU;
   fBuildFullEvent = false;

   ConfigureFor((dabc::Port*) port(), cmd);

//   DOUT0(("Create %p instance of UdpDataSocket", this));
}

hadaq::UdpDataSocket::~UdpDataSocket()
{
//   DOUT0(("Delete %p instance of UdpDataSocket", this));

   if (fTempBuf) { delete [] fTempBuf; fTempBuf = 0; }

   fEvtBuf.Release();
   fTgtBuf.Release();
   fQueue.Cleanup();
   fPool.Release();
}

void hadaq::UdpDataSocket::ConfigureFor(dabc::Port* port, dabc::Command cmd)
{
   SetName(port->GetName());
   fIdNumber=0;
   int id=0;
   if(sscanf(GetName(), "Input%d", &id)==1){
      fIdNumber=id;
      DOUT0(("hadaq::UdpDataSocket %s got id: %d", GetName(), fIdNumber));
   }

   fBufferSize = port->Cfg(dabc::xmlBufferSize, cmd).AsInt(fBufferSize);
   
   fFlushTimeout = port->Cfg(dabc::xmlFlushTimeout).AsDouble(fFlushTimeout);
   // fFlushTimeout = 0.2;
//   DOUT0(("fFlushTimeout = %5.3f %s  port %s", fFlushTimeout, dabc::xmlFlushTimeout, port->GetName()));

   DOUT0(("Hadaq buffer size %u flush timeout %3.2f", (unsigned) fBufferSize, fFlushTimeout));
   

   fWithObserver = port->Cfg(hadaq::xmlObserverEnabled, cmd).AsBool(false);

   fBuildFullEvent = port->Cfg(hadaq::xmlBuildFullEvent, cmd).AsBool(fBuildFullEvent);

   if(fBuildFullEvent)
      fRunNumber=hadaq::Event::CreateRunId(); // runid from configuration time
   else
      fRunNumber=0;

   fPool = port->GetMemoryPool();

   fMTU = port->Cfg(hadaq::xmlMTUsize, cmd).AsInt(DEFAULT_MTU);
   if (fTempBuf) delete [] fTempBuf;
   fTempBuf = new char[fMTU];

   fNPort = port->Cfg(hadaq::xmlUdpPort, cmd).AsInt(0);
   std::string hostname = port->Cfg(hadaq::xmlUdpAddress, cmd).AsStdStr("0.0.0.0");
   int rcvBufLenReq = port->Cfg(hadaq::xmlUdpBuffer, cmd).AsInt(1 * (1 << 20));
   
   if (OpenUdp(fNPort, fNPort, fNPort, rcvBufLenReq) < 0) {
      EOUT(("hadaq::UdpDataSocket:: failed to open udp port %d with receive buffer %d", fNPort,rcvBufLenReq));
      CloseSocket();
      OnConnectionClosed();
      return;
   }
   DOUT0(("hadaq::UdpDataSocket:: Open udp port %d with rcvbuf %d, dabc buffer size = %u, buildfullevents=%d", fNPort, rcvBufLenReq, fBufferSize, fBuildFullEvent));
   //std::cout <<"---------- fBuildFullEvent is "<< fBuildFullEvent << std::endl;
   if(fWithObserver)
   {
      // workaround to suppress problems with dim observer when this ratemeter is registered:
      std::string ratesprefix = GetName();
      fDataRateName = ratesprefix + "-Datarate";
      CreatePar(fDataRateName).SetRatemeter(false, 5.).SetUnits("B");
      Par(fDataRateName).SetDebugLevel(1);
      RegisterExportedCounters();
   }
}


void hadaq::UdpDataSocket::ProcessEvent(const dabc::EventId& evnt)
{
   switch (evnt.GetCode()) {
      case evntSocketRead: {

         // this is required for DABC 2.0 to again enable read event generation
         SetDoingInput(true);
         ReadUdp();

         /*
         static unsigned  numd(0);
         static dabc::TimeStamp tm1, tm2;
         if (++numd == 500) {
            tm2.GetNow();
            if (!tm1.null())
               DOUT0(("DRate = %5.2f", 1.*numd / (0.+tm2.AsDouble()-tm1.AsDouble())));
            tm1 = tm2;
            numd = 0;
         }
         */
         
         break;
      }

      default:
         dabc::SocketWorker::ProcessEvent(evnt);
         break;
   }
}

double  hadaq::UdpDataSocket::ProcessTimeout(double lastdiff)
{
   DOUT5(("hadaq::UdpDataSocket %s ProcessTimeoutlastdiff=%e", GetName(),lastdiff));
   
   if (fFlushTimeout > 0)
      NewReceiveBuffer();
   
   if (fWithObserver) 
      UpdateExportedCounters();
   
   return fFlushTimeout > 0. ? fFlushTimeout : 3.;
}

void hadaq::UdpDataSocket::RegisterExportedCounters()
{
   if(!fWithObserver) return;
   CreateNetmemPar(dabc::format("pktsReceived%d",fIdNumber));
   CreateNetmemPar(dabc::format("pktsDiscarded%d",fIdNumber));
   CreateNetmemPar(dabc::format("msgsReceived%d",fIdNumber));
   CreateNetmemPar(dabc::format("msgsDiscarded%d",fIdNumber));
   CreateNetmemPar(dabc::format("bytesReceived%d",fIdNumber));
   CreateNetmemPar(dabc::format("netmemBuff%d",fIdNumber));
   CreateNetmemPar(dabc::format("bytesReceivedRate%d",fIdNumber));
   CreateNetmemPar(dabc::format("portNr%d",fIdNumber));
   SetNetmemPar(dabc::format("portNr%d",fIdNumber),fNPort);

}


bool hadaq::UdpDataSocket::UpdateExportedCounters()
{
   if(!fWithObserver) return false;
   
   double tm1 = dabc::TimeStamp::Now().AsDouble();
   if (tm1 - fLastUpdateTime < 1) return false;
   fLastUpdateTime = tm1;
   
   
   SetNetmemPar(dabc::format("pktsReceived%d", fIdNumber), fTotalRecvPacket);
   SetNetmemPar(dabc::format("pktsDiscarded%d", fIdNumber),
         fTotalDiscardPacket);
   SetNetmemPar(dabc::format("msgsReceived%d", fIdNumber), fTotalRecvMsg);
   SetNetmemPar(dabc::format("msgsDiscarded%d", fIdNumber), fTotalDiscardMsg);
   SetNetmemPar(dabc::format("bytesReceived%d", fIdNumber), fTotalRecvBytes);
   float ratio=0;
   {
   // may this lead to some deadlock?
   //dabc::LockGuard guard(fQueueMutex);
      if(fQueue.Capacity()>0)
         ratio=fQueue.Size()/fQueue.Capacity();
   }
   unsigned fill=100*ratio;
   SetNetmemPar(dabc::format("netmemBuff%d",fIdNumber),fill);

   SetNetmemPar(dabc::format("bytesReceivedRate%d",fIdNumber), Par(fDataRateName).AsUInt());

   return true;
}

void hadaq::UdpDataSocket::ClearExportedCounters()
{
   fTotalRecvPacket = 0;
   fTotalDiscardPacket = 0;
   fTotalRecvMsg = 0;
   fTotalDiscardMsg = 0;
   fTotalRecvBytes = 0;
   fTotalRecvEvents = 0;
   fTotalRecvBuffers = 0;
   fTotalDroppedBuffers = 0;
}


bool hadaq::UdpDataSocket::Recv(dabc::Buffer& buf)
{
   {
      dabc::LockGuard lock(fQueueMutex);
      if (fQueue.Size() <= 0)
         return false;

      DOUT5(("%s UdpDataSocket::Recv has queue entry",GetName()));
#if DABC_VERSION_CODE >= DABC_VERSION(1,9,2)
      fQueue.PopBuffer(buf);
#else
      buf << fQueue.Pop();
#endif
   }
   return !buf.null();
}

unsigned hadaq::UdpDataSocket::RecvQueueSize() const
{
   dabc::LockGuard guard(fQueueMutex);
   DOUT5(("%s UdpDataSocket::RecvQueueSize()=%d",GetName(),fQueue.Size()));
   return fQueue.Size();
}

dabc::Buffer& hadaq::UdpDataSocket::RecvBuffer(unsigned indx) const
{
   dabc::LockGuard lock(fQueueMutex);
   DOUT5(("%s UdpDataSocket::RecvBuffer %d ",GetName(),indx));
   return fQueue.ItemRef(indx);
}

bool hadaq::UdpDataSocket::ProcessPoolRequest()
{
   return true;
}

void hadaq::UdpDataSocket::StartTransport()
{
   DOUT3(("Starting UDP transport %s",GetName()));
   NewReceiveBuffer();
   
   if(!ActivateTimeout(fFlushTimeout > 0. ? fFlushTimeout : 3.))
      EOUT(("%s could not activate timeout of %f s",GetName(),fFlushTimeout));
}

void hadaq::UdpDataSocket::StopTransport()
{
   DOUT3(("Stopping hadaq:udp transport -"));
   DOUT0(("%s: RecvPackets:%d, DiscPackets:%d, RecvMsg:%d, DiscMsg:%d, RecvBytes:%d, RcvBufs:%d DropBufs:%d BuildEvts:%d", GetName(), (int) fTotalRecvPacket, (int) fTotalDiscardPacket, (int) fTotalRecvMsg, (int) fTotalDiscardMsg, (int) fTotalRecvBytes, (int) fTotalRecvBuffers, (int) fTotalDroppedBuffers, (int) fTotalRecvEvents));
   // NOTE : we see strange things in DOUT, wrong or shifted values if we do not cast uint63_t to int!
// this output would not go into dabc logfile:
//   std::cout <<"----- UdpDataSocket "<<GetName()<<" Statistics: -----"<<std::endl;
//   std::cout << "RecvPackets:" << fTotalRecvPacket << ", DiscPackets:"
//         << fTotalDiscardPacket << ", RecvMsg:" << fTotalRecvMsg << ", DiscMsg:"
//         << fTotalDiscardMsg << ", RecvBytes:" << fTotalRecvBytes << ", RecvBuffers:" << fTotalRecvBuffers<< ", DroppedBuffers:" << fTotalDroppedBuffers;
//   if(fBuildFullEvent)
//      std::cout << ", fTotalRecvEvents:"<< fTotalRecvEvents;
//   std::cout <<std::endl;

}

void hadaq::UdpDataSocket::ReadUdp()
{
   // this function is a "translation" to dabc of the hadaq nettrans.c assembleMsg()
   if (fTgtPtr == 0) {
      // this will happen if the udp senders are active before our transport is configured
      //EOUT(("hadaq::UdpDataSocket::ReadUdp NEVER COME HERE: zero receive pointer"));
      return;
   }
   DOUT5(("ReadUdp before DoUdpRecvFrom"));
   ssize_t len = DoUdpRecvFrom(fTgtPtr + fTgtShift, fMTU); // request with full mtu buffer
   // check here length and validitiy:
   if (len < 0) {
      EOUT(("hadaq::UdpDataSocket::ReadUdp failed after recvfrom"));
      return; // NEVER COME HERE, DoUdpRecvFrom should have handled this!
   }

   if (fTgtBuf.null()) {
      // we used dummy buffer for receive
      fTgtShift = 0;
      fTotalDiscardPacket++;
   } else {
      // regular received into dabc buffer:
      fTgtShift += len;
      fTotalRecvPacket++;
   }
   hadaq::HadTu* hadTu = (hadaq::HadTu*) fTgtPtr;
   size_t msgsize = hadTu->GetPaddedSize() + 32; // trb sender adds a 32 byte control trailer identical to event header
   if ((size_t) len == fMTU) {
      // hadtu send from trb is bigger than mtu, we expect end of messages in the next recvfrom
      if ((size_t)(fEndPtr - fTgtPtr) < msgsize) {
         // rest of expected message does not fit anymore in current buffer
         NewReceiveBuffer(true); // copy the spanning fragment to new dabc buffer
      } //   if ((size_t)(fEndPtr - fTgtPtr
   } else {
      //std::cout << "!!!! fTgtShift:" << fTgtShift << ", hadTU size:"
      //      << hadTu->GetSize() << std::endl;
      // we finished receive with current hadtu message, check it:
      if (fTgtShift != msgsize
            || memcmp((char*) hadTu + hadTu->GetPaddedSize(), (char*) hadTu, 32)) {
         // received size does not match header info, or header!=trailer contents
         fTgtShift = 0;
         fTotalDiscardMsg++;
         DOUT0(("Discarded message: %d", fTotalDiscardMsg));

      } else {
         fTotalRecvMsg++;
         fTotalRecvBytes += fTgtShift;

         // only debug, should be seen swapped SYNC message
         // uint32_t* p1 = (uint32_t*)(fTgtPtr + hadTu->GetPaddedSize());
         // DOUT1(("Get UDP data of size %u %x", (unsigned) fTgtShift, *(p1-4)));

         Par(fDataRateName).SetDouble(fTgtShift);
         fTgtPtr += hadTu->GetPaddedSize(); // we will overwrite the trailing block of this message again
         fTgtShift = 0;
         if (fTgtBuf.null() || (size_t)(fEndPtr - fTgtPtr) < fMTU) {
            NewReceiveBuffer(false); // no more space for full mtu in buffer, finalize old and get new:
         }

      } // if (fTgtShift != msgsize

   } //if (len == fMTU)

}

void hadaq::UdpDataSocket::NewReceiveBuffer(bool copyspanning)
{

   DOUT5(("%s NewReceiveBuffer, copyspanning=%d",GetName(),copyspanning));
   dabc::Buffer outbuf;
   if (!fTgtBuf.null()) {
      // first shrink last receive buffer to full received hadtu envelopes:
      // to cut last trailing event header, otherwise we get trouble with read iterator for evtbuilding
      unsigned filledsize = fTgtPtr - (char*) fTgtBuf.SegmentPtr();
      
      if (filledsize==0) return;
      
      fTgtBuf.SetTotalSize(filledsize);

      // then we optionally format the output buffer

      if (fBuildFullEvent) {
         // in simple eventbuilding mode, we have to scan old receive buffer for actual subevents
         // and add an hades event header for each of those

         fEvtBuf = fPool.TakeBufferReq(this, fBufferSize);
         if (fEvtBuf.null()) {
            EOUT(("hadaq::UdpDataSocket::NewReceiveBuffer - could not take event buffer of size %d ",fBufferSize));
            OnConnectionClosed(); // FIXME: better error handling here!

         }
         fEvtBuf.SetTypeId(hadaq::mbt_HadaqEvents); // buffer will contain events with subevents
         fEvtPtr = (char*) fEvtBuf.SegmentPtr();
         fEvtEndPtr = (char*) fEvtBuf.SegmentPtr() + fEvtBuf.SegmentSize();

         outbuf = fEvtBuf;
         FillEventBuffer();

      } else {
         // no event building: pass receive buffer over as it is
         outbuf = fTgtBuf.HandOver();
      }
   } //  if (!fTgtBuf.null()) oldbuffer

// now get new receive buffer
   fTgtBuf = fPool.TakeBufferReq(this, fBufferSize);
   if (!fTgtBuf.null()) {
      fTgtBuf.SetTypeId(hadaq::mbt_HadaqTransportUnit); // raw transport unit with subevents as from trb

      char* bufstart = (char*) fTgtBuf.SegmentPtr(); // is modified by puteventheader
      if (copyspanning) {
         // copy data with full NetTransx event header for spanning events anyway
         size_t copylength = fTgtShift;
         memcpy(bufstart, fTgtPtr, copylength);
         DOUT0(("Copied %d spanning bytes to new DABC buffer of size %d",copylength,fTgtBuf.SegmentSize()));
         fTgtPtr = bufstart;
         // note that we do keep old fTgtShift for the spanning event
      } else {
         fTgtPtr = bufstart;
      }
      fEndPtr = (char*) fTgtBuf.SegmentPtr() + fTgtBuf.SegmentSize();
      DOUT3(("NewReceiveBuffer gets fTgtPtr: %x, fEndPtr: %x",fTgtPtr, fEndPtr));
      if (fTgtBuf.SegmentSize() < fMTU) {
         EOUT(("hadaq::UdpDataSocket::NewReceiveBuffer - buffer segment size %d < mtu $d",fTgtBuf.SegmentSize(),fMTU));
         OnConnectionClosed(); // FIXME: better error handling here!
      }

   } else {
      fTgtPtr = fTempBuf;
      fEndPtr = fTempBuf + fMTU*sizeof(char);
      fTgtShift = 0;
      DOUT0(("hadaq::UdpDataSocket:: No pool buffer available for socket read, use internal dummy!"));
   } //if (!fTgtBuf.null()) newbuffer

   if (!outbuf.null()) {
      // TODO: here check if we can still add something to queue. otherwise we drop
      bool dropbuffer = false;
      {
         dabc::LockGuard lock(fQueueMutex);
         if (fQueue.Full()) {
            DOUT3(("hadaq::UdpDataSocket %s: output queue is full, dropping buffer!",GetName()));
            dropbuffer = true;
         } else {
            fQueue.Push(outbuf.HandOver()); // put old buffer to transport queue no sooner than we have copied spanning event
         }
      } // lockguard
      // do framework actions outside queue mutex:
      if (dropbuffer) {
         fTotalDroppedBuffers++;
         outbuf.Release();
      } else {
         fTotalRecvBuffers++;
         FirePortInput();
         // DOUT0(("New buffer in the queue"));
      }
   } // if (!outbuf.null())
}




void  hadaq::UdpDataSocket::FillEventBuffer()
{
#ifdef   UDP_FILLEVENTS_WITHITERATOR
   // this is regular mode using the iterator classes:
   hadaq::WriteIterator witer(fEvtBuf);
   hadaq::ReadIterator riter(fTgtBuf);
   while (riter.NextHadTu()) {
      // udp envelope is hadtu
      while (riter.NextSubEvent()) {
         hadaq::Subevent* source = riter.subevnt();
         if(!witer.IsPlaceForEvent(source->GetPaddedSize()))
         {
            DOUT0(("hadaq::UdpDataSocket:: Remaining subevents do not fit in Event output buffer of size %d! Drop them. Check mempool setup",fEvtBuf.SegmentSize()));
            //witer.Close(); // will shrink output buffer to actual used size
            return;
            // TODO: dynamic spanning of output buffer. Maybe left to full combiner module
         }

         witer.NewEvent(fTotalRecvEvents++,fRunNumber);
         witer.AddSubevent(source);
         witer.FinishEvent();
      }

   }
   //witer.Close(); // will shrink output buffer to actual used size.
   // NOTE: we don't need explicit call, is done in witer dtor when leaving scope


#else

   // manual approach. This was first working code with event building. left for checking
            char* cursor=(char*) fTgtBuf.SegmentPtr();
            size_t bufsize=fTgtBuf.SegmentSize();
            while(bufsize> sizeof(hadaq::HadTu) + sizeof(hadaq::Subevent)){

               // outer loop get envelope header: This is a plain HadTu header!

               hadaq::HadTu* hev= (hadaq::Subevent*) cursor;
               size_t hevsize=hev->GetSize();
#ifdef UDP_PRINT_EVENTS
               std::cout <<"Container  0x"<< std::hex<< (int)hev <<" of size"<< std::dec<< hevsize << std::endl;
               char* ptr= cursor;
               for(int i=0; i<16; ++i)
                    {
                       std::cout <<"evt("<<i<<")=0x"<< (std::hex)<< ( (short) *(ptr+i) & 0xff ) <<" , ";
                    }
               std::cout <<(std::dec)<< std::endl;
#endif
               cursor += sizeof(hadaq::HadTu);
               bufsize -=sizeof(hadaq::HadTu);
               while(hevsize>sizeof(hadaq::Subevent)){
                   // inner loop scans actual subevents
                   hadaq::Subevent* sev= (hadaq::Subevent*) cursor;
                   size_t sublen = sev->GetSize(); // real payload of subevent
                   size_t padlen = sev->GetPaddedSize(); // actual copied data length, 8 byte padded
                   size_t evlen = sublen + sizeof(hadaq::Event);
                   // DUMP EVENT
#ifdef UDP_PRINT_EVENTS
                   std::cout <<" subevent 0x"<< std::hex<< (int)sev <<" of size "<< std::dec << sublen <<", padded size "<< std::dec << padlen<< std::endl;
                   char* ptr= cursor + sizeof(hadaq::Subevent);
                   for(int i=0; i<20; ++i)
                       {
                          std::cout <<"sub("<<i<<")=0x"<< (std::hex)<< ( (short) *(ptr+i) & 0xff ) <<" , ";
                       }
                  std::cout <<(std::dec)<< std::endl;
#endif
                  ///////////////


                  // each subevent of single stream is wrapped into event header
                  size_t subrest = (size_t)(fEvtEndPtr - fEvtPtr);
                  if (subrest < evlen) {
                     DOUT0(("hadaq::UdpDataSocket:: Remaining subevents do not fit in Event output buffer of size %d! Drop them. Check mempool setup",fEvtBuf.SegmentSize()));
                     break;
                     // TODO: dynamic spanning of output buffer. Maybe left to full combiner module
                  }

                  // TODO: handle this with write iterator later
                  hadaq::Event* curev = hadaq::Event::PutEventHeader(&fEvtPtr,
                        EvtId_data);
                  memcpy(fEvtPtr, (char*) sev, padlen);
                  curev->SetSize(evlen); // only one subevent in this mode.
                  // currId = currId | (DAQVERSION << 12);
                  uint32_t currId = 0x00003001; // we define DAQVERSION 3 for DABC
                  curev->SetId(currId);
                  curev->SetSeqNr(fTotalRecvEvents++);
                  curev->SetRunNr(0); // TODO: do we need to set from parameter? maybe left to eventbuilder module later.
#ifdef UDP_PRINT_EVENTS
                  std::cout << "Build NextEvent " << fTotalRecvEvents
                        << " with curev:" << (int) curev << ",fEvtPtr:"
                        << (int) fEvtPtr;
                  std::cout << ", evlen:" << evlen << std::endl;
#endif

                  fEvtPtr += padlen;
                  cursor += padlen;
                  hevsize -= padlen;
                  bufsize -=padlen;

               } // while next subevent
               //break; // use only first hadtu in buffer
            } // while next hadtu

            unsigned filledsize = fEvtPtr - (char*) fEvtBuf.SegmentPtr();
            fEvtBuf.SetTotalSize(filledsize);
#endif

}




ssize_t hadaq::UdpDataSocket::DoUdpRecvFrom(void* buf, size_t len, int flags)
{
   ssize_t res = 0;
   size_t socklen = (size_t) sizeof(fSockAddr);
   sockaddr* saptr = (sockaddr*) &fSockAddr; // trick for compiler
   //std::cout <<"recvfrom writes to "<<(int) buf <<", len="<<len<< std::endl;
   res = recvfrom(fSocket, buf, len, flags, saptr, (socklen_t *) &socklen);
   //std::cout <<"recvfrom returns"<<res << std::endl;
   if (res == 0)
      OnConnectionClosed();
   else if (res < 0) {
      if (errno != EAGAIN)
         OnSocketError(errno, "When recvfrom()");
   }
   return res;
}

int hadaq::UdpDataSocket::OpenUdp(int& portnum, int portmin, int portmax,
      int& rcvBufLenReq)
{

   int fd = socket(PF_INET, SOCK_DGRAM, 0);
   if (fd < 0)
      return -1;
   SetSocket(fd);
   int numtests = 1; // at least test value of portnum
   if ((portmin > 0) && (portmax > 0) && (portmin <= portmax))
      numtests += (portmax - portmin + 1);

   if (dabc::SocketThread::SetNonBlockSocket(fd))
      for (int ntest = 0; ntest < numtests; ntest++) {
         if ((ntest == 0) && (portnum < 0))
            continue;
         if (ntest > 0)
            portnum = portmin - 1 + ntest;

         memset(&fSockAddr, 0, sizeof(fSockAddr));
         fSockAddr.sin_family = AF_INET;
         fSockAddr.sin_port = htons(portnum);

         if (rcvBufLenReq != 0) {
            // for hadaq application: set receive buffer length _before_ bind:
            //         int rcvBufLenReq = 1 * (1 << 20);
            int rcvBufLenRet;
            size_t rcvBufLenLen = (size_t) sizeof(rcvBufLenReq);
            if (setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &rcvBufLenReq,
                  rcvBufLenLen) == -1) {
               EOUT(( "setsockopt(..., SO_RCVBUF, ...): %s", strerror(errno)));
            }

            if (getsockopt(fd, SOL_SOCKET, SO_RCVBUF, &rcvBufLenRet,
                  (socklen_t *) &rcvBufLenLen) == -1) {
               EOUT(( "getsockopt(..., SO_RCVBUF, ...): %s", strerror(errno)));
            }
            if (rcvBufLenRet < rcvBufLenReq) {
               EOUT(("UDP receive buffer length (%d) smaller than requested buffer length (%d)", rcvBufLenRet, rcvBufLenReq));
               rcvBufLenReq = rcvBufLenRet;
            }
         }

         if (!bind(fd, (struct sockaddr *) &fSockAddr, sizeof(fSockAddr)))
            return fd;
      }
   CloseSocket();
   return -1;
}



std::string  hadaq::UdpDataSocket::GetNetmemParName(const std::string& name)
{
   return dabc::format("%s_%s",hadaq::NetmemPrefix,name.c_str());
}

void hadaq::UdpDataSocket::CreateNetmemPar(const std::string& name)
{
   CreatePar(GetNetmemParName(name));
}

void hadaq::UdpDataSocket::SetNetmemPar(const std::string& name,
      unsigned value)
{
   Par(GetNetmemParName(name)).SetUInt(value);
}


