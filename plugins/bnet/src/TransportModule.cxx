#include "bnet/TransportModule.h"

#include "bnet/defines.h"

#include <vector>
#include <math.h>

#include "dabc/timing.h"
#include "dabc/logging.h"
#include "dabc/Queue.h"
#include "dabc/Manager.h"
#include "dabc/PoolHandle.h"

#ifdef WITH_VERBS
#include "bnet/VerbsRunnable.h"
#endif

bnet::TransportModule::TransportModule(const char* name, dabc::Command cmd) :
   dabc::ModuleSync(name, cmd),
   fStamping()
{
   fNodeNumber = cmd.Field("NodeNumber").AsInt(0);
   fNumNodes = cmd.Field("NumNodes").AsInt(1);

   fNumLids = Cfg("TestNumLids", cmd).AsInt(1);
   if (fNumLids>IBTEST_MAXLID) fNumLids = IBTEST_MAXLID;

   int nports = cmd.Field("NumPorts").AsInt(1);

   fTestKind = Cfg("TestKind", cmd).AsStdStr();

   fTestBufferSize = Cfg(dabc::xmlBufferSize, cmd).AsInt(65536);
   fTestNumBuffers = Cfg(dabc::xmlNumBuffers, cmd).AsInt(1000);

   CreatePoolHandle("BnetCtrlPool");

   CreatePoolHandle("BnetDataPool");

   for (int n=0;n<nports;n++) {
//      DOUT0(("Create IOport %d", n));
      CreateIOPort(FORMAT(("Port%d", n)), FindPool("BnetCtrlPool"), 1, 1);
  }

   fActiveNodes.clear();
   fActiveNodes.resize(fNumNodes, true);

   fTestScheduleFile = Cfg("TestSchedule",cmd).AsStdStr();
   if (!fTestScheduleFile.empty())
      fPreparedSch.ReadFromFile(fTestScheduleFile);

   fResults = 0;

   fCmdDelay = 0.;

   for (int n=0;n<IBTEST_MAXLID;n++) {
      fSendQueue[n] = 0;
      fRecvQueue[n] = 0;
   }
   fTotalSendQueue = 0;
   fTotalRecvQueue = 0;

   for (int lid=0; lid < NumLids(); lid++) {
      fSendQueue[lid] = new int[NumNodes()];
      fRecvQueue[lid] = new int[NumNodes()];
      for (int n=0;n<NumNodes();n++) {
         fSendQueue[lid][n] = 0;
         fRecvQueue[lid][n] = 0;
      }
   }


   fRecvRatemeter = 0;
   fSendRatemeter = 0;
   fWorkRatemeter = 0;

   fTrendingInterval = 0;

   fRunnable = 0;
   fRunThread = 0;

   #ifdef WITH_VERBS
   fRunnable = new VerbsRunnable();
   #endif

   if (fRunnable==0) fRunnable = new TransportRunnable();

   fRunnable->SetNodeId(fNodeNumber, fNumNodes);

   // call Configure before runnable has own thread - no any synchronization problems
   fRunnable->Configure(this, FindPool("BnetDataPool")->Pool(), cmd);

   fCmdBufferSize = 256*1024;
   fCmdDataBuffer = new char[fCmdBufferSize];
}

bnet::TransportModule::~TransportModule()
{
   DOUT2(("Calling ~TransportModule destructor name:%s", GetName()));

   delete fRunThread; fRunThread = 0;
   delete fRunnable; fRunnable = 0;

   if (fCmdDataBuffer) {
      delete [] fCmdDataBuffer;
      fCmdDataBuffer = 0;
   }

   if (fRecvRatemeter) {
      delete fRecvRatemeter;
      fRecvRatemeter = 0;
   }

   if (fSendRatemeter) {
      delete fSendRatemeter;
      fSendRatemeter = 0;
   }

   if (fWorkRatemeter) {
      delete fWorkRatemeter;
      fWorkRatemeter = 0;
   }

   for (int n=0;n<IBTEST_MAXLID;n++) {
      if (fSendQueue[n]) delete [] fSendQueue[n];
      if (fRecvQueue[n]) delete [] fRecvQueue[n];
   }


   AllocResults(0);
}


void bnet::TransportModule::AllocResults(int size)
{
   if (fResults!=0) delete[] fResults;
   fResults = 0;
   if (size>0) {
      fResults = new double[size];
      for (int n=0;n<size;n++) fResults[n] = 0.;
   }
}

int bnet::TransportModule::ExecuteCommand(dabc::Command cmd)
{
   return ModuleSync::ExecuteCommand(cmd);
}

void bnet::TransportModule::BeforeModuleStart()
{
   DOUT2(("IbTestWorkerModule starting"));

   int special_thrd = Cfg("SpecialThread").AsBool(false) ? 0 : -1;

   fRunThread = new dabc::PosixThread(special_thrd); // create thread with specially-allocated CPU

   fRunThread->Start(fRunnable);

   // set threads id to be able check correctness of calling
   fRunnable->SetThreadsIds(dabc::PosixThread::Self(), fRunThread->Id());
}

void bnet::TransportModule::AfterModuleStop()
{
   DOUT2(("IbTestWorkerModule finished"));
}

int bnet::TransportModule::NodeSendQueue(int node) const
{
   if ((node<0) || (node>=NumNodes())) return 0;
   int sum(0);
   for (int lid=0;lid<NumLids();lid++)
      if (fSendQueue[lid]) sum+=fSendQueue[lid][node];
   return sum;
}

int bnet::TransportModule::NodeRecvQueue(int node) const
{
   if ((node<0) || (node>=NumNodes())) return 0;
   int sum(0);
   for (int lid=0;lid<NumLids();lid++)
      if (fRecvQueue[lid]) sum+=fRecvQueue[lid][node];
   return sum;
}


bool bnet::TransportModule::SubmitToRunnable(int recid, OperRec* rec)
{
   if ((rec->tgtnode>=NumNodes()) || (rec->tgtindx>=NumLids())) {
      EOUT(("Wrong record tgt lid:%d node:%d", rec->tgtindx, rec->tgtnode));
      exit(6);
   }

   switch (rec->kind) {
      case kind_Send:
         fSendQueue[rec->tgtindx][rec->tgtnode]++;
         fTotalSendQueue++;
         break;
      case kind_Recv:
         fRecvQueue[rec->tgtindx][rec->tgtnode]++;
         fTotalRecvQueue++;
         break;
      default:
         EOUT(("Wrong operation kind"));
         break;
   }


   if (!fRunnable->SubmitRec(recid)) {
      EOUT(("Cannot submit receive operation to lid %d node %d", rec->tgtindx, rec->tgtnode));
      return false;
   }

   return true;

}


