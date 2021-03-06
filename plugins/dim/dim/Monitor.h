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

#ifndef DIM_Monitor
#define DIM_Monitor

#ifndef MBS_MonitorSlowControl
#include "mbs/MonitorSlowControl.h"
#endif

#include <map>

#include <dic.hxx>

class DimBrowser;
class DimInfo;

namespace dim {

   /** \brief Player of DIM data
    *
    * Module builds hierarchy for discovered DIM variables
    *
    **/

   class Monitor : public mbs::MonitorSlowControl,
                   public DimInfoHandler {

      protected:

         struct MyEntry {
            DimInfo* info;
            int      flag;
            long     fLong;      ///< buffer for long
            double   fDouble;    ///< buffer for double
            int      fKind;     ///< kind of new data (0 - none, 1 - long, 2 - double )
            MyEntry() : info(0), flag(0), fLong(0), fDouble(0.), fKind(0) {}
            MyEntry(const MyEntry& src) : info(src.info), flag(src.flag), fLong(src.fLong), fDouble(src.fDouble), fKind(src.fKind) {}
         };

         typedef std::map<std::string, MyEntry> DimServicesMap;

         std::string    fDimDns;      ///<  name of DNS server
         std::string    fDimMask;     ///<  mask of DIM services
         double         fDimPeriod;   ///<  how often DIM records will be checked, default 1 s
         ::DimBrowser*  fDimBr;       ///<  dim browser
         DimServicesMap fDimInfos;    ///< all subscribed DIM services
         dabc::TimeStamp fLastScan;   ///< last time when DIM services were scanned
         char           fNoLink[10];  ///< buffer used to detect nolink
         bool           fNeedDnsUpdate; ///< if true, should update DNS structures

         virtual void OnThreadAssigned();

         void ScanDimServices();

         virtual unsigned GetRecRawSize();


      public:
         Monitor(const std::string &name, dabc::Command cmd = nullptr);
         virtual ~Monitor();

         virtual void ProcessTimerEvent(unsigned timer);

         virtual int ExecuteCommand(dabc::Command cmd);

         virtual void infoHandler();

   };
}


#endif
