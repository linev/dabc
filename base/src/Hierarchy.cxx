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

#include "dabc/Hierarchy.h"

#include <string.h>
#include <stdlib.h>

#include "dabc/Iterator.h"
#include "dabc/timing.h"
#include "dabc/logging.h"
#include "dabc/Exception.h"
#include "dabc/Command.h"


const char* dabc::prop_kind = "kind";
const char* dabc::prop_realname = "realname";
const char* dabc::prop_masteritem = "master";
const char* dabc::prop_binary_producer = "binary_producer";
const char* dabc::prop_content_hash = "hash";
const char* dabc::prop_history = "history";
const char* dabc::prop_time = "time";


// ===============================================================================

dabc::HierarchyContainer::HierarchyContainer(const std::string& name) :
   dabc::RecordContainer(name, flNoMutex | flIsOwner),
   fNodeVersion(0),
   fHierarchyVersion(0),
   fPermanent(false),
   fNodeChanged(false),
   fHierarchyChanged(false),
   fBinData(),
   fHist()
{
   #ifdef DABC_EXTRA_CHECKS
   DebugObject("Hierarchy", this, 1);
   #endif

//   DOUT0("Constructor %s %p", GetName(), this);
}

dabc::HierarchyContainer::~HierarchyContainer()
{
   #ifdef DABC_EXTRA_CHECKS
   DebugObject("Hierarchy", this, -1);
   #endif

//   DOUT0("Destructor %s %p", GetName(), this);
}


dabc::HierarchyContainer* dabc::HierarchyContainer::TopParent()
{
   dabc::HierarchyContainer* parent = this;

   while (parent!=0) {
      dabc::HierarchyContainer* top = dynamic_cast<dabc::HierarchyContainer*> (parent->GetParent());
      if (top==0) return parent;
      parent = top;
   }

   EOUT("Should never happen");
   return this;
}

dabc::XMLNodePointer_t dabc::HierarchyContainer::SaveHierarchyInXmlNode(XMLNodePointer_t parentnode, uint64_t version, bool withversion)
{
   XMLNodePointer_t objnode = SaveInXmlNode(parentnode, WasNodeModifiedAfter(version));

   // provide information about node
   // sometime node just shows that it is there
   // provide mask attribute only when it is not 3

   unsigned mask = ModifiedMask(version);
   if (mask!=maskDefaultValue) Xml::NewAttr(objnode, 0, "dabc:mask", dabc::format("%u", mask).c_str());
   if (withversion) Xml::NewAttr(objnode, 0, "v", dabc::format("%lu", (long unsigned) GetVersion()).c_str());

   if (WasHierarchyModifiedAfter(version))
      for (unsigned n=0;n<NumChilds();n++) {
         dabc::HierarchyContainer* child = dynamic_cast<dabc::HierarchyContainer*> (GetChild(n));
         if (child) child->SaveHierarchyInXmlNode(objnode, version, withversion);
      }

   return objnode;
}

void dabc::HierarchyContainer::SetVersion(uint64_t version, bool recursive, bool force)
{
   if (force || fNodeChanged) fNodeVersion = version;
   if (force || fHierarchyChanged) fHierarchyVersion = version;
   if (recursive)
      for (unsigned n=0;n<NumChilds();n++) {
         dabc::HierarchyContainer* child = dynamic_cast<dabc::HierarchyContainer*> (GetChild(n));
         if (child) child->SetVersion(version, recursive, force);
      }
}

void dabc::HierarchyContainer::SetModified(bool node, bool hierarchy, bool recursive)
{
   if (node) fNodeChanged = true;
   if (hierarchy) fHierarchyChanged = true;
   if (recursive)
      for (unsigned n=0;n<NumChilds();n++) {
         dabc::HierarchyContainer* child = dynamic_cast<dabc::HierarchyContainer*> (GetChild(n));
         if (child) child->SetModified(node, hierarchy, recursive);
      }
}