int bnet::TransportModule::CheckCompletionQueue(double waittm)
{
   int recid = fRunnable->WaitCompleted(waittm);

   OperRec* rec = fRunnable->GetRec(recid);

   if (rec!=0) {
      if ((rec->tgtnode>=NumNodes()) || (rec->tgtindx>=NumLids())) {
         EOUT(("Wrong result id lid:%d node:%d", rec->tgtindx, rec->tgtnode));
         exit(6);
      }

      switch (rec->kind) {
         case kind_Send:
            fSendQueue[rec->tgtindx][rec->tgtnode]--;
            fTotalSendQueue--;
            break;
         case kind_Recv:
            fRecvQueue[rec->tgtindx][rec->tgtnode]--;
            fTotalRecvQueue--;
            break;
         default:
            EOUT(("Wrong operation kind"));
            break;
      }
   }

   return recid;
}


bool bnet::TransportModule::SlaveTimeSync(int64_t* cmddata)
{
   int numcycles = cmddata[0];
   int maxqueuelen = cmddata[1];
   int sync_lid = cmddata[2];
   int nrepeat = cmddata[3];

   fRunnable->ConfigSync(false, numcycles);

   bool res = fRunnable->RunSyncLoop(false, 0, sync_lid, maxqueuelen, nrepeat);

   if (res) fRunnable->GetSync(fStamping);

   return res;
}


bool bnet::TransportModule::MasterCommandRequest(int cmdid, void* cmddata, int cmddatasize, void* allresults, int resultpernode)
{
    // Request from master side to move in specific command
    // Actually, appropriate functions should run at about the
    // same time while special delay is calculated before and informed
    // each slave how long it should be delayed

   // if module not working, do not try to do anything else
   if (!ModuleWorking(0.)) {
      DOUT0(("Command %d cannot be executed - module is not active", cmdid));
      return false;
   }

   bool incremental_data = false;
   if (cmddatasize<0) {
      cmddatasize = -cmddatasize;
      incremental_data = true;
   }

   int fullpacketsize = sizeof(CommandMessage) + cmddatasize;

   if (resultpernode > cmddatasize) {
      // special case, to use same buffer on slave for reply,
      // original buffer size should be not less than requested
      // therefore we just append fake data
      fullpacketsize = sizeof(CommandMessage) + resultpernode;
   }

   if (fullpacketsize>fCmdBufferSize) {
      EOUT(("Too big command data size %d", cmddatasize));
      return false;
   }

   dabc::Buffer buf0;

   for(int node=0;node<NumNodes();node++) if (fActiveNodes[node]) {

      dabc::Buffer buf = TakeBuffer(Pool(), fullpacketsize, 1.);
      if (buf.null()) { EOUT(("No empty buffer")); return false; }

      CommandMessage* msg = (CommandMessage*) buf.GetPointer()();

      msg->magic = IBTEST_CMD_MAGIC;
      msg->cmdid = cmdid;
      msg->cmddatasize = cmddatasize;
      msg->delay = 0;
      msg->getresults = resultpernode;
      if ((cmddatasize>0) && (cmddata!=0)) {
         if (incremental_data)
            memcpy(msg->cmddata(), (uint8_t*) cmddata + node * cmddatasize, cmddatasize);
         else
            memcpy(msg->cmddata(), cmddata, cmddatasize);
      }

      buf.SetTotalSize(fullpacketsize);

      if (node==0) {
         buf0 << buf;
      } else {
//         DOUT0(("Send buffer to slave %d", node));
         Send(IOPort(node-1), buf, 1.);
      }
   }

   PreprocessSlaveCommand(buf0);

   std::vector<bool> replies;
   replies.resize(NumNodes(), false);
   bool finished = false;

   while (ModuleWorking() && !finished) {
      dabc::Buffer buf;
      buf << buf0;
      int nodeid = 0;

      if (buf.null()) {
         dabc::Port* port(0);
         nodeid = -1;
         buf = RecvFromAny(&port, 4.);
         if (port) nodeid = IOPortNumber(port) + 1;
      }

      if (buf.null() || (nodeid<0)) {
         for (unsigned n=0;n<replies.size();n++)
            if (fActiveNodes[n] && !replies[n]) EOUT(("Cannot get any reply from node %u", n));
         return false;
      }

//      DOUT0(("Recv reply from slave %d", nodeid));

      replies[nodeid] = true;

      CommandMessage* rcv = (CommandMessage*) buf.GetPointer()();

      if (rcv->magic!=IBTEST_CMD_MAGIC) { EOUT(("Wrong magic")); return false; }
      if (rcv->cmdid!=cmdid) { EOUT(("Wrong ID")); return false; }
      if (rcv->node != nodeid) { EOUT(("Wrong node")); return false; }

      if ((allresults!=0) && (resultpernode>0) && (rcv->cmddatasize == resultpernode)) {
         // DOUT0(("Copy from node %d buffer %d", nodeid, resultpernode));
         memcpy((uint8_t*)allresults + nodeid*resultpernode, rcv->cmddata(), resultpernode);
      }

      finished = true;

      for (unsigned n=0;n<replies.size();n++)
         if (fActiveNodes[n] && !replies[n]) finished = false;
   }

   // Should we call slave method here ???

   return true;
}

bool bnet::TransportModule::MasterCollectActiveNodes()
{
   fActiveNodes.clear();
   fActiveNodes.push_back(true); // master itself is active

   uint8_t buff[NumNodes()];

   int cnt = 1;
   buff[0] = 1;

   for(int node=1;node<NumNodes();node++) {
      bool active = IOPort(node-1)->IsConnected();
      fActiveNodes.push_back(active);
      if (active) cnt++;
      buff[node] = active ? 1 : 0;
   }

   DOUT0(("There are %d active nodes, %d missing", cnt, NumNodes() - cnt));

   if (!MasterCommandRequest(IBTEST_CMD_ACTIVENODES, buff, NumNodes())) return false;

   return cnt == NumNodes();
}



bool bnet::TransportModule::CalibrateCommandsChannel(int nloop)
{
   dabc::TimeStamp tm;

   dabc::Average aver;

   for (int n=0;n<nloop;n++) {
      dabc::Sleep(0.001);

      tm = dabc::Now();
      if (!MasterCommandRequest(IBTEST_CMD_TEST)) return false;

      if (n>0)
         aver.Fill(dabc::Now()-tm);
   }

   fCmdDelay = aver.Mean();

   aver.Show("Command loop", true);

   DOUT0(("Command loop = %5.3f ms", fCmdDelay*1e3));

   return true;
}

bool bnet::TransportModule::MasterConnectQPs()
{
   dabc::TimeStamp tm = dabc::Now();

   int blocksize = fRunnable->ConnectionBufferSize();

   if (blocksize==0) return false;

   void* recs = malloc(NumNodes() * blocksize);
   memset(recs, 0, NumNodes() * blocksize);

   // own records will be add automatically
   if (!MasterCommandRequest(IBTEST_CMD_CREATEQP, 0, 0, recs, blocksize)) {
      EOUT(("Cannot create and collect QPs"));
      free(recs);
      return false;
   }

   // now we should resort connection records, loop over targets

   DOUT0(("First collect all QPs info"));

   void* conn = malloc(NumNodes() * blocksize);
   memset(conn, 0, NumNodes()*blocksize);

   fRunnable->ResortConnections(recs, conn);

   bool res = MasterCommandRequest(IBTEST_CMD_CONNECTQP, conn, -blocksize);

   free(recs);

   free(conn);

   DOUT0(("Establish connections in %5.4f s ", tm.SpentTillNow()));

   return res;
}

