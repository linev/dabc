#ifndef BNET_TransportModule
#define BNET_TransportModule

#include "dabc/ModuleAsync.h"


#include "dabc/timing.h"
#include "dabc/statistic.h"
#include "dabc/threads.h"
#include "dabc/CommandsQueue.h"

#include "bnet/defines.h"
#include "bnet/Schedule.h"
#include "bnet/TransportRunnable.h"

struct ScheduleEntry {
    int    node;
    int    lid;
    double time;
};

namespace bnet {

class TransportCmd : public dabc::Command {

   DABC_COMMAND(TransportCmd, "TransportCmd");

   TransportCmd(int id, double timeout = - 1.) :
      dabc::Command(CmdName())
   {
      SetInt("CmdId", id);
      if (timeout>0.) SetDouble("CmdTimeout", timeout);
   }

   int GetId() const { return GetInt("CmdId"); }
   double GetTimeout() { return GetDouble("CmdTimeout", 5.); }

   void SetSyncArg(int nrepeat, bool dosync, bool doscale)
   {
      SetInt("NRepeat", nrepeat);
      SetBool("DoSync", dosync);
      SetBool("DoScale", doscale);
   }

   int GetNRepeat() const { return GetInt("NRepeat", 1); }
   bool GetDoSync() const { return GetBool("DoSync", false); }
   bool GetDoScale() const { return GetBool("DoScale", false); }
};


class TransportModule : public dabc::ModuleAsync {

   protected:
      int                 fNodeNumber;
      int                 fNumNodes;
      int                 fNumLids;

      int                 fCmdBufferSize; // size of buffer to send/receive command information
      int                 fCmdDataSize;
      char*               fCmdDataBuffer;

      TransportRunnable*  fRunnable;    // runnable where scheduled transfer is implemented
      dabc::PosixThread*  fRunThread;   // special thread where runnable is executed

      /** \brief symbolic name of global test which should be performed. Can be:
       *     "TimeSync" - long test of the time synchronization stability
       *
       *     */
      std::string         fTestKind;

      /** Name of the file with the schedule */
      std::string         fTestScheduleFile;

      double*            fResults;

      double            fCmdDelay;

      int               *fSendQueue[IBTEST_MAXLID];    // size of individual sending queue
      int               *fRecvQueue[IBTEST_MAXLID];    // size of individual receiving queue
      long               fTotalSendQueue;
      long               fTotalRecvQueue;
      long               fTestNumBuffers;

      TimeStamping       fStamping;     // copy of stamping from runnable

      double            fTrendingInterval;   //!< interval (in seconds) for send/recv rate trending

      /** array indicating active nodes in the system,
       *  Accumulated in the beginning by the master and distributed to all other nodes.
       *  Should be used in the most tests */
      std::vector<bool>  fActiveNodes;

      /** Schedule, loaded from the file */
      IBSchedule  fPreparedSch;

      /** Send and receive schedule, used in all-to-all tests */
      IBSchedule  fSendSch;
      IBSchedule  fRecvSch;


      // ================== this is new members for async module ==========================

      enum ModuleCmdState { mcmd_None, mcmd_Exec };

      int fRunningCmdId; // id of running command, 0 if no command is runs
      ModuleCmdState fCmdState; // that is now happens with the command
      dabc::TimeStamp fCmdStartTime;  // time when command should finish its execution
      dabc::TimeStamp fCmdEndTime;  // time when command should finish its execution
      std::vector<bool> fCmdReplies; // indicates if cuurent cmd was replied
      dabc::Average fCmdTestLoop; // average time of test loop
      void*  fCmdAllResults;   // buffer where replies from all nodes collected
      int    fCmdResultsPerNode;  // how many data in replied is expected

      void  *fConnRawData, *fConnSortData; // buffers used during connections;
      dabc::TimeStamp fConnStartTime;  // time when connection was starting

