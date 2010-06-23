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
package xgui;
import java.util.*;
import java.lang.*;
import javax.swing.SwingUtilities;
import dim.*;

/**
 * InfoHandler to manage list of DIM servers. List is composed in a text area.
 * DIM parameter is DIS_DNS/SERVER_LIST.
 * @author Hans G. Essel
 * @version 1.0
 * @see xDesktop
 */
public class xDimServiceList extends DimInfo implements Runnable{
private String servicelist;
private int nServices=0;
private int updates, chars;
private boolean create;
private boolean remove;
private xiDesktop desk;

/**
 * Constructor of DIM parameter handler.
 * @param service DIM name of service: DIS_DNS/SERVER_LIST
 * @param label Text area to store the DIM server list. 
 */
public xDimServiceList(String service, xiDesktop desktop){
    super(service,"none");
    desk=desktop;
	System.out.println(" ****** register "+service);
}
public void run(){
if(chars>0)
	System.out.println(getName()+" "+servicelist.charAt(0)+" Service list "+chars+" update "+create);	
  if(create){
	  desk.updateDim(false);  
  }
}
/**
 * The DIM parameter DIS_DNS/SERVICE_LIST is a list of services.
 * @see xDimBrowser 
 */
public void infoHandler(){
	updates++;
    servicelist=new String(getString());
    chars=servicelist.length();
    create=servicelist.startsWith("+");
    remove=servicelist.startsWith("-");
    if(!remove)SwingUtilities.invokeLater(this);
}
}