std::string dabc::HierarchyContainer::RequestHistory(uint64_t version, int limit)
{
   if (fHist.Capacity()==0) return "";

   XMLNodePointer_t topnode = Xml::NewChild(0, 0, "gethistory", 0);

   Xml::NewAttr(topnode, 0, "version", dabc::format("%lu", (long unsigned) GetVersion()).c_str());

   SaveInXmlNode(topnode, true);

   bool cross_boundary = false;

   for (unsigned n=0; n<fHist()->fArr.Size();n++) {
      HistoryItem& item = fHist()->fArr.Item(n);

      // we have longer history as requested
      if ((version>0) && (item.version < version)) { cross_boundary = true; continue; }

      if ((limit>0) && ((fHist()->fArr.Size()-n) > (unsigned)limit)) continue;

      dabc::Xml::AddRawLine(topnode, item.content.c_str());
   }

   // if specific version was defined, and we do not have history backward to that version
   // we need to indicate this to the client
   // Client in this case should cleanup its buffers while with gap
   // one cannot correctly reconstruct history backwards
   if ((version>0) && !cross_boundary)
      Xml::NewAttr(topnode, 0, "gap", "true");

   std::string res;

   Xml::SaveSingleNode(topnode, &res, 1);
   Xml::FreeNode(topnode);

   return res;
}

bool dabc::HierarchyContainer::UpdateHierarchyFrom(HierarchyContainer* cont)
{
   fNodeChanged = false;
   fHierarchyChanged = false;

   if (cont==0) throw dabc::Exception(ex_Hierarchy, "empty container", ItemName());

   // we do not check names here - top object name can be different
   // if (!IsName(obj->GetName())) throw dabc::Exception(ex_Hierarchy, "mismatch between object and hierarchy itme", ItemName());

   bool dohistory = fHist.DoHistory();

   if (dohistory) cont->Field(prop_history).SetUInt(fHist.Capacity());

   // we need to recognize if any attribute disappear or changed
   if (!CompareFields(cont->GetFieldsMap(), (dohistory ? prop_time : 0))) {
      fNodeChanged = true;
      if (dohistory) {
         if (fHist()->fRecordTime) {
            dabc::DateTime tm;
            tm.GetNow();
            char sbuf[100];
            tm.AsJSString(sbuf, sizeof(sbuf));
            cont->Field(prop_time).SetStr(sbuf);
         }
         AddHistory(BuildDiff(cont->GetFieldsMap()));
      }
      SetFieldsMap(cont->TakeFieldsMap());
   }

   // now we should check if any childs were changed

   unsigned cnt1(0); // counter over source container
   unsigned cnt2(0); // counter over existing childs

   // skip first permanent childs from update procedure
   while (cnt2 < NumChilds() && ((HierarchyContainer*) GetChild(cnt2))->fPermanent) cnt2++;

   while ((cnt1 < cont->NumChilds()) || (cnt2 < NumChilds())) {
      if (cnt1 >= cont->NumChilds()) {
         RemoveChildAt(cnt2, true);
         fHierarchyChanged = true;
         continue;
      }

      dabc::Hierarchy cont_child(cont->GetChild(cnt1));
      if (cont_child.null()) {
         EOUT("FAILURE");
         return false;
      }

      bool findchild(false);
      unsigned findindx(0);

      for (unsigned n=cnt2;n<NumChilds();n++)
         if (GetChild(n)->IsName(cont_child.GetName())) {
            findchild = true;
            findindx = n;
            break;
         }

      if (!findchild) {
         // if child did not found, just take it out form source container and place at proper position

         // remove object and do not cleanup (destroy) it
         cont->RemoveChild(cont_child(), false);

         cont_child()->SetModified(true, true, true);

         AddChildAt(cont_child(), cnt2);

         fHierarchyChanged = true;
         cnt2++;

         continue;
      }

      while (findindx > cnt2) { RemoveChildAt(cnt2, true); findindx--; fHierarchyChanged = true; }

      dabc::HierarchyContainer* child = (dabc::HierarchyContainer*) (GetChild(cnt2));

      if ((child==0) || !child->IsName(cont_child.GetName())) {
         throw dabc::Exception(ex_Hierarchy, "mismatch between object and hierarchy item", ItemName());
         return false;
      }

      if (child->UpdateHierarchyFrom(cont_child())) fHierarchyChanged = true;

      cnt1++;
      cnt2++;
   }

   return fHierarchyChanged || fNodeChanged;
}

