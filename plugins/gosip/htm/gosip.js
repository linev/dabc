var POLAND_TS_NUM = 3,
    POLAND_DAC_NUM = 32,
    POLAND_ERRCOUNT_NUM = 8,
    POLAND_TIME_UNIT = 0.02,

    POLAND_REG_TRIGCOUNT        = 0x000000,
    POLAND_REG_RESET            = 0x200000,
    POLAND_REG_STEPS_BASE       = 0x200014,
    POLAND_REG_STEPS_TS1        = 0x200014,
    POLAND_REG_STEPS_TS2        = 0x200018,
    POLAND_REG_STEPS_TS3        = 0x20001C,

    POLAND_REG_TIME_BASE        = 0x200020,
    POLAND_REG_TIME_TS1         = 0x200020,
    POLAND_REG_TIME_TS2         = 0x200024,
    POLAND_REG_TIME_TS3         = 0x200028,

    POLAND_REG_QFW_MODE         = 0x200004,

    POLAND_REG_DAC_MODE         = 0x20002C,
    POLAND_REG_DAC_PROGRAM      = 0x200030,
    POLAND_REG_DAC_BASE_WRITE   = 0x200050,
    POLAND_REG_DAC_BASE_READ    = 0x200180,

    POLAND_REG_DAC_ALLVAL       = 0x2000d4,
    POLAND_REG_DAC_CAL_STARTVAL = 0x2000d0,
    POLAND_REG_DAC_CAL_OFFSET   = 0x200034,
    POLAND_REG_DAC_CAL_DELTA    = 0x20000c,
    POLAND_REG_DAC_CAL_TIME     = 0x200038,

    POLAND_REG_INTERNAL_TRIGGER = 0x200040,
    POLAND_REG_DO_OFFSET        = 0x200044,
    POLAND_REG_OFFSET_BASE      = 0x200100,
    POLAND_REG_MASTERMODE       = 0x200048,
    POLAND_REG_ERRCOUNT_BASE    = 0x200,
    POLAND_REG_TRIG_ON          = 0x20004C;

function PolandSetup(cmdurl) {

   // address used to access gosip command
   this.CmdUrl = cmdurl;

   this.fSFP = 0;
   this.fDEV = 0;
   this.fLogging = false;
   this.fLogData = null;

   this.fSteps = [];
   this.fTimes = [];
   for ( var i = 0; i < POLAND_TS_NUM; i++) {
      this.fSteps.push(0);
      this.fTimes.push(0);
   }

   this.fInternalTrigger = 0;
   this.fTriggerMode = 0;
   this.fQFWMode = 0;

   this.fEventCounter = 0;
   this.fErrorCounter = [];
   for ( var i = 0; i < POLAND_ERRCOUNT_NUM; i++)
      this.fErrorCounter.push(0);

   /* DAC values and settings:*/
   this.fDACMode = 0;
   this.fDACValue = [];
   for ( var i = 0; i < POLAND_DAC_NUM; i++)
      this.fDACValue.push(i);

   this.fDACAllValue = 0;
   this.fDACStartValue = 0;
   this.fDACOffset = 0;
   this.fDACDelta = 0;
   this.fDACCalibTime = 0;
   this.fTriggerAcceptance = true;
}

PolandSetup.prototype.SetTriggerAcceptance = function(on) {
   this.fTriggerAcceptance = on;
}

PolandSetup.prototype.IsTriggerAcceptance = function() {
   return (this.fTriggerAcceptance);
}

PolandSetup.prototype.SetTriggerMaster = function(on) {
   this.fTriggerMode = on ? (this.fTriggerMode | 2) : (this.fTriggerMode & ~2);
}

PolandSetup.prototype.IsTriggerMaster = function() {
   return ((this.fTriggerMode & 2) === 2);
}

PolandSetup.prototype.SetFesaMode = function(on) {
   this.fTriggerMode = on ? (this.fTriggerMode | 1) : (this.fTriggerMode & ~1);
}

PolandSetup.prototype.IsFesaMode = function() {
   return (this.fTriggerMode & 1) === 1;
}

