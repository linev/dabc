#include "dabc/Configuration.h"

#include <unistd.h>

#include "dabc/logging.h"
#include "dabc/Manager.h"
#include "dabc/Application.h"
#include "dabc/Parameter.h"

bool dabc::Configuration::XDAQ_LoadLibs()
{
   XMLNodePointer_t modnode = fXml.GetChild(fSelected);

   while (modnode!=0) {
      if (strcmp(fXml.GetNodeName(modnode), xmlXDAQModule)==0) {

         const char* libname = fXml.GetNodeContent(modnode);

         DOUT1(("Find lib %s", libname));

         if ((strstr(libname,"libdim")==0) &&
             (strstr(libname,"libDabcBase")==0) &&
             (strstr(libname,"libDabcXDAQControl")==0))
                dabc::Manager::LoadLibrary(ResolveEnv(libname).c_str());
      }

      modnode = fXml.GetNext(modnode);
   }

   return true;
}

bool dabc::Configuration::XDAQ_ReadPars()
{

   XMLNodePointer_t appnode = fXml.GetChild(fSelected);

   if (!IsNodeName(appnode, xmlXDAQApplication)) {
      EOUT(("%s node in context not found", xmlXDAQApplication));
      return false;
   }

   XMLNodePointer_t propnode = fXml.GetChild(appnode);
   if (!IsNodeName(propnode, xmlXDAQproperties)!=0) {
      EOUT(("%s node not found", xmlXDAQproperties));
      return false;
   }

   XMLNodePointer_t parnode = fXml.GetChild(propnode);

   Application* app = mgr()->GetApp();

   while (parnode != 0) {
      const char* parname = fXml.GetNodeName(parnode);
      const char* parvalue = fXml.GetNodeContent(parnode);
//      const char* partyp = xml.GetAttr(parnode, "xsi:type");

      if (strcmp(parname,xmlXDAQdebuglevel)==0) {
         dabc::SetDebugLevel(atoi(parvalue));
         DOUT1(("Set debug level = %s", parvalue));
      } else {
         const char* separ = strchr(parname, '.');
         if ((separ!=0) && (app!=0)) {
            dabc::String shortname(parname, separ-parname);
            const char* ownername = separ+1;

            if (app->IsName(ownername)) {
               Parameter* par = app->FindPar(shortname.c_str());
               if (par!=0) {
                  par->SetStr(parvalue);
                  DOUT1(("Set parameter %s = %s", parname, parvalue));
               }
               else
                  EOUT(("Not found parameter %s in plugin", shortname.c_str()));
            } else {
               EOUT(("Not find owner %s for parameter %s", ownername, parname));
            }
         }
      }

      parnode = fXml.GetNext(parnode);
   }

   return true;
}

// ___________________________________________________________________________________


dabc::Configuration::Configuration(const char* fname) :
   ConfigBase(fname),
   fSelected(0)
{
}

dabc::Configuration::~Configuration()
{
}

bool dabc::Configuration::SelectContext(unsigned cfgid, unsigned nodeid, unsigned numnodes)
{
   fSelected = IsXDAQ() ? XDAQ_FindContext(cfgid) : FindContext(cfgid);

   if (fSelected==0) return false;

   const char* val = getenv(xmlDABCSYS);
   if (val!=0) envDABCSYS = val;

   val = 0;
   if (IsNative()) val = Find1(fSelected, 0, xmlRunNode, xmlDABCUSERDIR);
   if (val==0) val = getenv(xmlDABCUSERDIR);
   if (val!=0) envDABCUSERDIR = val;

   char sbuf[1000];
   if (getcwd(sbuf, sizeof(sbuf)))
      envDABCWORKDIR = sbuf;
   else
      envDABCWORKDIR = ".";

   envDABCNODEID = FORMAT(("%u", nodeid));
   envDABCNUMNODES = FORMAT(("%u", numnodes));

   return true;
}

bool dabc::Configuration::LoadLibs()
{
    if (fSelected==0) return false;

    if (IsXDAQ()) return XDAQ_LoadLibs();

    const char* libname = 0;
    XMLNodePointer_t last = 0;

    while ((libname = FindN(fSelected, last, xmlRunNode, xmlUserLib))!=0) {
       DOUT0(("Find library %s", libname));
       dabc::Manager::LoadLibrary(ResolveEnv(libname).c_str());
    }

    return true;
}

bool dabc::Configuration::ReadPars()
{
   if (fSelected==0) return false;

   if (IsXDAQ()) return XDAQ_ReadPars();

   return true;
}