bool bnet::TransportModule::MasterCallExit()
{
   MasterCommandRequest(IBTEST_CMD_EXIT);

   fRunnable->StopRunnable();
   fRunThread->Join();

   dabc::mgr.StopApplication();

   return true;
}

int bnet::TransportModule::PreprocessSlaveCommand(dabc::Buffer& buf)
{
   if (buf.null()) return IBTEST_CMD_EXIT;

   CommandMessage* msg = (CommandMessage*) buf.SegmentPtr(0);

   if (msg->magic!=IBTEST_CMD_MAGIC) {
       EOUT(("Magic id is not correct"));
       return IBTEST_CMD_EXIT;
   }

   int cmdid = msg->cmdid;

   fCmdDataSize = msg->cmddatasize;
   if (fCmdDataSize > fCmdBufferSize) {
      EOUT(("command data too big for buffer!!!"));
      fCmdDataSize = fCmdBufferSize;
   }

   memcpy(fCmdDataBuffer, msg->cmddata(), fCmdDataSize);
//   double delay = msg->delay;
//   dabc::TimeStamp recvtm = dabc::Now();

   msg->node = dabc::mgr()->NodeId();
   msg->cmddatasize = 0;
   int sendpacketsize = sizeof(CommandMessage);

   bool cmd_res(false);

   switch (cmdid) {
      case IBTEST_CMD_TEST:

         cmd_res = true;
         break;

      case IBTEST_CMD_ACTIVENODES: {
         uint8_t* buff = (uint8_t*) fCmdDataBuffer;

         if (fCmdDataSize!=NumNodes()) {
            EOUT(("Argument size mismatch"));
            cmd_res = false;
         }

         fActiveNodes.clear();
         for (int n=0;n<fCmdDataSize;n++)
            fActiveNodes.push_back(buff[n]>0);

         cmd_res = fRunnable->SetActiveNodes(fCmdDataBuffer, fCmdDataSize);

         break;
      }

      case IBTEST_CMD_COLLECT:
         if ((msg->getresults > 0) && (fResults!=0)) {
            msg->cmddatasize = msg->getresults;
            sendpacketsize += msg->cmddatasize;
            memcpy(msg->cmddata(), fResults, msg->getresults);
            cmd_res = true;
         }
         break;

      case IBTEST_CMD_CREATEQP: {
         int ressize = msg->getresults;
         cmd_res = fRunnable->CreateQPs(msg->cmddata(), ressize);
         msg->cmddatasize = ressize;
         sendpacketsize += ressize;
         break;
      }

      case IBTEST_CMD_CONNECTQP:
         cmd_res = fRunnable->ConnectQPs(fCmdDataBuffer, fCmdDataSize);
         break;

      case IBTEST_CMD_CLEANUP:
         cmd_res = ProcessCleanup((int32_t*)fCmdDataBuffer);
         break;

      case IBTEST_CMD_CLOSEQP:
         cmd_res = fRunnable->CloseQPs();
         break;

      case IBTEST_CMD_ASKQUEUE:
         msg->cmddatasize = ProcessAskQueue(msg->cmddata());
         sendpacketsize += msg->cmddatasize;
         cmd_res = true;
         break;

      default:
         cmd_res = true;
         break;
   }

   buf.SetTotalSize(sendpacketsize);

   if (!cmd_res) EOUT(("Command %d failed", cmdid));

   return cmdid;
}

bool bnet::TransportModule::ExecuteSlaveCommand(int cmdid)
{
   switch (cmdid) {
      case IBTEST_CMD_NONE:
         EOUT(("Error in WaitForCommand"));
         return false;

      case IBTEST_CMD_TIMESYNC:
         return SlaveTimeSync((int64_t*)fCmdDataBuffer);

      case IBTEST_CMD_SLEEP:
         WorkerSleep(((int64_t*)fCmdDataBuffer)[0]);
         break;

      case IBTEST_CMD_EXIT:
         WorkerSleep(0.1);
         DOUT2(("Propose worker to stop"));
         // Stop(); // stop module

         fRunnable->StopRunnable();
         fRunThread->Join();

         dabc::mgr.StopApplication();
         break;

      case IBTEST_CMD_ALLTOALL:
         return ExecuteAllToAll((double*)fCmdDataBuffer);

      case IBTEST_CMD_CHAOTIC:
//            cmd_res = ExecuteChaotic((int64_t*)cmddata);
         break;

      case IBTEST_CMD_INITMCAST:
//            cmd_res = ExecuteInitMulticast((int64_t*)cmddata);
         break;

      case IBTEST_CMD_MCAST:
//            cmd_res = ExecuteMulticast((int64_t*)cmddata);
         break;
      case IBTEST_CMD_RDMA:
//            cmd_res = ExecuteRDMA((int64_t*)cmddata);
         break;
   }

   return true;
}

void bnet::TransportModule::SlaveMainLoop()
{
   // slave is only good for reaction on the commands
   while (ModuleWorking()) {
      dabc::Buffer buf = Recv(IOPort(), 1.);
      if (buf.null()) continue;

      int cmdid = PreprocessSlaveCommand(buf);

      Send(IOPort(), buf);

      ExecuteSlaveCommand(cmdid);
   }
}

bool bnet::TransportModule::MasterCloseConnections()
{
   dabc::TimeStamp tm = dabc::Now();

   if (!MasterCommandRequest(IBTEST_CMD_CLOSEQP)) return false;

   // this is just ensure that all nodes are on ready state
   if (!MasterCommandRequest(IBTEST_CMD_TEST)) return false;

   DOUT0(("Comm ports are closed in %5.3fs", tm.SpentTillNow()));

   return true;
}

bool bnet::TransportModule::MasterTimeSync(bool dosynchronisation, int numcycles, bool doscaling)
{
   // first request all salves to start sync

   int maxqueuelen = 20;
   if (maxqueuelen > numcycles) maxqueuelen = numcycles;
   int nrepeat = numcycles/maxqueuelen;
   if (nrepeat==0) nrepeat=1;
   numcycles = nrepeat * maxqueuelen;

   int sync_lid = 0;

   int64_t pars[4];
   pars[0] = numcycles;
   pars[1] = maxqueuelen;
   pars[2] = sync_lid;
   pars[3] = nrepeat;

   if (!MasterCommandRequest(IBTEST_CMD_TIMESYNC, pars, sizeof(pars))) return false;

   dabc::TimeStamp starttm = dabc::Now();

   for(int nremote=1;nremote<NumNodes();nremote++) {

      if (!fActiveNodes[nremote]) continue;

      DOUT2(("Start with node %d", nremote));

      // configure runnable with parameters, later used for time sync
      fRunnable->ConfigSync(true, numcycles, dosynchronisation, doscaling);

      if (!fRunnable->RunSyncLoop(true, nremote, sync_lid, maxqueuelen, nrepeat)) return false;
   }

   DOUT0(("Tyme sync done in %5.4f sec", starttm.SpentTillNow()));

   return MasterCommandRequest(IBTEST_CMD_TEST);
}