PolandSetup.prototype.SetInternalTrigger = function(on) {
   this.fInternalTrigger = on ? (this.fInternalTrigger | 1)
         : (this.fInternalTrigger & ~1);
}

PolandSetup.prototype.IsInternalTrigger = function() {
   return (this.fInternalTrigger & 1) === 1;
}

PolandSetup.prototype.GetCalibrationTime = function() {
   return (this.fDACCalibTime * POLAND_TIME_UNIT) / 1000.;
}

PolandSetup.prototype.SetCalibrationTime = function(ms) {
   this.fDACCalibTime = 1000 * ms / POLAND_TIME_UNIT;
}

PolandSetup.prototype.GetStepTime = function(loop) {
   return this.fTimes[loop] * POLAND_TIME_UNIT;
}

PolandSetup.prototype.SetStepTime = function(loop, us) {
   this.fTimes[loop] = us / POLAND_TIME_UNIT;
}

PolandSetup.prototype.RefreshQFW = function(base) {
   var options = document.getElementById("QFWModeCombo").options;
   for ( var i = 0; i < options.length; i++)
      options[i].selected = (options[i].value == this.fQFWMode);
   $("#QFWModeCombo").selectmenu('refresh', true);

   if (!base)
      base = 16;
   var pre = base == 16 ? "0x" : "";

   document.getElementById("TS1Loop").value = pre
         + this.fSteps[0].toString(base);
   document.getElementById("TS2Loop").value = pre
         + this.fSteps[1].toString(base);
   document.getElementById("TS3Loop").value = pre
         + this.fSteps[2].toString(base);
   document.getElementById("TS1Time").value = this.GetStepTime(0).toString();
   document.getElementById("TS2Time").value = this.GetStepTime(1).toString();
   document.getElementById("TS3Time").value = this.GetStepTime(2).toString();

   document.getElementById("MasterTrigger").checked = this.IsTriggerMaster();
   document.getElementById("FESAMode").checked = this.IsFesaMode();
   document.getElementById("InternalTrigger").checked = this
         .IsInternalTrigger();
}

PolandSetup.prototype.EvaluateQFW = function() {
   var options = document.getElementById("QFWModeCombo").options;
   for ( var i = 0; i < options.length; i++)
      if (options[i].selected)
         this.fQFWMode = Number(options[i].value);

   this.fSteps[0] = parseInt(document.getElementById("TS1Loop").value);
   this.fSteps[1] = parseInt(document.getElementById("TS2Loop").value);
   this.fSteps[2] = parseInt(document.getElementById("TS3Loop").value);
   this.SetStepTime(0, parseFloat(document.getElementById("TS1Time").value));
   this.SetStepTime(1, parseFloat(document.getElementById("TS2Time").value));
   this.SetStepTime(2, parseFloat(document.getElementById("TS3Time").value));

   this.SetTriggerMaster(document.getElementById("MasterTrigger").checked);
   this.SetFesaMode(document.getElementById("FESAMode").checked);
   this.SetInternalTrigger(document.getElementById("InternalTrigger").checked);
}

