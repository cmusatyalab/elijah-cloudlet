//
// Copyright (C) 2011-2012 Carnegie Mellon University
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of version 2 of the GNU General Public License as published
// by the Free Software Foundation.  A copy of the GNU General Public License
// should have been distributed along with this program in the file
// LICENSE.GPL.

// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
// or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
// for more details.
//
package edu.cmu.cs.cloudlet.android;

import java.util.ArrayList;
import org.teleal.cling.android.AndroidUpnpServiceImpl;

import edu.cmu.cs.cloudlet.android.application.CloudletCameraActivity;
import edu.cmu.cs.cloudlet.android.data.VMInfo;
import edu.cmu.cs.cloudlet.android.network.CloudletConnector;
import edu.cmu.cs.cloudlet.android.upnp.DeviceDisplay;
import edu.cmu.cs.cloudlet.android.upnp.UPnPDiscovery;
import edu.cmu.cs.cloudlet.android.util.CloudletEnv;
import edu.cmu.cs.cloudlet.android.util.KLog;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.view.KeyEvent;
import android.view.View;
import android.widget.Button;

public class CloudletActivity extends Activity {
	public static final String SYNTHESIS_SERVER_IP = "dagama.isr.cs.cmu.edu";	// Cloudlet IP Address
	public static final int SYNTHESIS_PORT = 8021;								// Cloudlet port for VM Synthesis test 

	public static final String[] applications = {"MOPED", "MOPED_Disk", "FACE", "Speech", "NULL"};
	public static final int TEST_CLOUDLET_APP_MOPED_PORT = 19092;
	public static final int TEST_CLOUDLET_APP_FACE_PORT = 9876;
	private static final int TEST_CLOUDLET_APP_SPEECH_PORT = 6789;
	
	protected Button startConnectionButton;
	protected CloudletConnector connector;
	private UPnPDiscovery serviceDiscovery;
	protected int selectedOveralyIndex;

	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.main);
				
		// Initiate Environment Settings
		CloudletEnv.instance();

		// upnp service binding and show dialog
