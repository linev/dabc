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

#include "dimc/Factory.h"
#include "dimc/Observer.h"

#include "dabc/string.h"
#include "dabc/logging.h"
#include "dabc/Command.h"
#include "dabc/Manager.h"

dabc::FactoryPlugin dimcfactory(new dimc::Factory("dimc"));

void dimc::Factory::Initialize()
{

   dimc::Observer* observ = new dimc::Observer("/dim");
   if (!observ->IsEnabled()) {
      delete observ;
   } else {
      DOUT0("Initialize DIM control");
      dabc::WorkerRef(observ).MakeThreadForWorker("DimThread");
   }
}