PolandSetup.prototype.RefreshDAC = function(base) {

   var options = document.getElementById("DacModeCombo").options;
   for ( var i = 0; i < options.length; i++)
      options[i].selected = (options[i].value == this.fDACMode);
   $("#DacModeCombo").selectmenu('refresh', true);

   switch (this.fDACMode) {
   // enable/disable input elements depending on mode:
   case 1:
      // manual settings:
      $("#DAC-Edit-table").find("input").prop('disabled', false);
      $("#DAC-Cali-table").find("input").prop('disabled', true);

      //    DACscrollArea->setEnabled(true);
      //    DACCaliFrame->setEnabled(false);

      break;
   case 2:
      // test structure
      $("#DAC-Edit-table").find("input").prop('disabled', true);
      $("#DAC-Cali-table").find("input").prop('disabled', true);

      //    DACscrollArea->setEnabled(false);
      //    DACCaliFrame->setEnabled(false);
      break;
   case 3:
      // calibrate mode
      $("#DAC-Edit-table").find("input").prop('disabled', true);
      $("#DAC-Cali-table").find("input").prop('disabled', false);
      $("#DacStart").prop('disabled', false);
      $("#DacOffset").prop('disabled', false);
      $("#DacDeltaOffset").prop('disabled', false);
      $("#DacCalibTime").prop('disabled', false);

      //    DACscrollArea->setEnabled(false);
      //    DACCaliFrame->setEnabled(true);
      //    DACStartValueLineEdit->setEnabled(true);
      //    DACStartValueLineEdit->setText (pre+text.setNum (fSetup.fDACStartValue, fNumberBase));
      //    DACOffsetLineEdit->setEnabled(true);
      //    DACDeltaOffsetLineEdit->setEnabled(true);
      //    DACCalibTimeLineEdit->setEnabled(true);

      break;
   case 4:
      // all constant
      $("#DAC-Edit-table").find("input").prop('disabled', true);
      $("#DAC-Cali-table").find("input").prop('disabled', false);
      $("#DacStart").prop('disabled', false);
      $("#DacOffset").prop('disabled', true);
      $("#DacDeltaOffset").prop('disabled', true);
      $("#DacCalibTime").prop('disabled', true);

      //    DACscrollArea->setEnabled(false);
      //    DACCaliFrame->setEnabled(true);
      //    DACStartValueLineEdit->setEnabled(true);
      //    DACStartValueLineEdit->setText (pre+text.setNum (fSetup.fDACAllValue, fNumberBase));
      //    DACOffsetLineEdit->setEnabled(false);
      //    DACDeltaOffsetLineEdit->setEnabled(false);
      //    DACCalibTimeLineEdit->setEnabled(false);

      break;
   default:
      console.log("!!! RefreshDAC Never come here - undefined DAC mode "
            + this.fDACMode);
      break;

   }
   ;

   if (!base)
      base = 16;
   var pre = base == 16 ? "0x" : "";

   document.getElementById("DacOffset").value = pre
         + this.fDACOffset.toString(base);
   document.getElementById("DacDeltaOffset").value = pre
         + this.fDACDelta.toString(base);
   document.getElementById("DacCalibTime").value = this.GetCalibrationTime()
         .toString();

   for ( var i = 0; i < POLAND_DAC_NUM; i++) {
      var elem = document.getElementById("DacEdit" + (i + 1));

      elem.value = pre + this.fDACValue[i].toString(base);
   }
}

PolandSetup.prototype.EvaluateDAC = function() {
   var options = document.getElementById("DacModeCombo").options;
   for ( var i = 0; i < options.length; i++)
      if (options[i].selected)
         this.fDACMode = Number(options[i].value);

   if (this.fDACMode == 4)
      this.fDACAllValue = parseInt(document.getElementById("DacStart").value);
   else
      this.fDACStartValue = parseInt(document.getElementById("DacStart").value);

   this.fDACOffset = parseInt(document.getElementById("DacOffset").value);
   this.fDACDelta = parseInt(document.getElementById("DacDeltaOffset").value);
   this
         .SetCalibrationTime(parseFloat(document
               .getElementById("DacCalibTime").value));

   if (this.fDACMode == 1)
      for ( var i = 0; i < POLAND_DAC_NUM; i++) {
         var elem = document.getElementById("DacEdit" + (i + 1));
         this.fDACValue[i] = parseInt(elem.value);
      }
}