bool bnet::TransportModule::ExecuteAllToAll(double* arguments)
{
   int bufsize = arguments[0];
//   int NumUsedNodes = arguments[1];
   int max_sending_queue = arguments[2];
   int max_receiving_queue = arguments[3];
   double starttime = arguments[4];
   double stoptime = arguments[5];
   int patternid = arguments[6];
   double datarate = arguments[7];
   double ratemeterinterval = arguments[8];
   bool canskipoperation = arguments[9] > 0;

   // when receive operation should be prepared before send is started
   // we believe that 100 microsec is enough
   double recv_pre_time = 0.0001;

   // this is time required to deliver operation to the runnable
   // 0.5 ms should be enough
   double submit_pre_time = 0.0005;

   AllocResults(14);

   DOUT2(("ExecuteAllToAll - start"));

   dabc::Ratemeter sendrate, recvrate, multirate;
   dabc::Ratemeter** fIndividualRates = 0;
   double fMeasureInterval = 0; // set 0 to disable individual time measurements

   if (fMeasureInterval>0) {
       fIndividualRates = new dabc::Ratemeter*[NumNodes()];
       for (int n=0;n<NumNodes();n++) {
          fIndividualRates[n] = 0;
          if (n!=Node()) {
              fIndividualRates[n] = new dabc::Ratemeter;
              fIndividualRates[n]->DoMeasure(fMeasureInterval, 10000, starttime);
          }
       }
   }

   // this is interval for ratemeter, which can be requested later from master
   if (ratemeterinterval>0) {
       if (fRecvRatemeter==0) fRecvRatemeter = new dabc::Ratemeter;
       if (fSendRatemeter==0) fSendRatemeter = new dabc::Ratemeter;
       if (fWorkRatemeter==0) fWorkRatemeter = new dabc::Ratemeter;

       long npoints = lrint((stoptime-starttime) / ratemeterinterval);
       if (npoints>1000000) npoints=1000000;
       fRecvRatemeter->DoMeasure(ratemeterinterval, npoints, starttime);
       fSendRatemeter->DoMeasure(ratemeterinterval, npoints, starttime);
       fWorkRatemeter->DoMeasure(ratemeterinterval, npoints, starttime);
   } else {
      if (fRecvRatemeter) { delete fRecvRatemeter; fRecvRatemeter = 0; }
      if (fSendRatemeter) { delete fSendRatemeter; fSendRatemeter = 0; }
      if (fWorkRatemeter) { delete fWorkRatemeter; fWorkRatemeter = 0; }
   }

   // schedule variables

   for (int lid=0;lid<NumLids();lid++)
      for (int n=0;n<NumNodes();n++) {
         if (SendQueue(lid,n)>0) EOUT(("!!!! Problem in SendQueue[%d,%d]=%d",lid,n,SendQueue(lid,n)));
         if (RecvQueue(lid,n)>0) EOUT(("!!!! Problem in RecvQueue[%d,%d]=%d",lid,n,RecvQueue(lid,n)));
      }

   // schstep is in seconds, but datarate in MBytes/sec, therefore 1e-6
   double schstep = 1e-6 * bufsize / datarate;

   switch(patternid) {
      case 1: // send data to next node
         fSendSch.Allocate(1, NumNodes());
         for (int n=0;n<NumNodes();n++)
            fSendSch.Item(0,n).Set((n+1) % NumNodes(), n % NumLids());
         fSendSch.SetEndTime(schstep); // this is end of the schedule
         break;

      case 2: // send always with shift num/2
         fSendSch.Allocate(1, NumNodes());
         for (int n=0;n<NumNodes();n++)
            fSendSch.Item(0,n).Set((n+NumNodes()/2) % NumNodes(), n % NumLids());
         fSendSch.SetEndTime(schstep); // this is end of the schedule
         break;

      case 3: // one to all
         fSendSch.Allocate(NumNodes()-1, NumNodes());
         for (int nslot=0; nslot<NumNodes()-1;nslot++) {
            fSendSch.Item(nslot,NumNodes()-1).Set(nslot, nslot % NumLids());
            fSendSch.SetTimeSlot(nslot, schstep*nslot);
         }

         fSendSch.SetEndTime(schstep*(NumNodes()-1)); // this is end of the schedule
         break;

      case 4:    // all-to-one
         fSendSch.Allocate(NumNodes()-1, NumNodes());
         for (int nslot=0; nslot<NumNodes()-1;nslot++) {
            fSendSch.Item(nslot,nslot).Set(NumNodes()-1, nslot % NumLids());
            fSendSch.SetTimeSlot(nslot, schstep*nslot);
         }

         fSendSch.SetEndTime(schstep*(NumNodes()-1)); // this is end of the schedule
         break;

      case 5: // even nodes sending data to odd nodes
         fSendSch.Allocate(1, NumNodes());
         for (int n=0;n<NumNodes();n++)
            if ((n % 2 == 0) && (n<NumNodes()-1))
               fSendSch.Item(0,n).Set(n+1, n % NumLids());

         fSendSch.SetEndTime(schstep); // this is end of the schedule
         break;

      case 7:  // all-to-all from file
         if ((fPreparedSch.numSenders() == NumNodes()) && (fPreparedSch.numLids() <= NumLids())) {
            fPreparedSch.CopyTo(fSendSch);
            fSendSch.FillRegularTime(schstep / fPreparedSch.numSlots() * (NumNodes()-1));
         } else {
            fSendSch.Allocate(NumNodes()-1, NumNodes());
            fSendSch.FillRoundRoubin(0, schstep);
         }
         break;


      default:
         // default, all-to-all round-robin schedule
         fSendSch.Allocate(NumNodes()-1, NumNodes());
         fSendSch.FillRoundRoubin(0, schstep);
         break;
   }

   DOUT4(("ExecuteAllToAll: Define schedule"));

   fSendSch.FillReceiveSchedule(fRecvSch);

   for (unsigned node=0; node<fActiveNodes.size(); node++)
      if (!fActiveNodes[node]) {
         fSendSch.ExcludeInactiveNode(node);
         fRecvSch.ExcludeInactiveNode(node);
      }

   if (patternid==6) // disable receive operation 1->0 to check that rest data will be transformed
      for (int nslot=0; nslot<fRecvSch.numSlots(); nslot++)
         if (fRecvSch.Item(nslot, 0).node == 1) {
            DOUT0(("Disable recv operation 1->0"));
            fRecvSch.Item(nslot, 0).Reset();
          }

   double send_basetm(starttime), recv_basetm(starttime);
   int send_slot(-1), recv_slot(-1);

   bool dosending = fSendSch.ShiftToNextOperation(Node(), send_basetm, send_slot);
   bool doreceiving = fRecvSch.ShiftToNextOperation(Node(), recv_basetm, recv_slot);

   int send_total_limit = TestNumBuffers();
   int recv_total_limit = TestNumBuffers();

   // gave most buffers for receiving
   if (dosending && doreceiving) {
      send_total_limit = TestNumBuffers() / 10 - 1;
      recv_total_limit = TestNumBuffers() / 10 * 9 - 1;
   }

//   if (IsMaster()) {
//      fSendSch.Print(0);
//      fRecvSch.Print(0);
//   }

   dabc::Average send_start, send_compl, recv_compl, loop_time;

   double lastcmdchecktime = fStamping();

   DOUT0(("ExecuteAllToAll: Starting dosend %s dorecv %s remains: %5.3fs", DBOOL(dosending), DBOOL(doreceiving), starttime - fStamping()));

   // counter for must have send operations over each channel
   IbTestIntMatrix sendcounter(NumLids(), NumNodes());

   // actual received id from sender
   IbTestIntMatrix recvcounter(NumLids(), NumNodes());

   // number of recv skip operations one should do
   IbTestIntMatrix recvskipcounter(NumLids(), NumNodes());

   sendcounter.Fill(0);
   recvcounter.Fill(-1);
   recvskipcounter.Fill(0);

   long numlostpackets(0);

   long totalrecvpackets(0), numsendpackets(0), numcomplsend(0);

   int64_t sendmulticounter = 0;
   int64_t skipmulticounter = 0;
   int64_t numlostmulti = 0;
   int64_t totalrecvmulti = 0;
   int64_t skipsendcounter = 0;

   double last_tm(-1), curr_tm(0);

   dabc::CpuStatistic cpu_stat;

   double cq_waittime = 0.;
   if (patternid==-2) cq_waittime = 0.001;

   int recid(-1);
   OperRec* rec(0);


   while ((curr_tm=fStamping()) < stoptime) {

      if ((last_tm>0) && (last_tm>starttime))
         loop_time.Fill(curr_tm - last_tm);
      last_tm = curr_tm;

      // just indicate that we were active at this time interval
      if (fWorkRatemeter) fWorkRatemeter->Packet(1, curr_tm);

      recid = CheckCompletionQueue(cq_waittime);

      rec = fRunnable->GetRec(recid);

      if (rec!=0) {
         switch (rec->kind) {
            case kind_Send: {
               double sendtime(0);
               rec->buf.CopyTo(&sendtime, sizeof(sendtime));
               numcomplsend++;

               double sendcompltime = fStamping();

               send_start.Fill(rec->is_time - rec->oper_time);

               send_compl.Fill(sendcompltime - sendtime);

               sendrate.Packet(bufsize, sendcompltime);

               if (fSendRatemeter) fSendRatemeter->Packet(bufsize, sendcompltime);
               break;
            }
            case kind_Recv: {
               double mem[2] = {0., 0.};
               rec->buf.CopyTo(mem, sizeof(mem));
               double sendtime = mem[0];
               int sendcnt = (int) mem[1];

               int lost = sendcnt - recvcounter(rec->tgtindx, rec->tgtnode) - 1;
               if (lost>0) {
                  numlostpackets += lost;
                  recvskipcounter(rec->tgtindx, rec->tgtnode) += lost;
               }

               recvcounter(rec->tgtindx, rec->tgtnode) = sendcnt;

               double recv_time = fStamping();

               recv_compl.Fill(recv_time - sendtime);

               recvrate.Packet(bufsize, recv_time);
               if ((fIndividualRates!=0) && (fIndividualRates[rec->tgtnode]!=0))
                  fIndividualRates[rec->tgtnode]->Packet(bufsize, recv_time);

               if (fRecvRatemeter) fRecvRatemeter->Packet(bufsize, recv_time);

               totalrecvpackets++;
               break;
            }
            default:
               EOUT(("Wrong operation kind"));
               break;
         }

         // buffer will be released as well
         fRunnable->ReleaseRec(recid);
      }

      // this is submit of recv operation

      if (doreceiving && (TotalRecvQueue() < recv_total_limit)) {

         double next_recv_time = recv_basetm + fRecvSch.timeSlot(recv_slot) - recv_pre_time;

         curr_tm = fStamping();

         if (next_recv_time - submit_pre_time < curr_tm) {
            int node = fRecvSch.Item(recv_slot, Node()).node;
            int lid = fRecvSch.Item(recv_slot, Node()).lid;
            bool did_submit = false;

            if ((recvskipcounter(lid, node) > 0) && (RecvQueue(lid, node)>0)) {
               // if packets over this channel were lost, we should not try
               // to receive them - they were skipped by sender most probably
               recvskipcounter(lid, node)--;
               did_submit = true;
            } else
            if (RecvQueue(lid, node) < max_receiving_queue) {

               dabc::Buffer buf = FindPool("BnetDataPool")->TakeBuffer(fTestBufferSize);

               if (buf.null()) {
                  EOUT(("Not enough buffers"));
                  return false;
               }

               // DOUT0(("Submit recv operation for buffer %u", buf.GetTotalSize()));

               recid = fRunnable->PrepareOperation(kind_Recv, sizeof(TransportHeader), buf);
               rec = fRunnable->GetRec(recid);

               if (rec==0) {
                  EOUT(("Cannot prepare recv operation"));
                  return false;
               }

               rec->SetTarget(node, lid);
               rec->SetTime(next_recv_time);

               if (!SubmitToRunnable(recid,rec)) {
                  EOUT(("Cannot submit receive operation to lid %d node %d", lid, node));
                  return false;
               }

               did_submit = true;
            } else
            if (canskipoperation) {
               // skip recv operation - other node may be waiting for me
               did_submit = true;
            }

            // shift to next operation, even if previous was not submitted.
            // dangerous but it is like it is - we should continue receiving otherwise input will be blocked
            // we should do here more smart solutions

            if (did_submit)
               doreceiving = fRecvSch.ShiftToNextOperation(Node(), recv_basetm, recv_slot);
         }
      }

      // this is submit of send operation
      if (dosending && (TotalSendQueue() < send_total_limit)) {
         double next_send_time = send_basetm + fSendSch.timeSlot(send_slot);

         curr_tm = fStamping();

         if ((next_send_time - submit_pre_time < curr_tm) && (curr_tm<stoptime-0.1)) {

            int node = fSendSch.Item(send_slot, Node()).node;
            int lid = fSendSch.Item(send_slot, Node()).lid;
            bool did_submit = false;

            if (canskipoperation && ((SendQueue(lid, node) >= max_sending_queue) || (TotalSendQueue() >= 2*max_sending_queue)) ) {
               did_submit = true;
               skipsendcounter++;
            } else
            if ( (SendQueue(lid, node) < max_sending_queue) && (TotalSendQueue() < 2*max_sending_queue) ) {

               dabc::Buffer buf = FindPool("BnetDataPool")->TakeBuffer(fTestBufferSize);

               if (buf.null()) {
                  EOUT(("Not enough buffers"));
                  return false;
               }

               // DOUT0(("Submit send operation for buffer %u", buf.GetTotalSize()));

               double mem[2] = { curr_tm, sendcounter(lid,node) };
               buf.CopyFrom(mem, sizeof(mem));

               recid = fRunnable->PrepareOperation(kind_Send, sizeof(TransportHeader), buf);
               rec = fRunnable->GetRec(recid);

               if (rec==0) {
                  EOUT(("Cannot prepare send operation"));
                  return false;
               }

               TransportHeader* hdr = (TransportHeader*) rec->header;
               hdr->srcid = Node();    // source node id
               hdr->tgtid = node;      // target node id
               hdr->eventid = 123456;  // event id

               rec->SetTarget(node, lid);
               rec->SetTime(next_send_time);

               if (!SubmitToRunnable(recid,rec)) {
                  EOUT(("Cannot submit receive operation to lid %d node %d", lid, node));
                  return false;
               }

               numsendpackets++;
               did_submit = true;
            }

            // shift to next operation, in some cases even when previous operation was not performed
            if (did_submit) {
               sendcounter(lid,node)++; // this indicates that we perform operation and moved to next counter
               dosending = fSendSch.ShiftToNextOperation(Node(), send_basetm, send_slot);
            }
         }
      }

      if (curr_tm > lastcmdchecktime + 0.1) {
         if (Node()>0 && IOPort()->CanRecv()) {
             // set stop time in 0.1 s to allow completion of all send operation
             if (curr_tm+0.1 < stoptime) stoptime = curr_tm+0.1;
             // disable send operation anyhow
             dosending = false;

             EOUT(("Did emergence stop while command is arrived"));
             // do not do immediate break
             // break;
         }
         lastcmdchecktime = curr_tm;
         if (curr_tm < starttime) cpu_stat.Reset();
      }
   }

   DOUT3(("Total recv queue %ld limit %ld send queue %ld", TotalSendQueue(), recv_total_limit, TotalRecvQueue()));
   DOUT3(("Do recv %s curr_tm %8.7f next_tm %8.7f slot %d node %d lid %d queue %d",
         DBOOL(doreceiving), curr_tm, recv_basetm + fRecvSch.timeSlot(recv_slot), recv_slot,
         fRecvSch.Item(recv_slot, Node()).node, fRecvSch.Item(recv_slot, Node()).lid,
         RecvQueue(fRecvSch.Item(recv_slot, Node()).lid, fRecvSch.Item(recv_slot, Node()).node)));
   DOUT3(("Do send %s curr_tm %8.7f next_tm %8.7f slot %d node %d lid %d queue %d",
         DBOOL(dosending), curr_tm, send_basetm + fSendSch.timeSlot(send_slot), send_slot,
         fSendSch.Item(send_slot, Node()).node, fSendSch.Item(send_slot, Node()).lid,
         SendQueue(fSendSch.Item(send_slot, Node()).lid, fSendSch.Item(send_slot, Node()).node)));

   DOUT3(("Send operations diff %ld: submited %ld, completed %ld", numsendpackets-numcomplsend, numsendpackets, numcomplsend));

   cpu_stat.Measure();

   DOUT5(("CPU utilization = %5.1f % ", cpu_stat.CPUutil()*100.));

   if (numsendpackets!=numcomplsend) {
      EOUT(("Mismatch %d between submitted %ld and completed send operations %ld",numsendpackets-numcomplsend, numsendpackets, numcomplsend));

      for (int lid=0;lid<NumLids(); lid++)
        for (int node=0;node<NumNodes();node++) {
          if ((node==Node()) || !fActiveNodes[node]) continue;
          DOUT5(("   Lid:%d Node:%d RecvQueue = %d SendQueue = %d recvskp %d", lid, node, RecvQueue(lid,node), SendQueue(lid,node), recvskipcounter(lid, node)));
       }
   }

   fResults[0] = sendrate.GetRate();
   fResults[1] = recvrate.GetRate();
   fResults[2] = send_start.Mean();
   fResults[3] = send_compl.Mean();
   fResults[4] = numlostpackets;
   fResults[5] = totalrecvpackets;
   fResults[6] = numlostmulti;
   fResults[7] = totalrecvmulti;
   fResults[8] = multirate.GetRate();
   fResults[9] = cpu_stat.CPUutil();
   fResults[10] = loop_time.Mean();
   fResults[11] = loop_time.Max();
   fResults[12] = (numsendpackets+skipsendcounter) > 0 ? 1.*skipsendcounter / (numsendpackets + skipsendcounter) : 0.;
   fResults[13] = recv_compl.Mean();

   DOUT5(("Multi: total %ld lost %ld", totalrecvmulti, numlostmulti));

   if (sendmulticounter+skipmulticounter>0)
      DOUT0(("Multi send: %ld skipped %ld (%5.3f)",
             sendmulticounter, skipmulticounter,
             100.*skipmulticounter/(1.*sendmulticounter+skipmulticounter)));

   if (fIndividualRates!=0) {

      char fname[100];
      sprintf(fname,"recv_rates_%d.txt",Node());

      dabc::Ratemeter::SaveRatesInFile(fname, fIndividualRates, NumNodes(), true);

      for (int n=0;n<NumNodes();n++)
         delete fIndividualRates[n];
      delete[] fIndividualRates;
   }

   return true;
}