dabc::HierarchyContainer* dabc::HierarchyContainer::CreateChildAt(const std::string& name, int indx)
{
   while ((indx>=0) && (indx<(int) NumChilds())) {
      dabc::HierarchyContainer* child = (dabc::HierarchyContainer*) GetChild(indx);
      if (child->IsName(name.c_str())) return child;
      RemoveChildAt(indx, true);
   }

   dabc::HierarchyContainer* res = new dabc::HierarchyContainer(name);
   AddChild(res);

   return res;
}



std::string dabc::HierarchyContainer::ItemName()
{
   std::string res;
   FillFullName(res, 0, true);
   return res;
}

bool dabc::HierarchyContainer::UpdateHierarchyFromXmlNode(XMLNodePointer_t objnode)
{
   // we do not check node name - it is done when childs are selected
   // for top-level node name can differ

   // if (!IsName(Xml::GetNodeName(objnode))) return false;

   unsigned mask = maskDefaultValue;
   if (Xml::HasAttr(objnode,"dabc:mask")) {
      mask = (unsigned) Xml::GetIntAttr(objnode, "dabc:mask");
      Xml::FreeAttr(objnode, "dabc:mask");
   }

   fNodeChanged = (mask & maskNodeChanged) != 0;
   fHierarchyChanged = (mask & maskChildsChanged) != 0;

   if (fNodeChanged) {
      ClearFields();
      if (!ReadFieldsFromNode(objnode, true, false)) return false;
   }

   if (!fHierarchyChanged) return true;

   XMLNodePointer_t childnode = Xml::GetChild(objnode);
   unsigned cnt = 0;

   while ((childnode!=0) || (cnt<NumChilds())) {

      if (childnode==0) {
         // special case at the end - one should delete childs at the end
         RemoveChildAt(cnt, true);
         continue;
      }

      const char* childnodename = Xml::GetNodeName(childnode);
      if (strcmp(childnodename, "dabc:field") == 0) {
         Xml::ShiftToNext(childnode);
         continue;
      }

      bool findchild(false);
      unsigned findindx(0);

      for (unsigned n=cnt;n<NumChilds();n++)
         if (GetChild(n)->IsName(childnodename)) {
            findchild = true;
            findindx = n;
            break;
         }

      dabc::HierarchyContainer* child = 0;

      if (findchild) {
         // delete all child with non-matching names
         while (findindx > cnt) { RemoveChildAt(cnt, true); findindx--; }
         child = dynamic_cast<dabc::HierarchyContainer*> (GetChild(cnt));
      } else {
         child = new dabc::HierarchyContainer(childnodename);
         AddChildAt(child, cnt);
      }

      if ((child==0) || !child->IsName(childnodename)) {
         EOUT("Internal error - did not create or found child for node %s", childnodename);
         return false;
      }

      if (!child->UpdateHierarchyFromXmlNode(childnode)) return false;

      Xml::ShiftToNext(childnode);
      cnt++;
   }

   return true;
}

std::string dabc::HierarchyContainer::HtmlBrowserText()
{
   if (NumChilds()>0) return GetName();

   const char* kind = GetField(dabc::prop_kind);

   if (kind!=0) {
      std::string res = dabc::format("<a href='#' onClick='DABC.mgr.click(this);' kind='%s' fullname='%s'", kind, ItemName().c_str());

      const char* val = GetField("value");
      if (val!=0) res += dabc::format(" value='%s'", val);

      return res + dabc::format(">%s</a>", GetName());
   }

   return GetName();
}

