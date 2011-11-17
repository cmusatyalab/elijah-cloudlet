package edu.cmu.cs.cloudlet.android.network;

import java.io.IOException;
import java.net.UnknownHostException;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import edu.cmu.cs.cloudlet.android.CloudletActivity;
import edu.cmu.cs.cloudlet.android.R;
import edu.cmu.cs.cloudlet.android.data.VMInfo;
import edu.cmu.cs.cloudlet.android.util.KLog;

import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.os.Handler;
import android.os.Message;
import android.view.View;

public class CloudletConnector {
	protected Context mContext;
	protected NetworkClientSender networkClient;

	public CloudletConnector(Context context) {
		this.mContext = context;
		networkClient = new NetworkClientSender(context, eventHandler);
	}

	/*
	 * Server Message Handler
	 */
	Handler eventHandler = new Handler() {
		public void handleMessage(Message msg) {
		}
	};
	
	public void startConnection(String ipAddress, int port) {
		try {
			networkClient.initConnection(ipAddress, port); 
			networkClient.start();
			
		} catch (UnknownHostException e) {
			new AlertDialog.Builder(mContext).setTitle("Error")
			.setMessage("Unkown host: " + ipAddress + "(" + port + ")")
			.setNegativeButton("Confirm", null)
			.show();
			return;
		} catch (IOException e) {			
			new AlertDialog.Builder(mContext).setTitle("Error")
			.setMessage("IOException : " + e.toString())
			.setNegativeButton("Confirm", null)
			.show();
			return;
		}
	}
	
	public View.OnClickListener protocolTestClickListener = new View.OnClickListener() {
		public void onClick(View v) {
			NetworkMsg networkMsg = null;
			
			switch(v.getId()){
			case R.id.protocol_test1:
				// Send REQ VM List
				networkMsg = NetworkMsg.MSG_OverlayList();
				break;
			case R.id.protocol_test2:
				// Send REQ Transfer_Start
				VMInfo vm2 = new VMInfo().getTestVMInfo();
				networkMsg = NetworkMsg.MSG_SelectedVM(vm2);
				break;
			case R.id.protocol_test3:				
				// REQ Launch
				VMInfo vm3 = new VMInfo().getTestVMInfo();
				networkMsg = NetworkMsg.MSG_LaunchVM(vm3, 4, 512);
				break;
			case R.id.protocol_test4:
				// REQ Stop
				VMInfo vm4 = new VMInfo().getTestVMInfo();
				networkMsg = NetworkMsg.MSG_StopVM(vm4);
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
}