bool bnet::TransportModule::MasterAllToAll(int full_pattern,
                                        int bufsize,
                                        double duration,
                                        int datarate,
                                        int max_sending_queue,
                                        int max_receiving_queue,
                                        bool fromperfmtest)
{
   // pattern == 0 round-robin schedule
   // pattern == 1 fixed target (shift always 1)
   // pattern == 2 fixed target (shift always NumNodes / 2)
   // pattern == 3 one-to-all, one node sending, other receiving
   // pattern == 4 all-to-one, all sending, one receiving
   // pattern == 5 even nodes sending data to next odd node (0->1, 2->3, 4->5 and so on)
   // pattern == 6 same as 0, but node0 will not receive data from node1 (check if rest data will be transformed)
   // pattern == 7 schedule read from the file
   // pattern = 10..17, same as 0..7, but allowed to skip send/recv packets when queue full

   bool allowed_to_skip = false;
   int pattern = full_pattern;

   if ((pattern>=0)  && (pattern<20)) {
      allowed_to_skip = full_pattern / 10 > 0;
      pattern = pattern % 10;
   }

   const char* pattern_name = "---";
   switch (pattern) {
      case 1:
         pattern_name = "ALL-TO-ALL (shift 1)";
         break;
      case 2:
         pattern_name = "ALL-TO-ALL (shift n/2)";
         break;
      case 3:
         pattern_name = "ONE-TO-ALL";
         break;
      case 4:
         pattern_name = "ALL-TO-ONE";
         break;
      case 5:
         pattern_name = "EVEN-TO-ODD";
         break;
      case 7:
         pattern_name = "FILE_BASED";
         break;
      default:
         pattern_name = "ALL-TO-ALL";
         break;
   }


   double arguments[10];

   arguments[0] = bufsize;
   arguments[1] = NumNodes();
   arguments[2] = max_sending_queue;
   arguments[3] = max_receiving_queue;
   arguments[4] = fStamping() + fCmdDelay + 1.; // start 1 sec after cmd is delivered
   arguments[5] = arguments[4] + duration;      // stopping time
   arguments[6] = pattern;
   arguments[7] = datarate;
   arguments[8] = fTrendingInterval; // interval for ratemeter, 0 - off
   arguments[9] = allowed_to_skip ? 1 : 0;

   DOUT0(("====================================="));
   DOUT0(("%s pattern:%d size:%d rate:%d send:%d recv:%d nodes:%d canskip:%s",
         pattern_name, pattern, bufsize, datarate, max_sending_queue, max_receiving_queue, NumNodes(), DBOOL(allowed_to_skip)));

   if (!MasterCommandRequest(IBTEST_CMD_ALLTOALL, arguments, sizeof(arguments))) return false;

   if (!ExecuteAllToAll(arguments)) return false;

   int setsize = 14;
   double allres[setsize*NumNodes()];
   for (int n=0;n<setsize*NumNodes();n++) allres[n] = 0.;

   if (!MasterCommandRequest(IBTEST_CMD_COLLECT, 0, 0, allres, setsize*sizeof(double))) return false;

//   DOUT((1,"Results of all-to-all test"));
   DOUT0(("  # |      Node |   Send |   Recv |   Start |S_Compl|R_Compl| Lost | Skip | Loop | Max ms | CPU  | Multicast "));
   double sum1send(0.), sum2recv(0.), sum3multi(0.), sum4cpu(0.);
   int sumcnt = 0;
   bool isok = true;
   bool isskipok = true;
   double totallost = 0, totalrecv = 0;
   double totalmultilost = 0, totalmulti = 0, totalskipped = 0;

   char sbuf1[100], cpuinfobuf[100];

   for (int n=0;n<NumNodes();n++) {
       if (allres[n*setsize+12]>=0.099)
          sprintf(sbuf1,"%4.0f%s",allres[n*setsize+12]*100.,"%");
       else
          sprintf(sbuf1,"%4.1f%s",allres[n*setsize+12]*100.,"%");

       if (allres[n*setsize+9]>=0.099)
          sprintf(cpuinfobuf,"%4.0f%s",allres[n*setsize+9]*100.,"%");
       else
          sprintf(cpuinfobuf,"%4.1f%s",allres[n*setsize+9]*100.,"%");

       DOUT0(("%3d |%10s |%7.1f |%7.1f |%8.1f |%6.0f |%6.0f |%5.0f |%s |%5.2f |%7.2f |%s |%5.1f (%5.0f)",
             n, dabc::mgr()->GetNodeName(n).c_str(),
           allres[n*setsize+0], allres[n*setsize+1], allres[n*setsize+2]*1e6, allres[n*setsize+3]*1e6, allres[n*setsize+13]*1e6, allres[n*setsize+4], sbuf1, allres[n*setsize+10]*1e6, allres[n*setsize+11]*1e3, cpuinfobuf, allres[n*setsize+8], allres[n*setsize+7]));
       totallost += allres[n*setsize+4];
       totalrecv += allres[n*setsize+5];
       totalmultilost += allres[n*setsize+6];
       totalmulti += allres[n*setsize+7];

       // check if send starts in time
       if (allres[n*setsize+2] > 20.) isok = false;

       sumcnt++;
       sum1send += allres[n*setsize+0];
       sum2recv += allres[n*setsize+1];
       sum3multi += allres[n*setsize+8]; // Multi rate
       sum4cpu += allres[n*setsize+9]; // CPU util
       totalskipped += allres[n*setsize+12];
       if (allres[n*setsize+12]>0.03) isskipok = false;
   }
   DOUT0(("          Total |%7.1f |%7.1f | Skip: %4.0f/%4.0f = %3.1f percent",
          sum1send, sum2recv, totallost, totalrecv, 100*(totalrecv>0. ? totallost/(totalrecv+totallost) : 0.)));
   if (totalmulti>0)
      DOUT0(("Multicast %4.0f/%4.0f = %7.6f%s  Rate = %4.1f",
         totalmultilost, totalmulti, totalmultilost/totalmulti*100., "%", sum3multi/sumcnt));

   MasterCleanup();

   totalskipped = totalskipped / sumcnt;
   if (totalskipped > 0.01) isskipok = false;

   AllocResults(8);
   fResults[0] = sum1send / sumcnt;
   fResults[1] = sum2recv / sumcnt;

   if ((pattern==3) || (pattern==4)) {
      fResults[0] = fResults[0] * NumNodes();
      fResults[1] = fResults[1] * NumNodes();
   }

   if ((pattern>=0) && (pattern!=5)) {
      if (fResults[0] < datarate - 5) isok = false;
      if (!isskipok) isok = false;
   }

   fResults[2] = isok ? 1. : 0.;
   fResults[3] = (totalrecv>0 ? totallost/totalrecv : 0.);

   if (totalmulti>0) {
      fResults[4] = sum3multi/sumcnt;
      fResults[5] = totalmultilost/totalmulti;
   }
   fResults[6] = sum4cpu/sumcnt;
   fResults[7] = totalskipped;

   return true;
}

