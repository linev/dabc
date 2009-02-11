#include "dabc/Configuration.h"

#include <unistd.h>
#include <fnmatch.h>

#include "dabc/logging.h"
#include "dabc/Manager.h"
#include "dabc/Application.h"
#include "dabc/Parameter.h"
#include "dabc/Iterator.h"
#include "dabc/Factory.h"

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
                dabc::Factory::LoadLibrary(ResolveEnv(libname));
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
            std::string shortname(parname, separ-parname);
            const char* ownername = separ+1;

            if (app->IsName(ownername)) {
               Parameter* par = app->FindPar(shortname.c_str());
               if (par!=0) {
                  par->SetValue(parvalue);
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
   ConfigIO(),
   fSelected(0),
   fStoreStack(),
   fStoreLastPop(0),
   fCurrItem(0),
   fLastTop(0),
   fCurrStrict(true),
   fMgrHost(),
   fMgrName("Manager"),
   fMgrNodeId(0),
   fMgrNumNodes(0),
   fMgrDimNode(),
   fMgrDimPort(0)
{
}

dabc::Configuration::~Configuration()
{
}

bool dabc::Configuration::SelectContext(unsigned cfgid, unsigned nodeid, unsigned numnodes, const char* dimnode)
{
   fSelected = IsXDAQ() ? XDAQ_FindContext(cfgid) : FindContext(cfgid);

   if (fSelected==0) return false;

   envDABCNODEID = dabc::format("%u", nodeid);
   envDABCNUMNODES = dabc::format("%u", numnodes);

   envDABCSYS = GetEnv(xmlDABCSYS);

   if (IsNative()) envDABCUSERDIR = Find1(fSelected, "", xmlRunNode, xmlDABCUSERDIR);
   if (envDABCUSERDIR.empty()) envDABCUSERDIR = GetEnv(xmlDABCUSERDIR);

   char sbuf[1000];
   if (getcwd(sbuf, sizeof(sbuf)))
      envDABCWORKDIR = sbuf;
   else
      envDABCWORKDIR = ".";

   fMgrHost     = NodeName(cfgid);
   envHost      = fMgrHost;

   fMgrName     = ContextName(cfgid);
   envContext   = fMgrName;

   DOUT2(("Select context %u nodeid %u name %s", cfgid, nodeid, fMgrName.c_str()));

   dabc::SetDebugPrefix(fMgrName.c_str());

   fMgrNodeId   = nodeid;
   fMgrNumNodes = numnodes;

   if (numnodes>1) {
      dabc::SetDebugLevel(0);
      dabc::SetFileLevel(1);
   } else {
      dabc::SetDebugLevel(1);
      dabc::SetFileLevel(1);
   }

   if (IsNative()) {
      std::string val = Find1(fSelected, "", xmlRunNode, xmlDebuglevel);
      if (!val.empty()) dabc::SetDebugLevel(atoi(val.c_str()));
      val = Find1(fSelected, "", xmlRunNode, xmlLoglevel);
      if (!val.empty()) dabc::SetFileLevel(atoi(val.c_str()));
      val = Find1(fSelected, "", xmlRunNode, xmlParslevel);
      if (!val.empty()) dabc::WorkingProcessor::SetGlobalParsVisibility(atoi(val.c_str()));
   }

   std::string log;
   if (IsNative()) {
      log = Find1(fSelected, "", xmlRunNode, xmlLogfile);
   } else {
      log = fMgrName;
      log += ".log";
   }

   if (log.length()>0)
      dabc::Logger::Instance()->LogFile(log.c_str());

   if (dimnode==0) dimnode = getenv(xmlDIM_DNS_NODE);

   if (dimnode!=0) {
      const char* separ = strchr(dimnode, ':');
      if (separ==0)
         fMgrDimNode = dimnode;
      else {
         fMgrDimNode.assign(dimnode, separ - dimnode);
         fMgrDimPort = atoi(separ + 1);
      }
      DOUT4(("Mgr dim node %s port %d", fMgrDimNode.c_str(), fMgrDimPort));
   }

   return true;
}

std::string dabc::Configuration::InitFuncName()
{
   if (IsXDAQ() || (fSelected==0)) return std::string("");

   return Find1(fSelected, "", xmlRunNode, xmlInitFunc);
}

std::string dabc::Configuration::RunFuncName()
{
   if (IsXDAQ() || (fSelected==0)) return std::string("");

   return Find1(fSelected, "", xmlRunNode, xmlRunFunc);
}

const char* dabc::Configuration::ConetextAppClass()
{
   if (IsXDAQ() || (fSelected==0)) return 0;

   XMLNodePointer_t node = fXml.GetChild(fSelected);

   while (node!=0) {
      if (IsNodeName(node, xmlApplication)) break;
      node = fXml.GetNext(node);
   }

   if (node==0)
      node = FindMatch(0, fSelected, xmlApplication);

   return fXml.GetAttr(node, xmlClassAttr);
}


bool dabc::Configuration::LoadLibs()
{
    if (fSelected==0) return false;

    if (IsXDAQ()) return XDAQ_LoadLibs();

    std::string libname;
    XMLNodePointer_t last = 0;

    do {
       libname = FindN(fSelected, last, xmlRunNode, xmlUserLib);
       if (libname.empty()) break;
       DOUT2(("Find library %s in config", libname.c_str()));
       dabc::Factory::LoadLibrary(ResolveEnv(libname));
    } while (true);

    return true;
}

bool dabc::Configuration::ReadPars()
{
   if (fSelected==0) return false;

   if (IsXDAQ()) return XDAQ_ReadPars();

   return true;
}


bool dabc::Configuration::CreateItem(const char* name, const char* value)
{
   XMLNodePointer_t parent = 0;
   if (fStoreStack.size() > 0) parent = fStoreStack.back();
   XMLNodePointer_t item = fXml.NewChild(parent, 0, name, value);
   fStoreStack.push_back(item);
   return true;
}

bool dabc::Configuration::CreateAttr(const char* name, const char* value)
{
   if (fStoreStack.size() == 0) return false;
   XMLNodePointer_t parent = fStoreStack.back();
   fXml.NewAttr(parent, 0, name, value);
   return true;
}

bool dabc::Configuration::PopItem()
{
   if (fStoreStack.size() == 0) return false;
   fStoreLastPop = fStoreStack.back();
   fStoreStack.pop_back();
   return true;
}

bool dabc::Configuration::PushLastItem()
{
   if (fStoreLastPop==0) return false;
   fStoreStack.push_back(fStoreLastPop);
   fStoreLastPop = 0;
   return true;
}

bool dabc::Configuration::StoreObject(const char* fname, Basic* obj)
{
   if (obj == 0) return false;

   fStoreStack.clear();
   fStoreLastPop = 0;

   obj->Store(*this);

   if ((fStoreStack.size()!=0) || (fStoreLastPop==0)) {
      EOUT(("Error during store of object %s", obj->GetName()));
      if (fStoreStack.size()==0)
         fXml.FreeNode(fStoreLastPop);
      else
         fXml.FreeNode(fStoreStack.front());
      fStoreStack.clear();
      fStoreLastPop = 0;
      return false;
   }

   XMLDocPointer_t doc = fXml.NewDoc();
   fXml.DocSetRootElement(doc, fStoreLastPop);
   fXml.SaveDoc(doc, fname);
   fXml.FreeDoc(doc);

   fStoreLastPop = 0;

   return true;
}

bool dabc::Configuration::FindItem(const char* name)
{
   if (!IsNative() || (fCurrItem==0)) return false;

   if (fCurrChld==0)
      fCurrChld = fXml.GetChild(fCurrItem);
   else
      fCurrChld = fXml.GetNext(fCurrChld);

   while (fCurrChld!=0) {
      if (IsNodeName(fCurrChld, name)) {
         fCurrItem = fCurrChld;
         fCurrChld = 0;
         return true;
      }
      fCurrChld = fXml.GetNext(fCurrChld);
   }

   return false;
}

bool dabc::Configuration::CheckAttr(const char* name, const char* value)
{
   // make extra check - if fCurrChld!=0 something was wrong already
   if ((fCurrChld!=0) || (fCurrItem==0)) return false;

   bool res = true;

   if (value==0)
      res = fXml.HasAttr(fCurrItem, name);
   else {
      const char* attr = fXml.GetAttr(fCurrItem, name);

      std::string sattr = attr ? ResolveEnv(attr) : std::string("");

      if (fCurrStrict)
         res = sattr.empty() ? false : (sattr == value);
      else
      if (!sattr.empty()) {
         res = (sattr == value);
         if (!res) res = fnmatch(sattr.c_str(), value, FNM_NOESCAPE) == 0;
      }
   }

   if (!res) {
      fCurrChld = fCurrItem;
      fCurrItem = fXml.GetParent(fCurrItem);
   }

   return res;
}

std::string dabc::Configuration::GetAttrValue(const char* name)
{
   const char* res = fXml.GetAttr(fCurrItem, name);
   if (res==0) return std::string();
   return ResolveEnv(res);
}

dabc::Basic* dabc::Configuration::GetObjParent(Basic* obj, int lvl)
{
   while ((obj!=0) && (lvl-->0)) obj = obj->GetParent();
   return obj;
}


bool dabc::Configuration::Find(Basic* obj, std::string& value, const char* findattr)
{
   std::string item;

   if (findattr!=0) {
      const char* pos = strrchr(findattr, '/');

      if (pos!=0) {
         item.assign(findattr, pos - findattr);
         findattr = pos+1;
         if (*findattr==0) findattr = 0;
      }
   }

   return FindItem(obj, value, item.empty() ? 0 : item.c_str(), findattr);
}

dabc::XMLNodePointer_t dabc::Configuration::FindSubItem(XMLNodePointer_t node, const char* name)
{
   if ((node==0) || (name==0) || (strlen(name)==0)) return node;

   const char* pos = strchr(name, '/');
   if (pos==0) return FindChild(node, name);

   std::string subname(name, pos-name);
   return FindSubItem(FindChild(node, subname.c_str()), pos+1);
}

bool dabc::Configuration::FindItem(Basic* obj, std::string &res, const char* finditem, const char* findattr)
{
   res.clear();

   if (obj==0) return false;

   int maxlevel = 0;
   Basic* prnt = 0;
   while ((prnt = GetObjParent(obj, maxlevel)) != 0) {
      if (prnt==dabc::mgr()) break;
      maxlevel++;
   }

   DOUT3(("Configuration::Find object %s lvl = %d  attr = %s",
          obj->GetFullName().c_str(), maxlevel, (findattr ? findattr : "---")));

   if (prnt==0) return false;

   // first, try strict syntax
   fCurrStrict = true;
   fCurrItem = fSelected;
   fCurrChld = 0;
   int level = maxlevel - 1;

   while (level>=0) {
      prnt = GetObjParent(obj, level);
      if (prnt==0) return false;

      // search on same level several items - it may match attributes only with second or third
      do {
         if (prnt->Find(*this)) break;
         if (fCurrChld == 0) prnt = 0;
      } while (prnt != 0);
      if (prnt==0) break;

      if (level--==0) {
         if (finditem!=0) fCurrItem = FindSubItem(fCurrItem, finditem);

         if ((fCurrItem!=0) && ((findattr==0) || fXml.HasAttr(fCurrItem, findattr))) {
            res = findattr ? GetAttrValue(findattr) : GetNodeValue(fCurrItem);
            DOUT1(("Exact found %s res = %s", obj->GetFullName().c_str(), res.c_str()));
            return true;
         }
      }
   }

   fCurrStrict = false;

   do {

      DOUT3(("Start search with maxlevel = %d", maxlevel));

      fCurrItem = Dflts();
      fCurrChld = 0;

      level = maxlevel;
      while (level >= 0) {
         prnt = GetObjParent(obj, level);
         if (prnt == 0) return false;

         DOUT3(("Search parent %s", prnt->GetName()));

         if (prnt->Find(*this)) {
            if (level--==0) {
               if (finditem!=0) fCurrItem = FindSubItem(fCurrItem, finditem);
               if ((fCurrItem!=0) && ((findattr==0) || fXml.HasAttr(fCurrItem, findattr))) {
                  res = findattr ? GetAttrValue(findattr) : GetNodeValue(fCurrItem);
                  DOUT3(("Found object %s res = %s", obj->GetFullName().c_str(), res.c_str()));
                  return true;
               }
            }
         } else
         if (fCurrChld == 0) {
            level++;
            if (level > maxlevel) break;
            fCurrChld = fCurrItem;
            fCurrItem = fXml.GetParent(fCurrItem);
         }
      }

      maxlevel--;

      while (maxlevel > 0) {
         prnt = GetObjParent(obj, maxlevel);
         if (prnt->UseMasterClassName()) {
            DOUT3(("Try with master %s", prnt->GetName()));
            break;
         }
         maxlevel--;
      }

   } while (maxlevel>0);

   return false;
}