PolandSetup.prototype.RefreshCounters = function(base) {
   if (!base)
      base = 16;
   var pre = base == 16 ? "0x" : "";

   document.getElementById("TriggerLbl").innerHTML = "<h2>" + pre
         + this.fEventCounter.toString(base) + "</h2>";

   var txt = "";

   for ( var i = 0; i < this.fErrorCounter.length; i++)
      txt += "<div align=\"center\" ;style=\"font-size:200%\" title=\"QFW "+(i+1).toString()+" Error counter\" <p>" + pre + this.fErrorCounter[i].toString(base) + "</p>  </div>";
// TODO: find out how to manipulate inner font size/weight correctly here!

    //   txt += "<div align=\"center\" ;style=\"width: 100% ; font-size:200%\" title=\"QFW "+(i+1).toString()+" Error counter\" <h2>" + pre + this.fErrorCounter[i].toString(base) + "</h2>  </div>";
   //   txt += "\<div  title=\"QFW "+(i+1).toString()+" Error counter\" \<b\>" + pre + this.fErrorCounter[i].toString(base) + "\</b\> \</div\>";


   //(document.getElementById("ErrorsLbl").outerHTML = txt;
   $("#ErrorsLbl").html(txt);
}

PolandSetup.prototype.RefreshTrigger = function() {
   var labelprefix = "Trigger";
   var labelstate = this.fTriggerAcceptance ? " <span style=\" color:#00ff00;\">ON </span>"
         : " <span style=\"color:#ff0000;\">OFF</span>";

   document.getElementById("buttonTriggerLabel").innerHTML = labelprefix
         + labelstate;

   //   var label = this.fTriggerAcceptance ? "<span class=\"styleGreen\"> -Trigger On- </span>" : "<span class=\"styleRed\"> -Trigger Off- </span>";

   //    document.getElementById("buttonTriggerLabel").innerHTML =label;
   if (this.fTriggerAcceptance) {
      $("#trigger_container").addClass("styleGreen").removeClass("styleRed");
   } else {
      $("#trigger_container").addClass("styleRed").removeClass("styleGreen");
   }
}

PolandSetup.prototype.GosipCommand = function(cmd, command_callback) {
   var xmlHttp = new XMLHttpRequest();

   var cmdtext = this.CmdUrl + "?sfp=" + this.fSFP + "&dev=" + this.fDEV;

   if (this.fLogging)
      cmdtext += "&log=1";

   cmdtext += "&cmd=\'" + cmd + "\'";

   console.log(cmdtext);

   xmlHttp.open('GET', cmdtext, true);

   var pthis = this;

   xmlHttp.onreadystatechange = function() {
      // console.log("onready change " + xmlHttp.readyState);
      if (xmlHttp.readyState == 4) {
         var reply = JSON.parse(xmlHttp.responseText);

         if (!reply || (reply["_Result_"] != 1)) {
            command_callback(false, null);
            return;
         }

         if (reply['log'] != null) {
            var ddd = "";
            // console.log("log length = " + Setup.fLogData.length);
            for ( var i in reply['log']) {

               if (reply['log'][i].search("\n") >= 0)
                  console.log("found 1");
               if (reply['log'][i].search("\\n") >= 0)
                  console.log("found 2");
               if (reply['log'][i].search("\\\n") >= 0)
                  console.log("found 3");

               ddd += "<pre>";
               ddd += reply['log'][i].replace(/\\n/g, "<br/>");
               ddd += "</pre>";
            }

            document.getElementById("logging").innerHTML += ddd;
            updateElementsSize();
         }

         command_callback(true, reply["res"]);
      }
   };

   xmlHttp.send(null);
}