int bnet::TransportModule::ProcessAskQueue(void* tgt)
{
   // method is used to calculate how many queues entries are existing

   dabc::TimeStamp tm = dabc::Now();

   while (!tm.Expired(0.01)) {
      int recid = CheckCompletionQueue(0.);
      if (recid<0) break;
      fRunnable->ReleaseRec(recid);
   }
   int32_t* mem = (int32_t*) tgt;
   for(int node=0;node<NumNodes();node++)
      for(int nlid=0;nlid<NumLids();nlid++) {
         *mem++ = SendQueue(nlid, node);
         *mem++ = RecvQueue(nlid, node);
      }

   return NumNodes()*NumLids()*2*sizeof(int32_t);
}

bool bnet::TransportModule::MasterCleanup()
{
   double tm = fStamping();

   int blocksize = NumNodes()*NumLids()*2*sizeof(uint32_t);

   void* recs = malloc(NumNodes() * blocksize);
   memset(recs, 0, NumNodes() * blocksize);

   // own records will be add automatically
   if (!MasterCommandRequest(IBTEST_CMD_ASKQUEUE, 0, 0, recs, blocksize)) {
      EOUT(("Cannot collect queue sizes"));
      free(recs);
      return false;
   }

   // now we should resort connection records, loop over targets

   DOUT0(("First collect all queue sizes"));

   void* conn = malloc(NumNodes() * blocksize);
   memset(conn, 0, NumNodes()*blocksize);

   int32_t* src = (int32_t*) recs;
   int32_t* tgt = (int32_t*) conn;

   int allsend(0), allrecv(0);

   for(int node1=0;node1<NumNodes();node1++)
      for (int node2=0;node2<NumNodes();node2++)
         for(int nlid=0;nlid<NumLids();nlid++) {
            int tgtindx = node1*NumNodes()*NumLids()*2 + node2*NumLids()*2 + nlid*2;
            int srcindx = node2*NumNodes()*NumLids()*2 + node1*NumLids()*2 + nlid*2;
            // send operation, which should be executed from node1 -> node2
            tgt[tgtindx] = src[srcindx + 1];
            // recv operation, which should be executed on node1 for node2
            tgt[tgtindx + 1] = src[srcindx];

            allsend += tgt[tgtindx];
            allrecv += tgt[tgtindx + 1];
      }


   DOUT0(("All send %d and recv %d operation", allsend, allrecv));

   bool res(true);
   if ((allsend!=0) || (allrecv!=0))
       res = MasterCommandRequest(IBTEST_CMD_CLEANUP, conn, -blocksize);

   free(recs);

   free(conn);

   // for syncronisation
   if (!MasterCommandRequest(IBTEST_CMD_TEST)) res = false;

   DOUT0(("Queues are cleaned in %5.4f s res = %s", fStamping() - tm, DBOOL(res)));

   return res;
}