bool dabc::HierarchyContainer::SaveToJSON(std::string& sbuf, bool isfirstchild, int level)
{
   if (GetField("hidden")!=0) return false;

   bool compact = level==0;
   const char* nl = compact ? "" : "\n";

   if (!isfirstchild) sbuf += dabc::format(",%s", nl);

   if (NumChilds()==0) {
      sbuf += dabc::format("%*s{\"text\":\"%s\"}", level*3, "", HtmlBrowserText().c_str());
      return true;
   }

   sbuf += dabc::format("%*s{\"text\":\"%s\",%s", level*3, "",  HtmlBrowserText().c_str(), nl);
   sbuf += dabc::format("%*s\"children\":[%s", level*3, "", nl);
   if (!compact) level++;

   bool isfirst = true;
   for (unsigned n=0;n<NumChilds();n++) {
      dabc::HierarchyContainer* child = dynamic_cast<dabc::HierarchyContainer*> (GetChild(n));

      if (child==0) continue;

      if (child->SaveToJSON(sbuf, isfirst, compact ? 0 : level+1)) isfirst = false;
   }

   sbuf+= dabc::format("%s%*s]%s", nl, level*3, "", nl);
   if (!compact) level--;
   sbuf+= dabc::format("%*s}", level*3, "");

   return true;
}

void dabc::HierarchyContainer::BuildHierarchy(HierarchyContainer* cont)
{
   cont->CopyFieldsMap(GetFieldsMap());

   dabc::RecordContainer::BuildHierarchy(cont);
}

std::string dabc::HierarchyContainer::MakeSimpleDiff(const char* oldvalue)
{
   XMLNodePointer_t node = Xml::NewChild(0, 0, "h", 0);

   if (fHist() && fHist()->fRecordTime) {
      const char* tvalue = GetField(prop_time);
      if (tvalue) Xml::NewAttr(node, 0, prop_time, tvalue);
   }

   Xml::NewAttr(node, 0, fHist()->fAutoRecord.c_str(), oldvalue);

   std::string res;

   Xml::SaveSingleNode(node, &res, 0);
   Xml::FreeNode(node);

   return res;
}

void dabc::HierarchyContainer::AddHistory(const std::string& diff)
{
   if (fHist.Capacity()==0) return;
   if (fHist()->fArr.Full()) fHist()->fArr.PopOnly();
   HistoryItem* h = fHist()->fArr.PushEmpty();
   h->version = fNodeVersion;
   h->content = diff;
}

bool dabc::HierarchyContainer::SetField(const std::string& name, const char* value, const char* kind)
{
   if (fHist() && !fHist()->fAutoRecord.empty()) {
      const std::string usename = name.empty() ? DefaultFiledName() : name;

      if (usename == fHist()->fAutoRecord) {
         const char* oldvalue = GetField(name);
         if ((oldvalue!=0) && (value!=0) && (fHist()->fForceAutoRecord || (strcmp(oldvalue, value)!=0))) {
            // record history
            if (fHist.DoHistory())
               AddHistory(MakeSimpleDiff(oldvalue));

            // record time
            if (fHist()->fRecordTime) {
               dabc::DateTime tm;
               tm.GetNow();
               char sbuf[100];
               tm.AsJSString(sbuf, sizeof(sbuf));

               dabc::RecordContainer::SetField(prop_time, sbuf, 0);
            }

            fNodeChanged = true;

            // we detect change of important field and mark it with new version
            MarkWithChangedVersion();
         }
      }
   }


   return dabc::RecordContainer::SetField(name, value, kind);
}


// __________________________________________________-


void dabc::Hierarchy::Build(const std::string& topname, Reference top)
{
   if (null() && !topname.empty()) Create(topname);

   if (!top.null() && !null())
      top()->BuildHierarchy(GetObject());
}

void dabc::HierarchyContainer::MarkWithChangedVersion()
{
   HierarchyContainer* top = this, *prnt = this;

   while (prnt!=0) {
      top = prnt;
      prnt = dynamic_cast<HierarchyContainer*> (prnt->GetParent());
   }

   uint64_t next_ver = top->GetVersion() + 1;

   SetVersion(next_ver, true, false);

   prnt = this;
   while (prnt != 0) {
      prnt = dynamic_cast<HierarchyContainer*> (prnt->GetParent());
      if (prnt) prnt->SetVersion(next_ver, false, true);
   }
}


bool dabc::Hierarchy::Update(dabc::Hierarchy& src)
{
   if (src.null()) {
      // now release will work - any hierarchy will be deleted once no any other refs exists
      Release();
      return true;
   }

   if (null()) {
      EOUT("Hierarchy structure MUST exist at this moment");
      Create("DABC");
   }

   if (GetObject()->UpdateHierarchyFrom(src()))
      GetObject()->MarkWithChangedVersion();

   return true;
}