PolandSetup.prototype.ReadRegisters = function(callback) {
   var regs = [];
   regs.push(POLAND_REG_INTERNAL_TRIGGER, POLAND_REG_MASTERMODE,
         POLAND_REG_TRIGCOUNT, POLAND_REG_QFW_MODE, POLAND_REG_TRIG_ON);

   for ( var i = 0; i < POLAND_TS_NUM; ++i) {
      regs.push(POLAND_REG_STEPS_BASE + 4 * i);
      regs.push(POLAND_REG_TIME_BASE + 4 * i);
   }

   // evaluate location of error counter info in token memory from real data length
   // for errorcounter base we have to use previous token payload.
   // due to asynchronous and chained gosipcmd request, we can not easily read actual steps values before preparing
   // readout of error registers. But pressing show button 2 times should do the job anyway
   var errcountstart = 4 * (32 + this.fSteps[0] * 32 + this.fSteps[1] * 32 + this.fSteps[2] * 32);
   for ( var e = 0; e < POLAND_ERRCOUNT_NUM; ++e) {
      regs.push(errcountstart + 4 * e);
   }

   regs.push(POLAND_REG_DAC_MODE, POLAND_REG_DAC_CAL_TIME,
         POLAND_REG_DAC_CAL_OFFSET, POLAND_REG_DAC_CAL_STARTVAL);

   for ( var d = 0; d < POLAND_DAC_NUM; ++d) {
      regs.push(POLAND_REG_DAC_BASE_READ + 4 * d);
   }

   var pthis = this;

   this.GosipCommand("[" + regs.toString() + "]", function(isok, res) {

      if (isok && (res.length != regs.length)) {
         console.log("return length mismatch");
         isok = false;
      }

      if (isok) {
         var indx = 0;

         pthis.fInternalTrigger = Number(res[indx++]);
         pthis.fTriggerMode = Number(res[indx++]);
         pthis.fEventCounter = Number(res[indx++]);
         pthis.fQFWMode = Number(res[indx++]);
         pthis.fTriggerAcceptance = Number(res[indx++]);

         for ( var i = 0; i < POLAND_TS_NUM; ++i) {
            pthis.fSteps[i] = Number(res[indx++]);
            pthis.fTimes[i] = Number(res[indx++]);
         }
         for ( var e = 0; e < POLAND_ERRCOUNT_NUM; ++e) {
            pthis.fErrorCounter[e] = Number(res[indx++]);
         }

         pthis.fDACMode = Number(res[indx++]);
         pthis.fDACCalibTime = Number(res[indx++]);
         pthis.fDACOffset = Number(res[indx++]);
         pthis.fDACStartValue = Number(res[indx++]);

         for ( var d = 0; d < POLAND_DAC_NUM; ++d) {
            pthis.fDACValue[d] = Number(res[indx++]);
         }
      }

      callback(isok);
   });
}

PolandSetup.prototype.SetRegisters = function(kind, callback) {
   // one could set "QFW", "DAC" or all registers

   // write register values from strucure with gosipcmd

   var regs = [];
   var sfpsave, devsave;
   if (kind == "QFW") {
      // writing of registers on QFW page

      if ((this.fSFP >= 0) && (this.fDEV >= 0)) {
         // change trigger modes only in single device
         regs.push([ POLAND_REG_INTERNAL_TRIGGER, this.fInternalTrigger ]);
         regs.push([ POLAND_REG_MASTERMODE, this.fTriggerMode ]);
      }

      regs.push([ POLAND_REG_QFW_MODE, this.fQFWMode ]);

      for ( var i = 0; i < POLAND_TS_NUM; ++i) {
         regs.push([ POLAND_REG_STEPS_BASE + 4 * i, this.fSteps[i] ]);
         regs.push([ POLAND_REG_TIME_BASE + 4 * i, this.fTimes[i] ]);
      }
   }

   if (kind == "DAC") {
      // writing of registers on DAC page

      regs.push([ POLAND_REG_DAC_MODE, this.fDACMode ]);

      switch (this.fDACMode) {
      case 1:
         // manual settings:
         for ( var d = 0; d < POLAND_DAC_NUM; ++d) {
            regs.push([ POLAND_REG_DAC_BASE_WRITE + 4 * d,
                  this.fDACValue[d] ]);
         }
         break;
      case 2:
         // test structure:
         // no more actions needed
         break;
      case 3:
         // issue calibration:
         regs.push([ POLAND_REG_DAC_CAL_STARTVAL, this.fDACStartValue ]);
         regs.push([ POLAND_REG_DAC_CAL_OFFSET, this.fDACOffset ]);
         regs.push([ POLAND_REG_DAC_CAL_DELTA, this.fDACDelta ]);
         regs.push([ POLAND_REG_DAC_CAL_TIME, this.fDACCalibTime ]);

         break;
      case 4:
         // all same constant value mode:
         regs.push([ POLAND_REG_DAC_ALLVAL, this.fDACAllValue ]);
         break;

      default:
         console.log("!!! ApplyDAC Never come here - undefined DAC mode "
               + this.fDACMode);
         break;

      }
      ;

      regs.push([ POLAND_REG_DAC_PROGRAM, 1 ]);
      regs.push([ POLAND_REG_DAC_PROGRAM, 0 ]);
   }

   if (kind == "TRIGGER") {
      // change trigger acceptance on all frontends:
      sfpsave = this.fSFP;
      devsave = this.fDEV;
      this.fSFP = -1;
      this.fDEV = -1;
      regs.push([ POLAND_REG_TRIG_ON, (this.fTriggerAcceptance ? 1 : 0) ]);

   }

   var cmdtext = "[";

   for ( var i = 0; i < regs.length; i++) {
      if (i > 0)
         cmdtext += ",";
      cmdtext += "\"-w adr " + regs[i][0] + " " + regs[i][1] + "\"";
   }

   cmdtext += "]";

   this.GosipCommand(cmdtext, function(isok, res) {
      if (isok && (res.length != regs.length)) {
         console.log("return length mismatch");
         isok = false;
      }
      callback(isok);
   });

   if (kind == "TRIGGER") {
      // change trigger acceptance on all frontends:
      this.fSFP = sfpsave;
      this.fDEV = devsave;
      // must restore original sfp and dev after broadcasting trigger acceptance state
   }

}

