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
package edu.cmu.cs.cloudlet.android.network;

import java.io.IOException;
import java.util.ArrayList;
import java.util.Vector;

import org.apache.http.util.ByteArrayBuffer;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import edu.cmu.cs.cloudlet.android.CloudletActivity;
import edu.cmu.cs.cloudlet.android.R;
import edu.cmu.cs.cloudlet.android.application.CloudletCameraActivity;
import edu.cmu.cs.cloudlet.android.data.Measure;
import edu.cmu.cs.cloudlet.android.data.VMInfo;
import edu.cmu.cs.cloudlet.android.util.KLog;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.util.TypedValue;
import android.view.View;
import android.widget.TextView;

public class CloudletConnector {
	public static final int CONNECTION_ERROR			 	= 1;
	public static final int NETWORK_ERROR 					= 2;
	public static final int PROGRESS_MESSAGE				= 3;
	public static final int FINISH_MESSAGE					= 4;
	public static final int PROGRESS_MESSAGE_TRANFER		= 5;
	
	protected CloudletActivity activity;
	protected Context mContext;
	protected NetworkClientSender networkClient;
	protected ProgressDialog mDialog;

	private VMInfo requestBaseVMInfo;

	public CloudletConnector(CloudletActivity activity, Context context) {
		this.activity = activity;
		this.mContext = context;
	}
	
	public void startConnection(String ipAddress, int port, VMInfo requestBaseVMInfo) {
		this.requestBaseVMInfo = requestBaseVMInfo; 
		if(mDialog == null){						 
			mDialog = ProgressDialog.show(this.mContext, "Info", "Connecting to " + ipAddress , true);
			mDialog.setIcon(R.drawable.ic_launcher);
		}else{
			mDialog.setMessage("Connecting to " + ipAddress);
		}
		mDialog.show();

		if(networkClient != null){
			networkClient.close();
			networkClient.stop();
		}
		networkClient = new NetworkClientSender(this.mContext, eventHandler);
		networkClient.setConnector(this);
		networkClient.setConnection(ipAddress, port);		
		networkClient.start();
		mDialog.setMessage("Step 1. Requesting VM Synthesis");
		
		// Send VM Request Message 
		NetworkMsg networkMsg = NetworkMsg.MSG_SelectedVM(this.requestBaseVMInfo);
		networkClient.requestCommand(networkMsg);
	}
	
	public void updateMessage(String dialogMessage){
		if(mDialog != null && mDialog.isShowing()){
			mDialog.setMessage(dialogMessage);
		}
	}

	public void close() {
		if(this.networkClient != null)
			this.networkClient.close();
	}
	

	/*
	 * Server Message Handler
	 */
	Handler eventHandler = new Handler() {
		public void handleMessage(Message msg) {
			if(msg.what == CloudletConnector.CONNECTION_ERROR){
				if(mDialog != null){
					mDialog.dismiss();
				}
				
				String message = msg.getData().getString("message");
				showAlertDialog(message);
			}else if(msg.what == CloudletConnector.NETWORK_ERROR){
				if(mDialog != null){
					mDialog.dismiss();
				}
				if(networkClient != null){
					networkClient.close();
				}
				String message = msg.getData().getString("message");
				
				showAlertDialog(message);
			}else if(msg.what == CloudletConnector.PROGRESS_MESSAGE){
				// Response parsing
				ByteArrayBuffer responseArray = (ByteArrayBuffer) msg.obj;
				String resString = new String(responseArray.toByteArray());
				KLog.println(resString);
				NetworkMsg response = null;
				try {
					response = new NetworkMsg(new JSONObject(resString));
				} catch (JSONException e) {
					KLog.printErr(e.toString());
					showAlertDialog("Not valid JSON response : " + resString);
					return;
				}				
								
				// Check JSON to see an error message if it has.
				if(checkErrorStutus(response.getJsonPayload()) == true){
					return;
				}
				
				// Handle Response
				if(response != null){
					switch(response.getCommandType()){
					case NetworkMsg.COMMAND_ACK_TRANSFER_START:
						KLog.println("2. COMMAND_ACK_TRANSFER_START message received");
						updateMessage("Step 2. Synthesis is done..");
						Measure.put(Measure.NET_ACK_OVERLAY_TRASFER);

						if(mDialog != null){
							mDialog.dismiss();
						}
						responseTransferStart(response);
						break;
					}
				}
			}
		}
	};

	private void responseTransferStart(NetworkMsg response) {
		try {
			String ipaddress = response.getJsonPayload().getString(VMInfo.JSON_KEY_LAUNCH_VM_IP);
			activity.runStandAlone(this.requestBaseVMInfo.getInfo(VMInfo.JSON_KEY_NAME));			
		} catch (JSONException e) {
			KLog.printErr(e.toString());
		}
	}
	
	/*
	 * Check Error Message in JSON
	 */
	private boolean checkErrorStutus(JSONObject json) {
		String errorMsg = null;
		try {
			errorMsg = json.getString("Error");
		} catch (JSONException e) {
		}
		if(errorMsg != null && errorMsg.length() > 0){
			// Response is error message
			showAlertDialog(errorMsg);
			return true;
		}
		return false;
	}
	
	private void showAlertDialog(String errorMsg) {
		if(activity.isFinishing() == false){
			new AlertDialog.Builder(mContext).setTitle("Error")
			.setIcon(R.drawable.ic_launcher)
			.setMessage(errorMsg)
			.setNegativeButton("Confirm", null)
			.show();			
		}
	}
	
	/*
	 * Callback function after VM Synthesis
	 */
	public DialogInterface.OnClickListener launchApplication = new DialogInterface.OnClickListener() {
		@Override
		public void onClick(DialogInterface dialog, int which) {
			Intent intent = new Intent(mContext, CloudletCameraActivity.class);
			intent.putExtra("address", "desk.krha.kr");
			activity.startActivityForResult(intent, 0);			
		}
	};
}