bool dabc::Hierarchy::UpdateHierarchy(Reference top)
{
   if (null()) {
      Build(top.GetName(), top);
      return true;
   }

   if (top.null()) {
      EOUT("Objects structure must be provided");
      return false;
   }

   dabc::Hierarchy src;

   src.Build(top.GetName(), top);

   return Update(src);
}

void dabc::Hierarchy::EnableHistory(int length, const std::string& autorec, bool force)
{
   if (null()) return;

   if (GetObject()->fHist.Capacity() == (unsigned) length) return;

   if (length>0) {
      GetObject()->fHist.Allocate();
      GetObject()->fHist()->fEnabled = true;
      GetObject()->fHist()->fArr.Allocate(length);
      GetObject()->fHist()->fRecordTime = true;
      GetObject()->fHist()->fAutoRecord = autorec;
      GetObject()->fHist()->fForceAutoRecord = force;
      Field(prop_history).SetInt(length);
   } else {
      GetObject()->fHist.Release();
      RemoveField(prop_history);
   }
}


bool dabc::Hierarchy::HasLocalHistory() const
{
   return null() ? false : GetObject()->fHist.DoHistory();
}


bool dabc::Hierarchy::HasActualRemoteHistory() const
{
   if (null() || (GetObject()->fHist.Capacity() == 0)) return false;

   // we have actual copy of remote history when request was done after last hierarchy update
   return ((GetObject()->fHist()->fRemoteReqVersion > 0) &&
           (GetObject()->fHist()->fLocalReqVersion == GetObject()->GetVersion()));
}


std::string dabc::Hierarchy::SaveToXml(bool compact, uint64_t version)
{
   Iterator iter2(*this);

   bool withversion = false;

   if (version == (uint64_t)-1) {
      withversion = true;
      version = 0;
   }

   XMLNodePointer_t topnode = GetObject()->SaveHierarchyInXmlNode(0, version, withversion);

   Xml::NewAttr(topnode, 0, "dabc:version", dabc::format("%lu", (long unsigned) GetVersion()).c_str());

   std::string res;

   if (topnode) {
      Xml::SaveSingleNode(topnode, &res, compact ? 0 : 1);
      Xml::FreeNode(topnode);
   }

   return res;
}


void dabc::Hierarchy::Create(const std::string& name)
{
   Release();
   SetObject(new HierarchyContainer(name));
}

dabc::Hierarchy dabc::Hierarchy::CreateChild(const std::string& name, int indx)
{
   if (null() || name.empty()) return dabc::Hierarchy();

   return GetObject()->CreateChildAt(name, indx);

   return FindChild(name.c_str());
}

dabc::Hierarchy dabc::Hierarchy::FindMaster()
{
   if (null() || (GetParent()==0) || !HasField(dabc::prop_masteritem)) return dabc::Hierarchy();

   std::string masteritem = Field(dabc::prop_masteritem).AsStdStr();

   if (masteritem.empty()) return dabc::Hierarchy();

   return GetParent()->FindChildRef(masteritem.c_str());
}

bool dabc::Hierarchy::UpdateFromXml(const std::string& src)
{
   if (src.empty()) return false;

   XMLNodePointer_t topnode = Xml::ReadSingleNode(src.c_str());

   if (topnode==0) return false;

   bool res = true;
   long unsigned version(0);

   if (!Xml::HasAttr(topnode,"dabc:version") || !dabc::str_to_luint(Xml::GetAttr(topnode,"dabc:version"), &version)) {
      res = false;
      EOUT("Not found topnode version");
   } else {
      DOUT3("Hierarchy version is %lu", version);
      Xml::FreeAttr(topnode, "dabc:version");
   }

   if (res) {

      if (null()) Create(Xml::GetNodeName(topnode));

      if (!GetObject()->UpdateHierarchyFromXmlNode(topnode)) {
         EOUT("Fail to update hierarchy from xml");
         res = false;
      } else {
         GetObject()->SetVersion(version, true, false);
      }
   }

   Xml::FreeNode(topnode);

   return res;
}