PolandSetup.prototype.DumpData = function(callback) {

}
//////  polandsetup class end
////////////////////////////////////////////////////////////////77
///// begin functions:

function ButtonAction() {
   console.log("sfp = " + $("#id_sfp").val() + "  dev = " + $("#id_dev").val()
         + " active = " + $("#tabs").tabs("option", "active"));
}

var Setup;

function ReadSlave() {
   if (document.getElementById("broadcast").checked) {
      Setup.fSFP = -1;
      Setup.fDEV = -1;
   } else {
      Setup.fSFP = Number($("#id_sfp").val());
      Setup.fDEV = Number($("#id_dev").val());
   }
   Setup.fLogging = document.getElementById("Verbose").checked;
}

function SetStatusMessage(info) {
   var d = new Date();
   var txt = " SFP:" + Setup.fSFP + " DEV:"
         + Setup.fDEV + " - " + d.toLocaleString() +"  >" + info;
   document.getElementById("status_message").innerHTML = txt;
}

function updateElementsSize() {

   $("#content_log").scrollTop($("#content_log")[0].scrollHeight - $("#content_log").height());

   $("#QFWModeCombo").selectmenu("option", "width", $('#QFW-table').width());
   $("#DacModeCombo").selectmenu("option", "width",
         $('#DAC-Cali-table').width());
}

function RefreshView(res) {

   SetStatusMessage("Reading registers " + (res ? "OK" : "Fail"));

   if (Setup.fLogData != null) {
      var ddd = "";
      // console.log("log length = " + Setup.fLogData.length);
      for ( var i in Setup.fLogData) {
         ddd += "<pre>";
         ddd += Setup.fLogData[i];
         ddd += "</pre>";
      }

      var logelement = document.getElementById("logging");
      logelement.innerHTML += ddd;
      logelement.scrollTop = logelement.scrollHeight;
      Setup.fLogData = null;
   }

   if (!res)
      return;

   var base = document.getElementById("HexMode").checked ? 16 : 10;

   Setup.RefreshQFW(base);

   Setup.RefreshDAC(base);

   Setup.RefreshCounters(base);

   Setup.RefreshTrigger();

   updateElementsSize();
}

function CompleteCommand(res) {

   SetStatusMessage("Writing registers " + (res ? "OK" : "Fail"));

   if (Setup.fLogData != null) {
      var ddd = "";
      //console.log("log length = " + Setup.fLogData.length);
      for ( var i in Setup.fLogData) {
         ddd += "<pre>";
         ddd += Setup.fLogData[i];
         ddd += "</pre>";
      }

      document.getElementById("logging").innerHTML += ddd;
      updateElementsSize();
      Setup.fLogData = null;
   }
}

