package edu.cmu.cs.cloudlet.android.network;

import java.io.IOException;
import java.util.ArrayList;
import java.util.Vector;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import edu.cmu.cs.cloudlet.android.CloudletActivity;
import edu.cmu.cs.cloudlet.android.R;
import edu.cmu.cs.cloudlet.android.application.CloudletCameraActivity;
import edu.cmu.cs.cloudlet.android.application.face.ui.FaceRecClientCameraPreview;
import edu.cmu.cs.cloudlet.android.data.Measure;
import edu.cmu.cs.cloudlet.android.data.VMInfo;
import edu.cmu.cs.cloudlet.android.util.CloudletEnv;
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

	private VMInfo requestBaseVM;

	public CloudletConnector(CloudletActivity activity, Context context) {
		this.activity = activity;
		this.mContext = context;
	}
	
	public void startConnection(String ipAddress, int port) {
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
		mDialog.setMessage("Step 1. Waiting for VM Lists");
		
		// Send VM Request Message 
		NetworkMsg networkMsg = NetworkMsg.MSG_OverlayList();
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
				if(networkClient != null){
					networkClient.close();
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
				Bundle data = msg.getData();
				NetworkMsg response = (NetworkMsg) msg.obj;
								
				// Check JSON there is error message or not.
				if(checkErrorStutus(response.getJsonPayload()) == true){
					return;
				}
				
				// Handle Cloudlet response
				if(response != null){
					switch(response.getCommandNumber()){
					case NetworkMsg.COMMAND_ACK_VMLIST: 						
						KLog.println("1. COMMAND_ACK_VMLIST message received");
						updateMessage("Step 2. Sending overlay VM ..");
						Measure.put(Measure.NET_ACK_VMLIST);
						responseVMList(response);
						break;
					case NetworkMsg.COMMAND_ACK_TRANSFER_START:
						KLog.println("2. COMMAND_ACK_TRANSFER_START message received");
						updateMessage("Step 3. Waiting for VM Synthesis ..");
						Measure.put(Measure.NET_ACK_OVERLAY_TRASFER);
						responseTransferStart(response);
						break;
					case NetworkMsg.COMMAND_ACK_VM_LAUNCH:
						KLog.println("3. COMMAND_ACK_VM_LAUNCH message received");
						updateMessage("Step 3. VM is ready.");
						Measure.put(Measure.VM_LAUNCHED);
						responseVMLaunch(response);
						mDialog.dismiss();
						break;
					case NetworkMsg.COMMAND_ACK_VM_STOP:
						KLog.println("4. COMMAND_ACK_VM_STOP message received");
						responseVMStop(response);
						break;
					default:
						break;
					}
				}
			}
		}
	};

	private void responseVMList(NetworkMsg response) {
		ArrayList<VMInfo> overlayVMList = CloudletEnv.instance().getOverlayDirectoryInfo();
		Vector<VMInfo> matchingVMList = new Vector<VMInfo>();
		VMInfo overlayVM = null;
		
		// find matching VM
		/*
		try {
			JSONObject json = response.getJsonPayload();
			JSONArray vms = json.getJSONArray("VM");
			for(int i = 0; i < vms.length(); i++){
				JSONObject vm = (JSONObject) vms.get(i);
				VMInfo newVM = new VMInfo(vm);
				for(int j = 0; j < overlayVMList.size(); j++){
					// matching with mobile overlay list
					String name = overlayVMList.get(j).getInfo(VMInfo.JSON_KEY_NAME);
					if(name.equalsIgnoreCase(newVM.getInfo(VMInfo.JSON_KEY_NAME))){
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
		*/
		// For Test, Select the first VM
		try{
			JSONObject json = response.getJsonPayload();
			JSONArray vms = json.getJSONArray("VM");
			for(int i = 0; i < Math.min(vms.length(), 1); i++){
				JSONObject vm = (JSONObject) vms.get(i);
				VMInfo newVM = new VMInfo(vm);
				matchingVMList.add(newVM);
				overlayVM = overlayVMList.get(0);
			}
		} catch (JSONException e) {
			e.printStackTrace();
		}
		
		// Send VM Transfer Message
		if(matchingVMList.size() == 1){
			VMInfo baseVM = matchingVMList.get(0);
			NetworkMsg sendMsg = NetworkMsg.MSG_SelectedVM(baseVM, overlayVM);
			this.networkClient.requestCommand(sendMsg);
			
			//save requested VM Information
			this.requestBaseVM = baseVM;								
			
		}else{
			// No matching VM List
			this.showAlertDialog("No Matching VM with Cloudlet");
		}		
	}

	private void responseTransferStart(NetworkMsg response) {
		// Now do nothing			
	}

	private void responseVMLaunch(NetworkMsg response) {
		final String ipaddress;
		try {
			ipaddress = response.getJsonPayload().getString(VMInfo.JSON_KEY_LAUNCH_VM_IP);
			if(checkVMValidity(response, this.requestBaseVM) == true){
				String synthesisInfo = "Finish VM Synthesis\n" + Measure.printInfo() + "\nServer IP: " + ipaddress;
				// Run Application
				new AlertDialog.Builder(mContext).setTitle("SUCCESS")
				.setMessage(synthesisInfo)
				.setPositiveButton("Run App", new DialogInterface.OnClickListener(){
					public void onClick(DialogInterface dialog, int which) {
						ArrayList<VMInfo> vmList = CloudletEnv.instance().getOverlayDirectoryInfo();
						if(vmList == null && vmList.size() != 1){
							// Error
							showAlertDialog("No Overlay VM List is suit");
							return;
						}
						
						String VMName = vmList.get(0).getInfo(VMInfo.JSON_KEY_NAME);
						if(VMName == null){
							// Error
							showAlertDialog("VM Name is NULL");
							return;
						}
						
						// Launch Application
						activity.runApplication(VMName);
						
					}					
				})
				.setNegativeButton("Done", null)
				.show();
			}else{
				this.showAlertDialog("Retuned VM information is wrong, check # of received VM Information");		
			}
		} catch (JSONException e) {
			KLog.printErr(e.toString());
		}
	}

	private void responseVMStop(NetworkMsg response) {
		ArrayList<VMInfo> vms = response.getVMList();
		
		// Get Residue URL
		
		// Get Residue Data
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
	
	private boolean checkVMValidity(NetworkMsg response, VMInfo requestBaseVM2) {
		JSONObject json = response.getJsonPayload();
		JSONArray vms = null;
		try {
			vms = json.getJSONArray("VM");
		} catch (JSONException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
				
		if(vms != null && vms.length() == 1){
			return true;
		}else{
			return false;
		}
			
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