std::string dabc::Hierarchy::SaveToJSON(bool compact, bool excludetop)
{
   if (null()) return "";

   std::string res;

   res.append(compact ? "[" : "[\n");

   if (excludetop) {
      bool isfirst = true;
      for (unsigned n=0;n<NumChilds();n++) {
         dabc::Hierarchy child = GetChild(n);

         if (child.null()) continue;

         if (child()->SaveToJSON(res, isfirst, compact ? 0 : 1)) isfirst = false;
      }
   } else {
      GetObject()->SaveToJSON(res, true, compact ? 0 : 1);
   }

   res.append(compact ? "]" : "\n]\n");

   return res;
}



dabc::Buffer dabc::Hierarchy::GetBinaryData(uint64_t query_version)
{
   if (null()) return 0;

   dabc::Hierarchy master = FindMaster();

   // take data under lock that we are sure - nothing change for me
   uint64_t item_version = GetVersion();
   uint64_t master_version(0);

   if (!master.null()) {
      master_version = master.GetVersion();
   }

   Buffer& bindata = GetObject()->bindata();

   dabc::BinDataHeader* bindatahdr = 0;

   if (!bindata.null())
      bindatahdr = (dabc::BinDataHeader*) bindata.SegmentPtr();

   bool can_reply = (bindatahdr!=0) && (item_version==bindatahdr->version) && (query_version<=item_version);

   if (!can_reply) return 0;

   if ((query_version>0) && (query_version==bindatahdr->version)) {
      dabc::BinDataHeader dummyhdr;
      dummyhdr.reset();
      dummyhdr.version = bindatahdr->version;
      dummyhdr.master_version = master_version;

      // create temporary buffer with header only indicating that version is not changed
      return dabc::Buffer::CreateBuffer(&dummyhdr, sizeof(dummyhdr), false, true);
   }

   bindatahdr->master_version = master_version;
   return bindata;
}


dabc::Command dabc::Hierarchy::ProduceBinaryRequest()
{
   if (null()) return 0;

   std::string request_name;
   std::string producer_name = FindBinaryProducer(request_name);

   if (request_name.empty()) return 0;

   dabc::Command cmd("GetBinary");
   cmd.SetReceiver(producer_name);
   cmd.SetStr("Item", request_name);

   return cmd;
}

dabc::Buffer dabc::Hierarchy::ApplyBinaryRequest(Command cmd)
{
   if (null() || (cmd.GetResult() != cmd_true)) return 0;

   Buffer bindata = cmd.GetRawData();
   if (bindata.null()) return 0;

   uint64_t item_version = GetVersion();

   dabc::BinDataHeader* bindatahdr = (dabc::BinDataHeader*) bindata.SegmentPtr();
   bindatahdr->version = item_version;

   GetObject()->bindata() = bindata;

   dabc::Hierarchy master = FindMaster();

   uint64_t master_version = master.GetVersion();

   //DOUT0("BINARY REQUEST AFTER MASTER VERSION %u", (unsigned) master_version);

   // master hash can let us know if something changed in the master
   std::string masterhash = cmd.GetStdStr("MasterHash");

   if (!master.null() && !masterhash.empty() && (master.Field(dabc::prop_content_hash).AsStdStr()!=masterhash)) {

      master()->fNodeChanged = true;

      master()->MarkWithChangedVersion();

      // DOUT0("MASTER VERSION WAS %u NOW %u", (unsigned) master_version, (unsigned) master.GetVersion());

      master_version = master.GetVersion();
      master.Field(dabc::prop_content_hash).SetStr(masterhash);
   }

   bindatahdr->master_version = master_version;

   return bindata;
}


std::string dabc::Hierarchy::FindBinaryProducer(std::string& request_name)
{
   dabc::Hierarchy parent = *this;
   std::string producer_name;

   // we need to find parent on the most-top position with binary_producer property
   while (!parent.null()) {
      if (parent.HasField(dabc::prop_binary_producer)) {
         producer_name = parent.Field(dabc::prop_binary_producer).AsStdStr();
         request_name = RelativeName(parent);
      }
      parent = parent.GetParentRef();
   }

   return producer_name;
}


