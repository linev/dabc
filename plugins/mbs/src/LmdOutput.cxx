// $Id$

/************************************************************
 * The Data Acquisition Backbone Core (DABC)                *
 ************************************************************
 * Copyright (C) 2009 -                                     *
 * GSI Helmholtzzentrum fuer Schwerionenforschung GmbH      *
 * Planckstr. 1, 64291 Darmstadt, Germany                   *
 * Contact:  http://dabc.gsi.de                             *
 ************************************************************
 * This software can be used under the GPL license          *
 * agreements as stated in LICENSE.txt file                 *
 * which is part of the distribution.                       *
 ************************************************************/

#include "mbs/LmdOutput.h"

#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>

#if defined(__MACH__) /* Apple OSX section */
#include <machine/endian.h>
#else
#include <endian.h>
#endif

#include "dabc/Manager.h"

#include "mbs/Iterator.h"


mbs::LmdOutput::LmdOutput(const dabc::Url& url) :
   dabc::FileOutput(url, ".lmd"),
   fFile(),fUrlOptions()
{
	 fUrlOptions = url.GetOptions();

   if (url.HasOption("rfio"))
      fFile.SetIO((dabc::FileInterface*) dabc::mgr.CreateAny("rfio::FileInterface"), true);
   else if (url.HasOption("ltsm"))
   	  fFile.SetIO((dabc::FileInterface*) dabc::mgr.CreateAny("ltsm::FileInterface"), true);
}

mbs::LmdOutput::~LmdOutput()
{
    DOUT0(" mbs::LmdOutput::DTOR");
   CloseFile();
}

bool mbs::LmdOutput::Write_Init()
{
   if (!dabc::FileOutput::Write_Init()) return false;

   return StartNewFile();
}

bool mbs::LmdOutput::StartNewFile()
{
   CloseFile();

   ProduceNewFileName();

   if (!fFile.OpenWriting(CurrentFileName().c_str(), fUrlOptions.c_str())) {
      ShowInfo(-1, dabc::format("%s cannot open file for writing", CurrentFileName().c_str()));
      return false;
   }

   ShowInfo(0, dabc::format("Open %s for writing", CurrentFileName().c_str()));

   return true;
}

bool mbs::LmdOutput::CloseFile()
{
   DOUT0(" mbs::LmdOutput::CloseFile()");
   if (fFile.isWriting()) {
      ShowInfo(0, dabc::format("Close file %s", CurrentFileName().c_str()));
      fFile.Close();
   }
   return true;
}

unsigned mbs::LmdOutput::Write_Buffer(dabc::Buffer& buf)
{
   if (!fFile.isWriting() || buf.null()) return dabc::do_Error;

   if (buf.GetTypeId() == dabc::mbt_EOF) {
      CloseFile();
      return dabc::do_Close;
   }

   if (buf.GetTypeId() != mbs::mbt_MbsEvents) {
      ShowInfo(-1, dabc::format("Buffer must contain mbs event(s) 10-1, but has type %u", buf.GetTypeId()));
      return dabc::do_Error;
   }

   if (buf.NumSegments()>1) {
      ShowInfo(-1, "Segmented buffer not (yet) supported");
      return dabc::do_Error;
   }

   if (CheckBufferForNextFile(buf.GetTotalSize()))
      if (!StartNewFile()) {
         EOUT("Cannot start new file for writing");
         return dabc::do_Error;
      }

   unsigned numevents = mbs::ReadIterator::NumEvents(buf);

   for (unsigned n=0;n<buf.NumSegments();n++)
      if (!fFile.WriteBuffer(buf.SegmentPtr(n), buf.SegmentSize(n))) {
         EOUT("lmd write error");
         return dabc::do_Error;
      }

   AccountBuffer(buf.GetTotalSize(), numevents);

   return dabc::do_Ok;
}