bool bnet::TransportModule::ProcessCleanup(int32_t* pars)
{
   dabc::TimeStamp tm = dabc::Now();

   int recid(-1);
   OperRec* rec(0);
   int maxqueue(5);

   // execute at maximum 7 seconds
   while (!tm.Expired(7.)) {
      recid = CheckCompletionQueue(0.001);

      rec = fRunnable->GetRec(recid);

      if (rec!=0) {
         // buffer will be released as well
         fRunnable->ReleaseRec(recid);
      }

      bool isany = false;

      for (int node=0;node<NumNodes();node++)
         for (int lid=0;lid<NumLids();lid++) {
            int* entry = pars + node*NumLids()*2 + lid*2;

            if ((entry[0]>0) || (entry[1]>0)) isany = true;

            if ((entry[0]>0) && (SendQueue(lid,node)<maxqueue)) {
               // perform send operation
               dabc::Buffer buf = FindPool("BnetDataPool")->TakeBuffer(fTestBufferSize);

               if (buf.null()) {
                  EOUT(("Not enough buffers"));
                  return false;
               }

               recid = fRunnable->PrepareOperation(kind_Send, sizeof(TransportHeader), buf);
               rec = fRunnable->GetRec(recid);

               if (rec==0) {
                  EOUT(("Cannot prepare send operation"));
                  return false;
               }

               TransportHeader* hdr = (TransportHeader*) rec->header;
               hdr->srcid = Node();    // source node id
               hdr->tgtid = node;      // target node id
               hdr->eventid = 123456;  // event id

               rec->SetTarget(node, lid);
               rec->SetImmediateTime();

               if (!SubmitToRunnable(recid,rec)) {
                  EOUT(("Cannot submit receive operation to lid %d node %d", lid, node));
                  return false;
               }

               entry[0]--;
            }

            if ((entry[1]>0) && (RecvQueue(lid,node)<maxqueue)) {
               // perform recv operation

               dabc::Buffer buf = FindPool("BnetDataPool")->TakeBuffer(fTestBufferSize);

               if (buf.null()) {
                  EOUT(("Not enough buffers"));
                  return false;
               }

               recid = fRunnable->PrepareOperation(kind_Recv, sizeof(TransportHeader), buf);
               rec = fRunnable->GetRec(recid);

               if (rec==0) {
                  EOUT(("Cannot prepare recv operation"));
                  return false;
               }

               rec->SetTarget(node, lid);
               rec->SetImmediateTime();

               if (!SubmitToRunnable(recid,rec)) {
                  EOUT(("Cannot submit receive operation to lid %d node %d", lid, node));
                  return false;
               }

               entry[1]--;
            }
         }

      // no any operation to execute
      if (!isany && (TotalSendQueue()==0) && (TotalRecvQueue()==0)) break;
   }

   if ((TotalSendQueue()!=0) || (TotalRecvQueue()!=0))
      EOUT(("Cleanup failed still %d send and %d recv operations", TotalSendQueue(), TotalRecvQueue()));

   return true;
}

