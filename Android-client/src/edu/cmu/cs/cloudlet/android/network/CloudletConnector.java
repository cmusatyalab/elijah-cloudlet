package edu.cmu.cs.cloudlet.android.network;

import java.io.IOException;
import java.util.ArrayList;
import java.util.Vector;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import edu.cmu.cs.cloudlet.android.R;
import edu.cmu.cs.cloudlet.android.data.VMInfo;
import edu.cmu.cs.cloudlet.android.util.CloudletEnv;
import edu.cmu.cs.cloudlet.android.util.KLog;

import android.app.AlertDialog;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.util.TypedValue;
import android.view.View;
import android.widget.TextView;

public class CloudletConnector {
	public static final int CONNECTION_ERROR	 	= 1;
	public static final int NETWORK_ERROR 			= 2;
	public static final int PROGRESS_MESSAGE		= 3;
	public static final int FINISH_MESSAGE			= 4;
	
	protected Context mContext;
	protected NetworkClientSender networkClient;

	protected ProgressDialog mDialog;
	protected StringBuffer messageBuffer;

	public CloudletConnector(Context context) {
		this.mContext = context;
		networkClient = new NetworkClientSender(context, eventHandler);
		messageBuffer = new StringBuffer();
	}
	
	public void startConnection(String ipAddress, int port) {
		if(mDialog == null){						 
			mDialog = ProgressDialog.show(this.mContext, "Info", "Connecting to " + ipAddress , true);
			mDialog.setIcon(R.drawable.ic_launcher);
		}else{
			mDialog.setMessage("Connecting to " + ipAddress);
		}
		mDialog.show();
		
		networkClient.setConnection(ipAddress, port); 
		networkClient.start();
		
		// Send VM Request Message 
		NetworkMsg networkMsg = NetworkMsg.MSG_OverlayList();
		networkClient.requestCommand(networkMsg);
	}
	
	public View.OnClickListener protocolTestClickListener = new View.OnClickListener() {
		public void onClick(View v) {
			NetworkMsg networkMsg = null;
			VMInfo vm = CloudletEnv.instance().getOverlayDirectoryInfo().get(0);
			
			switch(v.getId()){
			case R.id.protocol_test1:
				// Send REQ VM List
				networkMsg = NetworkMsg.MSG_OverlayList();
				break;
			case R.id.protocol_test2:
				// Send REQ Transfer_Start
				networkMsg = NetworkMsg.MSG_SelectedVM(vm, vm);
				break;
			case R.id.protocol_test3:				
				// REQ Launch
				networkMsg = NetworkMsg.MSG_LaunchVM(vm, 4, 512);
				break;
			case R.id.protocol_test4:
				// REQ Stop
				networkMsg = NetworkMsg.MSG_StopVM(vm);
				break;
			case R.id.protocol_test5:
			break;
			}
			
			networkMsg.printJSON();
			networkClient.requestCommand(networkMsg);
		}
	};

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
				String message = msg.getData().getString("message");
				showAlertDialog(message);
			}else if(msg.what == CloudletConnector.PROGRESS_MESSAGE){
				Bundle data = msg.getData();
				NetworkMsg response = (NetworkMsg) msg.obj;
				
				// Display to Dialog
				messageBuffer.append(data.getString("message") + "\n");
				KLog.println(messageBuffer.toString());				
				if(mDialog != null){
//					mDialog = ProgressDialog.show(mContext, "Info", messageBuffer.toString(), true);
					mDialog.dismiss();					
				}
				
				// Check JSON there is error message or not.
				if(checkErrorStutus(response.getJsonPayload()) == true){
					return;
				}
				
				// Handle Cloudlet response
				if(response != null){
					switch(response.getCommandNumber()){
					case NetworkMsg.COMMAND_ACK_VMLIST:
						KLog.println("COMMAND_ACK_VMLIST message received");
						responseVMList(response);
						break;
					case NetworkMsg.COMMAND_ACK_TRANSFER_START:
						KLog.println("COMMAND_ACK_TRANSFER_START message received");
						responseTransferStart(response);
						break;
					case NetworkMsg.COMMAND_ACK_VM_LAUNCH:
						KLog.println("COMMAND_ACK_VM_LAUNCH message received");
						responseVMLaunch(response);
						break;
					case NetworkMsg.COMMAND_ACK_VM_STOP:
						KLog.println("COMMAND_ACK_VM_STOP message received");
						responseVMStop(response);
						break;
					default:
						break;
					}
				}
			}else if(msg.what == CloudletConnector.FINISH_MESSAGE){
				if(mDialog != null && mDialog.isShowing() == true){
					mDialog.dismiss();
					
					// Do network work
				}
			}
		}
	};

	private void responseVMList(NetworkMsg response) {
		ArrayList<VMInfo> overlayVMList = CloudletEnv.instance().getOverlayDirectoryInfo();
		Vector<VMInfo> matchingVMList = new Vector<VMInfo>();
		VMInfo overlayVM = null;
		
		try {
			JSONObject json = response.getJsonPayload();
			JSONArray vms = json.getJSONArray("VM");
			for(int i = 0; i < vms.length(); i++){
				JSONObject vm = (JSONObject) vms.get(i);
				VMInfo newVM = new VMInfo(vm);
				for(int j = 0; j < overlayVMList.size(); j++){
					// matching with mobile overlay list
					String name = overlayVMList.get(j).getInfo(VMInfo.KEY_NAME);
					if(name.equalsIgnoreCase(newVM.getInfo(VMInfo.KEY_NAME))){
						if(matchingVMList.contains(newVM) == false){
							matchingVMList.add(newVM);
							overlayVM = overlayVMList.get(j);
						}
					}
				}
				KLog.println(newVM.toJSON().toString());
			}
		} catch (JSONException e) {
			KLog.printErr(e.toString());
		}
		
		// Send VM Transfer Message
		if(matchingVMList.size() == 1){
			VMInfo baseVM = matchingVMList.get(0);
			NetworkMsg sendMsg = NetworkMsg.MSG_SelectedVM(baseVM, overlayVM);
			this.networkClient.requestCommand(sendMsg);								
			
		}else{
			// No matching VM List
			this.showAlertDialog("No Matching VM with Cloudlet");
		}
		
	}

	private void responseTransferStart(NetworkMsg response) {
	}
	
	private void responseVMLaunch(NetworkMsg response) {
	}

	private void responseVMStop(NetworkMsg response) {
	}


	
	/*
	 * Check Error Message in JSON
	 */
	private boolean checkErrorStutus(JSONObject json) {
		String errorMsg = null;;
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
		new AlertDialog.Builder(mContext).setTitle("Error")
		.setMessage(errorMsg)
		.setNegativeButton("Confirm", null)
		.show();
	}
}