dabc::Command dabc::Hierarchy::ProduceHistoryRequest()
{
   // STEP1. Create request which can be send to remote

   if (null()) return 0;

   int history_size = Field(dabc::prop_history).AsInt(0);

   if (history_size<=0) return 0;

   std::string producer_name, request_name;

   producer_name = FindBinaryProducer(request_name);

   if (producer_name.empty() && request_name.empty()) return 0;

   if (GetObject()->fHist.null())
      GetObject()->fHist.Allocate(history_size);

   dabc::Command cmd("GetBinary");
   cmd.SetStr("Item", request_name);
   cmd.SetBool("history", true); // indicate that we are requesting history
   cmd.SetInt("limit", history_size);
   cmd.SetUInt("version", GetObject()->fHist()->fRemoteReqVersion);
   cmd.SetReceiver(producer_name);

   return cmd;
}

dabc::Buffer dabc::Hierarchy::ExecuteHistoryRequest(Command cmd)
{
   // STEP2. Process request in the structure, which really records history

   if (null()) return 0;

   unsigned ver = cmd.GetUInt("version");
   int limit = cmd.GetInt("limit");

   std::string res = GetObject()->RequestHistory(ver, limit);

   if (res.empty()) return 0;

   cmd.SetUInt("version", GetVersion());

   // we transport trailing 0 to avoid problem on receiver side
   return dabc::Buffer::CreateBuffer(res.c_str(), res.length()+1, false, true);
}


bool dabc::Hierarchy::ApplyHierarchyRequest(Command cmd)
{
   // STEP3. Unfold reply from remote and reproduce history recording here

   if (cmd.GetResult() != cmd_true) return false;

   if (null() || GetObject()->fHist.null()) return false;


//   DOUT0("STEP3 - analyze reply");

   dabc::Buffer buf = cmd.GetRawData();

   if (buf.null()) { EOUT("No raw data in history reply"); return false; }

   // decode reply
   XMLNodePointer_t node = Xml::ReadSingleNode((const char*) buf.SegmentPtr());

   // can not decode xml code
   if (node==0) { EOUT("Bad XML syntax in history reply"); return false; }

   bool res = true;

   XMLNodePointer_t child = Xml::GetChild(node);

   // TODO: force all fields from xml
   if (!IsName(Xml::GetNodeName(child)) || !GetObject()->ReadFieldsFromNode(child, true, false)) {
      EOUT("First item in reply should be node itself");
      child = 0; res = false;
   }

   // this is all about history
   // we are adding history with previous number while
   while ((child = Xml::GetNext(child)) != 0) {
      std::string sbuf;
      Xml::SaveSingleNode(child, &sbuf, 0);
      GetObject()->AddHistory(sbuf);
   }

   GetObject()->fNodeChanged = true;
   GetObject()->MarkWithChangedVersion();

   Xml::FreeNode(node);

   // remember when reply comes back
   if (res) {
      GetObject()->fHist()->fRemoteReqVersion = cmd.GetUInt("version");
      GetObject()->fHist()->fLocalReqVersion = GetVersion();
   }

   return res;
}


dabc::Buffer dabc::Hierarchy::GetLocalImage(uint64_t version)
{
   if (null()) return 0;
   if (Field(prop_kind).AsStdStr()!="image.png") return 0;

   return GetObject()->bindata();
}

dabc::Command dabc::Hierarchy::ProduceImageRequest()
{
   std::string producer_name, request_name;

   producer_name = FindBinaryProducer(request_name);

   if (producer_name.empty() && request_name.empty()) return 0;

   dabc::Command cmd("GetBinary");
   cmd.SetStr("Item", request_name);
   cmd.SetBool("image", true); // indicate that we are requesting image
   cmd.SetReceiver(producer_name);

   return cmd;
}

bool dabc::Hierarchy::ApplyImageRequest(Command cmd)
{
   if (null() || (cmd.GetResult() != cmd_true)) return false;

   Buffer bindata = cmd.GetRawData();
   if (bindata.null()) return false;

   GetObject()->fNodeChanged = true;
   GetObject()->MarkWithChangedVersion();
   GetObject()->bindata() = bindata;

   return true;
}