void bnet::TransportModule::PerformTimingTest()
{
   for (int n=0; n<5; n++) {
      DOUT0(("SLEEP 10 sec N=%d", n));
      WorkerSleep(10.);
      if (n%5 == 0)
         MasterTimeSync(true, 200, true);
      else
         MasterTimeSync(false, 200, false);
   }
}

void bnet::TransportModule::PerformNormalTest()
{
//   MasterTiming();

//   DOUT0(("SLEEP 1 sec"));
//   WorkerSleep(1.);

   int outq = Cfg("TestOutputQueue").AsInt(5);
   int inpq = Cfg("TestInputQueue").AsInt(10);
   int buffersize = Cfg("TestBufferSize").AsInt(128*1024);
   int rate = Cfg("TestRate").AsInt(1000);
   int testtime = Cfg("TestTime").AsInt(10);
   int testpattern = Cfg("TestPattern").AsInt(0);

//   fTrendingInterval = 0.0002;
   MasterAllToAll(testpattern, buffersize, testtime, rate, outq, inpq);
//   if (fWorkRatemeter) fWorkRatemeter->SaveInFile("mywork1K.txt");
//   fTrendingInterval = 0;

   MasterAllToAll(testpattern, buffersize, testtime, rate, outq, inpq);

//   MasterAllToAll(0, 2*1024,   5., 100., 5, 10);

//   MasterAllToAll(-1, 128*1024, 5., 1000., 5, 10);

//   MasterAllToAll(0,  128*1024, 5., 500., 5, 10);

//   MasterAllToAll(-1, 1024*1024, 5., 900., 2, 4);

//   MasterAllToAll(0, 1024*1024, 5., 2300., 2, 4);
}

void bnet::TransportModule::MainLoop()
{
   if (IsSlave()) {
      SlaveMainLoop();
      return;
   }

   DOUT0(("Entering mainloop master: %s test = %s", DBOOL(IsMaster()), fTestKind.c_str()));

   // here we are doing our test sequence (like in former verbs tests)

   MasterCollectActiveNodes();

   CalibrateCommandsChannel();

   if (fTestKind == "OnlyConnect") {

      DOUT0(("----------------- TRY ONLY COMMAND CHANNEL ------------------- "));

      CalibrateCommandsChannel(30);

      MasterCallExit();

      return;
   }


   DOUT0(("----------------- TRY CONN ------------------- "));

   MasterConnectQPs();

   DOUT0(("----------------- DID CONN !!! ------------------- "));

   MasterTimeSync(true, 200, false);

   if (fTestKind != "Simple") {

      DOUT0(("SLEEP 10 sec"));
      WorkerSleep(10.);

      MasterTimeSync(true, 200, true);

      if (fTestKind == "SimpleSync") {
         DOUT0(("Sleep 10 sec more before end"));
         WorkerSleep(10.);
      } else
      if (fTestKind == "TimeSync") {
         PerformTimingTest();
      } else {
         PerformNormalTest();
      }

      MasterTimeSync(false, 500, false);
   }

   MasterCloseConnections();

   MasterCallExit();

}


