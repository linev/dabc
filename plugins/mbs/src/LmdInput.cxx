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

#include "mbs/LmdInput.h"

#include <string.h>
#include <stdlib.h>

#include "dabc/logging.h"
#include "dabc/Buffer.h"
#include "dabc/FileIO.h"
#include "dabc/Manager.h"
#include "dabc/Port.h"

#include "mbs/MbsTypeDefs.h"

mbs::LmdInput::LmdInput(const char* fname, uint32_t bufsize) :
   dabc::DataInput(),
   fFileName(fname ? fname : ""),
   fBufferSize(bufsize),
   fFilesList(0),
   fFile(),
   fCurrentFileName(),
   fCurrentRead(0)
{
}

mbs::LmdInput::~LmdInput()
{
   // FIXME: cleanup should be done much earlier
   CloseFile();
   if (fFilesList) {
      dabc::Object::Destroy(fFilesList);
      fFilesList = 0;
   }
}

bool mbs::LmdInput::Read_Init(const dabc::WorkerRef& wrk, const dabc::Command& cmd)
{
   fFileName = wrk.Cfg(mbs::xmlFileName, cmd).AsStdStr(fFileName);
   fBufferSize = wrk.Cfg(dabc::xmlBufferSize, cmd).AsInt(fBufferSize);

   // DOUT1(("BufferSize = %d", fBufferSize));

   return Init();
}

bool mbs::LmdInput::Init()
{
   if (fFileName.length()==0) return false;

   if (fFilesList!=0) {
      EOUT(("Files list already exists"));
      return false;
   }

   if (fBufferSize==0) {
      EOUT(("Buffer size not specified !!!!"));
      return false;
   }

   if (strpbrk(fFileName.c_str(),"*?")!=0)
      fFilesList = dabc::mgr()->ListMatchFiles("", fFileName.c_str());
   else {
      fFilesList = new dabc::Object(0, "FilesList", true);
      new dabc::Object(fFilesList, fFileName.c_str());
   }

   return OpenNextFile();
}

bool mbs::LmdInput::OpenNextFile()
{
   CloseFile();

   if ((fFilesList==0) || (fFilesList->NumChilds()==0)) return false;

   const char* nextfilename = fFilesList->GetChild(0)->GetName();

   bool res = fFile.OpenRead(nextfilename);

   if (!res)
      EOUT(("Cannot open file %s for reading, errcode:%u", nextfilename, fFile.LastError()));
   else {
      fCurrentFileName = nextfilename;
      DOUT1(("Open lmd file %s for reading", fCurrentFileName.c_str()));
   }

   fFilesList->DeleteChild(0);

   return res;
}


bool mbs::LmdInput::CloseFile()
{
   fFile.Close();
   fCurrentFileName = "";
   fCurrentRead = 0;
   return true;
}

unsigned mbs::LmdInput::Read_Size()
{
   // get size of the buffer which should be read from the file

   if (!fFile.IsReadMode())
      if (!OpenNextFile()) return dabc::di_EndOfStream;

   return fBufferSize;
}

unsigned mbs::LmdInput::Read_Complete(dabc::Buffer& buf)
{
   unsigned numev = 0;
   uint32_t bufsize = 0;

   do {

       if (!fFile.IsReadMode()) return dabc::di_Error;

       // TODO: read into segmented buffer
       bufsize = buf.SegmentSize(0);

       numev = fFile.ReadBuffer(buf.SegmentPtr(0), bufsize);

       if (numev==0) {
          DOUT3(("File %s return 0 numev for buffer %u - end of file", fCurrentFileName.c_str(), buf.GetTotalSize()));
          if (!OpenNextFile()) return dabc::di_EndOfStream;
       }

   } while (numev==0);

   fCurrentRead += bufsize;
   buf.SetTotalSize(bufsize);
   buf.SetTypeId(mbs::mbt_MbsEvents);

   return dabc::di_Ok;
}

mbs::EventHeader* mbs::LmdInput::ReadEvent()
{
   while (true) {
       if (!fFile.IsReadMode()) return 0;

       mbs::EventHeader* hdr = fFile.ReadEvent();
       if (hdr!=0) return hdr;

       DOUT1(("File %s return 0 - end of file", fCurrentFileName.c_str()));
       if (!OpenNextFile()) return 0;
   }

   return 0;
}