$(function() {

   Setup = new PolandSetup("../CmdGosip/execute");

   $("#tabs").tabs({
      activate : function(event, ui) {
         updateElementsSize();
      }
   });

   $("#id_sfp").selectmenu();
   $("#id_dev").selectmenu();

   $("#broadcast").button().click(function() {

      if ($(this).is(':checked')) {
         $("#id_sfp").selectmenu("disable");
         $("#id_dev").selectmenu("disable");
         $("#MasterTrigger").prop('disabled', true);
         $("#InternalTrigger").prop('disabled', true);
         $("#FESAMode").prop('disabled', true);
         $("#buttonShow").prop('disabled', true);

      } else {
         $("#id_sfp").selectmenu("enable");
         $("#id_dev").selectmenu("enable");
         $("#MasterTrigger").prop('disabled', false);
         $("#InternalTrigger").prop('disabled', false);
         $("#FESAMode").prop('disabled', false);
         $("#buttonShow").prop('disabled', false);
      }
   });

   $("#HexMode").button().click(function() {
      RefreshView(true); // refresh everything with new number base
   });

   $("#Verbose").button()

   $("#buttonScanOffset").button().click(
         function() {
            ReadSlave();

            var response = confirm("Really scan offset for SFP chain "
                  + Setup.fSFP + ", Slave " + Setup.fDEV + "?");
            if (!response)
               return;

            SetStatusMessage("Doing offset measurement...");
            var cmd = "[\"-w adr " + POLAND_REG_DO_OFFSET + " 1\", \"-w adr "
                  + POLAND_REG_DO_OFFSET + " 0 ," +
                        " \"]";

            // read consecutively 32 offset registers and dump:
            var dumpcmd ="-d -r -x -- "+Setup.fSFP.toString()+" "+Setup.fDEV.toString() +" 0x"+POLAND_REG_OFFSET_BASE.toString(16) + " 0x20 ";

            Setup.GosipCommand(cmd, function(res) {
               SetStatusMessage("Scan offset " + (res ? "OK" : "Fail"));
               if (!res)
                  return;

               });
            Setup.fLogging = true;
                Setup.GosipCommand(dumpcmd, function(res) {
                  SetStatusMessage("Dump data after scan "
                        + (res ? "OK" : "Fail"));
                  Setup.fLogging = false;
               });
         });

   $("#buttonInitChain").button().click(
         function() {
            ReadSlave();
            var numslaves = prompt(
                  "Please specify NUMBER OF DEVICES to initialize at SFP "
                        + Setup.fSFP, "4");
            if (!numslaves)
               return;

            SetStatusMessage("Initializing chain...");

            var cmd = "-i " + Setup.fSFP + " " + numslaves;

            //Setup.fLogging = true;

            Setup.GosipCommand(cmd, function(res) {
               SetStatusMessage("Initialize chain "
                     + (res ? "OK" : "Fail"));
            });

         });
   $("#buttonReset").button().click(
         function() {
            ReadSlave();
            var requestmsg = "Really reset logic on POLAND device at SFP "
                  + Setup.fSFP + " Slave " + Setup.fDEV + "?";
            var response = confirm(requestmsg);
            if (!response)
               return;
            SetStatusMessage("Resetting logic on POLAND...");
            var cmd = "[\"-w adr " + POLAND_REG_RESET + " 0\", \"-w adr "
                  + POLAND_REG_RESET + " 1\"]";
            Setup.GosipCommand(cmd, function(res) {
               SetStatusMessage("Reset logic on POLAND "
                     + (res ? "OK" : "Fail"));
            });

         });
   $("#buttonResetPEX").button().click(function() {
      var requestmsg = "Really Reset gosip on pex board?";
      var response = confirm(requestmsg);
      if (!response)
         return;
      SetStatusMessage("Resetting PEXOR...");
      //Setup.fLogging = true;
      Setup.GosipCommand("-z", function(res) {
         SetStatusMessage("Reset of PEXOR " + (res ? "OK" : "Fail"));
      });
   });

   $("#buttonTriggerLabel").button();

   $("#buttonTrigger").button().click(
         function() {

            var requestmsg = "Really Change frontend Trigger acceptance?";
            if (Setup.fTriggerAcceptance)
               requestmsg = "Really disable Frontend Trigger acceptance?";
            else
               requestmsg = "Really enable Frontend Trigger acceptance?";

            var response = confirm(requestmsg);
            if (!response)
               return;
            Setup.fTriggerAcceptance ? Setup.fTriggerAcceptance = false
                  : Setup.fTriggerAcceptance = true;
            var value = (Setup.fTriggerAcceptance ? 1 : 0);
            var state = (Setup.fTriggerAcceptance ? "ON" : "OFF");
            SetStatusMessage("--- Set all devices trigger acceptance to "
                  + state);
            Setup.SetRegisters("TRIGGER", CompleteCommand);
            Setup.RefreshTrigger();

         });

   $("#DacModeCombo").selectmenu({
      change : function(event, ui) {
         Setup.fDACMode = Number(ui.item.value);
         Setup.RefreshDAC();
      }
   });

   $("#QFWModeCombo").selectmenu({
      change : function(event, ui) {
         Setup.fQFWMode = ui.item.value;
      }
   });

   $("#buttonShow").button().click(function() {
      ReadSlave();
      SetStatusMessage("Start register reading...");
      Setup.ReadRegisters(RefreshView);
   });

   $("#buttonApply").button().click(
         function() {
            ReadSlave();
            var cmd = ($("#tabs").tabs("option", "active") == 0) ? "QFW"
                  : "DAC";
            var requestmsg = "Really apply " + cmd + " settings  to SFP"
                  + +Setup.fSFP + " Device " + Setup.fDEV + "?";
            var response = confirm(requestmsg);
            if (!response)
               return;
            Setup.EvaluateDAC();
            Setup.EvaluateQFW();
            SetStatusMessage("Start writing " + cmd);
            Setup.SetRegisters(cmd, CompleteCommand);
         });

   $("#buttonConfigure").button().click(function() {
      //ButtonAction();

      var configfilename = prompt(
            "Please type name of configuration file (*.gos) on server"
                  , "initqfw.gos");
      if (!configfilename)
         return;

      SetStatusMessage("Apply configuration file "+configfilename+" ...");
//
      var cmd = "-x -c " + configfilename +" ";
//
//      Setup.fLogging = true;
//
      Setup.GosipCommand(cmd, function(res) {
         SetStatusMessage("Configuration with "+ configfilename
               + (res ? " - OK" : " -Failed!"));
      });
   });

   $("#buttonDataDump")
         .button()
         .click(
               function() {
                  ReadSlave();
                  Setup.fLogging = true;
                  SetStatusMessage("Dumping data...");
                  var numwords = 32 + Number(Setup.fSteps[0]) * 32
                        + Number(Setup.fSteps[1]) * 32
                        + Number(Setup.fSteps[2]) * 32 + 32;
                  if (numwords > 512)
                     numwords = 512;

                  document.getElementById("logging").innerHTML += "--- Register Dump ---<br/>";

                  Setup.GosipCommand("-d -r -x adr 0 0x"
                        + numwords.toString(16), function(res) {
                     SetStatusMessage("Dump data "
                           + (res ? "OK" : "Fail"));
                  });
                  updateElementsSize();
               });

   $("#buttonClear")
         .button()
         .click(
               function() {
                  document.getElementById("logging").innerHTML = "Welcome to POLAND Web GUI!<br/>  -    v0.5, 6-November 2014 by S. Linev/ J. Adamzewski-Musch (JAM)<br/>";
                  updateElementsSize();
               });


   $( document ).tooltip();

});

function initGUI() {
   var base = document.getElementById("HexMode").checked ? 16 : 10;
   Setup.fDACMode = 1;
   Setup.RefreshQFW(base);
   Setup.RefreshDAC(base);
   Setup.RefreshCounters(base);
   $("#buttonClear").button().click();

   updateElementsSize();
}

$(document).ready(initGUI);

$(window).on('resize', function() {
   updateElementsSize();
});
