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
#include "bnet/BuilderModule.h"

#include "dabc/logging.h"
#include "dabc/PoolHandle.h"
#include "dabc/Command.h"
#include "dabc/Port.h"
#include "dabc/Parameter.h"

#include "bnet/common.h"

bnet::BuilderModule::BuilderModule(const char* name, dabc::Command cmd) :
   dabc::ModuleSync(name, cmd),
   fInpPool(0),
   fOutPool(0),
   fNumSenders(1)
{
   SetSyncCommands(true);

   fOutPool = CreatePoolHandle(Cfg(dabc::xmlOutputPoolName, cmd).AsStr(bnet::EventPoolName));

   CreateOutput("Output", fOutPool, BuilderOutQueueSize);

   fInpPool = CreatePoolHandle(Cfg(dabc::xmlInputPoolName, cmd).AsStr(bnet::TransportPoolName));

   CreateInput("Input", fInpPool, BuilderInpQueueSize);

   fOutBufferSize = Cfg(xmlEventBuffer,cmd).AsInt(2048);

   CreatePar(parSendMask).SetStr("xxxx");
}

bnet::BuilderModule::~BuilderModule()
{
   for (unsigned n=0; n<fBuffers.size(); n++)
      dabc::Buffer::Release(fBuffers[n]);
   fBuffers.clear();
}

void bnet::BuilderModule::MainLoop()
{
   DOUT3(("In builder %u buffers collected",  fBuffers.size()));

   fNumSenders = bnet::NodesVector(Par(parSendMask).AsStdStr()).size();

   while (ModuleWorking()) {

      bool iseol = false;

      // read N buffers from input
      while (fBuffers.size() < (unsigned) fNumSenders) {
         dabc::Buffer* buf = Recv(Input(0));
         if (buf==0)
            EOUT(("Cannot read buffer from input"));
         else {
            fBuffers.push_back(buf);

            if (buf->GetTypeId() == dabc::mbt_EOL) {
               iseol = true;
               if (fBuffers.size() > 1) EOUT(("EOL buffer is not first !!!!"));
               break;
            }
         }
      }

      if (iseol) {
         DOUT1(("Send EOL buffer to the output"));
         dabc::BufferGuard eolbuf = fOutPool->TakeEmptyBuffer();
         eolbuf()->SetTypeId(dabc::mbt_EOL);
         Send(Output(0), eolbuf);
      } else
         // produce output
         DoBuildEvent(fBuffers);

      // release N buffers
      for (unsigned n=0;n<fBuffers.size();n++)
         dabc::Buffer::Release(fBuffers[n]);
      fBuffers.clear();
   }

}