//		this.serviceDiscovery = new UPnPDiscovery(this, CloudletActivity.this, discoveryHandler);
//		getApplicationContext().bindService(new Intent(this, AndroidUpnpServiceImpl.class), this.serviceDiscovery.serviceConnection, Context.BIND_AUTO_CREATE);	
//		serviceDiscovery.showDialogSelectOption();
		
		this.connector = new CloudletConnector(this, CloudletActivity.this);

		// Performance Button
		findViewById(R.id.testSynthesis).setOnClickListener(clickListener);;
		findViewById(R.id.runApplication).setOnClickListener(clickListener);
	}
	
	
	private void showDialogSelectOverlay(final ArrayList<VMInfo> vmList) {
		String[] nameList = new String[vmList.size()];
		for(int i = 0; i < nameList.length; i++){
			nameList[i] = new String(vmList.get(i).getInfo(VMInfo.JSON_KEY_NAME));			
		}
		
		AlertDialog.Builder ab = new AlertDialog.Builder(this);
		ab.setTitle("Overlay List");
		ab.setIcon(R.drawable.ic_launcher);
		ab.setSingleChoiceItems(nameList, 0, new DialogInterface.OnClickListener() {
			public void onClick(DialogInterface dialog, int position) {
				selectedOveralyIndex = position;
			}
		}).setPositiveButton("Ok", new DialogInterface.OnClickListener() {
			public void onClick(DialogInterface dialog, int position) {
				if(position >= 0){
					selectedOveralyIndex = position;
				}
                VMInfo overlayVM = vmList.get(selectedOveralyIndex);
                runConnection(SYNTHESIS_SERVER_IP, SYNTHESIS_PORT, overlayVM);
			}
		}).setNegativeButton("Cancel", new DialogInterface.OnClickListener() {
			public void onClick(DialogInterface dialog, int position) {
				return;
			}
		});
		ab.show();
	}
	
	/*
	 * Synthesis initiation through HTTP Post
	 */
	protected void runConnection(String address, int port, VMInfo overlayVM) {
		connector.startConnection(address, port, overlayVM);
	}

	/*
	 * Launch Application as a Stand-Alone Program
	 */
	private void showDialogSelectApp(final String[] applications) {
		// Show Dialog
		AlertDialog.Builder ab = new AlertDialog.Builder(this);
		ab.setTitle("Application List");
		ab.setIcon(R.drawable.ic_launcher);
		ab.setSingleChoiceItems(applications, 0, new DialogInterface.OnClickListener() {
			public void onClick(DialogInterface dialog, int position) {
				selectedOveralyIndex = position;
			}
		}).setPositiveButton("Ok", new DialogInterface.OnClickListener() {
			public void onClick(DialogInterface dialog, int position) {
				if(position >= 0){
					selectedOveralyIndex = position;
				}else if(applications.length > 0 && selectedOveralyIndex == -1){
					selectedOveralyIndex = 0;
				}
				String application = applications[selectedOveralyIndex];
				runStandAlone(application);
			}
		}).setNegativeButton("Cancel", new DialogInterface.OnClickListener() {
			public void onClick(DialogInterface dialog, int position) {
				return;
			}
		});
		ab.show();
	}	
	
	public void runStandAlone(String application) {
		application = application.trim();
		
		if(application.toLowerCase().startsWith("moped")){
			Intent intent = new Intent(CloudletActivity.this, CloudletCameraActivity.class);			
			intent.putExtra("address", SYNTHESIS_SERVER_IP);
			intent.putExtra("port", TEST_CLOUDLET_APP_MOPED_PORT);
			startActivityForResult(intent, 0);			
		}else if(application.equalsIgnoreCase("null")){
			showAlert("Info", "NUll has no UI");
		}else{
			showAlert("Error", "NO such Application : " + application);			
		}
	}
	
	/*
	 * Service Discovery Handler
	 */
	Handler discoveryHandler = new Handler() {
		public void handleMessage(Message msg) {
			if (msg.what == UPnPDiscovery.DEVICE_SELECTED) {
				DeviceDisplay device = (DeviceDisplay) msg.obj;
				String ipAddress = device.getIPAddress();
				int port = device.getPort();
				KLog.println("ip : " + ipAddress + ", port : " + port);
				
//				connector.startConnection(ipAddress, 9090);
//				connector.startConnection("128.2.212.207", 9090);
				
			}else if(msg.what == UPnPDiscovery.USER_CANCELED){
				showAlert("Info", "Select UPnP Server for Cloudlet Service");
			}
		}
	};

	View.OnClickListener clickListener = new View.OnClickListener() {
		@Override
		public void onClick(View v) {			
			if(v.getId() == R.id.testSynthesis){
				// Find All overlay and let user select one of them.
				CloudletEnv.instance().resetOverlayList();
				ArrayList<VMInfo> vmList = CloudletEnv.instance().getOverlayDirectoryInfo();
				if(vmList.size() > 0){
					showDialogSelectOverlay(vmList);			
				}else{
					showAlert("Error", "We found No Overlay");
				}			
			}else if(v.getId() == R.id.runApplication){
				showDialogSelectApp(applications);
			}
		}
	};
	
	public void showAlert(String type, String message){
		new AlertDialog.Builder(CloudletActivity.this).setTitle(type)
		.setMessage(message)
		.setIcon(R.drawable.ic_launcher)
		.setNegativeButton("Confirm", null)
		.show();		
	}


	@Override 
    protected void onActivityResult(int requestCode, int resultCode, Intent data) { 
        super.onActivityResult(requestCode, resultCode, data); 
        if (resultCode == RESULT_OK) { 
            if (requestCode == 0) {  
                String ret = data.getExtras().getString("message"); 
            } 
        } 
    } 
	
	public boolean onKeyDown(int keyCode, KeyEvent event) {
		if (keyCode == KeyEvent.KEYCODE_BACK) {
			new AlertDialog.Builder(CloudletActivity.this).setTitle("Exit").setMessage("Finish Application")
					.setPositiveButton("Confirm", new DialogInterface.OnClickListener() {
						public void onClick(DialogInterface dialog, int which) {
							moveTaskToBack(true);
							finish();
						}
					}).setNegativeButton("Cancel", null).show();
		}
		return super.onKeyDown(keyCode, event);
	}

	@Override
	public void onDestroy() {
		if(serviceDiscovery != null){
			getApplicationContext().unbindService(this.serviceDiscovery.serviceConnection);
			this.serviceDiscovery.close();
		}
		this.connector.close();
		super.onDestroy();
	}
}