      int64_t    fSyncArgs[6];  // arguments for time sync
      int       fSlaveSyncNode;  // current slave node for time sync
      dabc::TimeStamp fSyncStartTime;  // time when connection was starting

      dabc::CommandsQueue  fCmdsQueue; // commands submitted for execution

      TransportCmd      fCurrentCmd; // currently executed command

      dabc::ModuleItem*  fReplyItem; // item is used to generate events from runnable


      // these all about data transfer ....

      int                 fTestBufferSize;
      double             fTestStartTime, fTestStopTime; // start/stop time for data transfer
      int                 fSendSlotIndx, fRecvSlotIndx;
      double              fSendBaseTime, fRecvBaseTime;
      bool                fDoSending, fDoReceiving;

      dabc::Ratemeter    fWorkRate;
      dabc::Ratemeter    fSendRate;
      dabc::Ratemeter    fRecvRate;



      virtual void ProcessInputEvent(dabc::Port* port);
      virtual void ProcessOutputEvent(dabc::Port* port);
      virtual void ProcessTimerEvent(dabc::Timer* timer);
      virtual void ProcessUserEvent(dabc::ModuleItem* item, uint16_t evid);

      void ProcessNextSlaveInputEvent();

      bool RequestMasterCommand(int cmdid, void* cmddata = 0, int cmddatasize = 0, void* allresults = 0, int resultpernode = 0);

      bool ProcessReplyBuffer(int nodeid, dabc::Buffer buf);

      void PrepareSyncLoop(int tgtnode);

      void ActivateAllToAll(double* buff);

      void CompleteRunningCommand(int res = dabc::cmd_true);


      void ProcessSendCompleted(int recid, OperRec* rec);
      void ProcessRecvCompleted(int recid, OperRec* rec);



      // ===================================================================================

      int Node() const { return fNodeNumber; }
      int NumNodes() const { return fNumNodes; }
      int NumLids() const { return fNumLids; }
      bool IsSlave() const { return Node() > 0; }
      bool IsMaster() const { return Node()==0; }

      void AllocResults(int size);

      inline int SendQueue(int lid, int node) const { return (lid>=0) && (lid<NumLids()) && (node>=0) && (node<NumNodes()) && fSendQueue[lid] ? fSendQueue[lid][node] : 0; }
      inline int RecvQueue(int lid, int node) const { return (lid>=0) && (lid<NumLids()) && (node>=0) && (node<NumNodes()) && fRecvQueue[lid] ? fRecvQueue[lid][node] : 0; }

      inline long TotalSendQueue() const { return fTotalSendQueue; }
      inline long TotalRecvQueue() const { return fTotalRecvQueue; }
      inline long TestNumBuffers() const { return fTestNumBuffers; }

      bool SubmitToRunnable(int recid, OperRec* rec);

      int PreprocessTransportCommand(dabc::Buffer& buf);

      bool MasterCollectActiveNodes();

      bool MasterCommandRequest(int cmdid, void* cmddata = 0, int cmddatasize = 0, void* allresults = 0, int resultpernode = 0);

      bool CalibrateCommandsChannel(int nloop = 10);

      bool MasterCloseConnections();

      bool MasterCallExit();

      bool ExecuteAllToAll(double* arguments);

      bool MasterAllToAll(int pattern,
                          int bufsize,
                          double duration,
                          int datarate,
                          int max_sending_queue,
                          int max_recieving_queue,
                          bool fromperfmtest = false);

      int ProcessAskQueue(void* tgt);

      bool MasterCleanup();
      bool ProcessCleanup(int32_t* pars);

      void PerformNormalTest();

   public:
      TransportModule(const char* name, dabc::Command cmd);

      virtual ~TransportModule();

      virtual int ExecuteCommand(dabc::Command cmd);

      virtual void MainLoop();

      virtual void BeforeModuleStart();

      virtual void AfterModuleStop();
};

}

#endif
