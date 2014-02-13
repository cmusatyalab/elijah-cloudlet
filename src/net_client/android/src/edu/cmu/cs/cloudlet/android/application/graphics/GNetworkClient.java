package edu.cmu.cs.cloudlet.android.application.graphics;

import java.io.File;
import java.util.ArrayList;
import java.util.Vector;

import org.apache.http.util.ByteArrayBuffer;
import org.json.JSONException;
import org.json.JSONObject;

import edu.cmu.cs.cloudlet.android.R;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.util.TypedValue;
import android.view.View;
import android.widget.TextView;

public class GNetworkClient {
	public static final int CONNECTION_ERROR = 1;
	public static final int NETWORK_ERROR = 2;
	public static final int PROGRESS_MESSAGE = 3;
	public static final int FINISH_MESSAGE = 4;
	public static final int PROGRESS_MESSAGE_TRANFER = 5;
	public static final int DATA_SENT_MESSAGE = 10;
	public static final int DATA_RECV_MESSAGE = 11;

	protected GraphicsClientActivity activity;
	protected Context mContext;
	protected GNetworkClientSender networkClient;

//	private ArrayList<String> mAcclist;	
	protected long prevDuration = Long.MAX_VALUE;
	private int messageUpdateRateCount = 0;
	

	public GNetworkClient(GraphicsClientActivity activity, Context context) {
		this.activity = activity;
		this.mContext = context;
	}

	public void startConnection(String ipAddress, int port) {
//		this.activity.updateLog("Connecting to " + ipAddress);
		
		if (networkClient != null) {
			networkClient.close();
			networkClient.stop();
		}
		networkClient = new GNetworkClientSender(this.mContext, eventHandler);
		networkClient.setConnector(this);
		networkClient.setConnection(ipAddress, port);
		networkClient.start();
		
	}
	
	public void updateAccValue(float[] values){
		GNetworkMessage networkMsg = new GNetworkMessage(GNetworkMessage.COMMAND_REQ_TRANSFER_START, values);
		if(networkClient != null && networkClient.isAlive()){
			networkClient.requestCommand(networkMsg);
		}
		
	}

	/*
	public void updateAccList(ArrayList<String> testAccList) {
		this.mAcclist = testAccList;		
	}
	*/
	
	public void updateMessage(String dialogMessage) {
		this.activity.updateLog(dialogMessage);
	}

	public void close() {
		if (this.networkClient != null){
			this.networkClient.close();
//			this.networkClient.stop();
		}
	}

	/*
	 * Server Message Handler
	 */
	Handler eventHandler = new Handler() {
		public void handleMessage(Message msg) {
			if (msg.what == GNetworkClient.CONNECTION_ERROR) {
				String message = msg.getData().getString("message");
				showAlertDialog(message);
			} else if (msg.what == GNetworkClient.NETWORK_ERROR) {
				if (networkClient != null) {
					networkClient.close();
				}
				String message = msg.getData().getString("message");
				showAlertDialog(message);
			} else if (msg.what == GNetworkClient.PROGRESS_MESSAGE) {
				Bundle data = msg.getData();
				activity.updateData(msg.obj);				
				if(messageUpdateRateCount++ % 10 ==0){
					activity.updateLog(data.getString("message"));
				}	
			} else if (msg.what == GNetworkClient.FINISH_MESSAGE){
				Bundle data = msg.getData();
//				activity.updateLog(data.getString("message") + "\n");
				if (networkClient != null) {
					networkClient.close();
				}
				showAlertDialog("Finish");
			}
		}
	};

	private void showAlertDialog(String errorMsg) {
		if (activity.isFinishing() == false) {
			new AlertDialog.Builder(mContext).setTitle("Info")
					.setIcon(R.drawable.ic_launcher).setMessage(errorMsg)
					.setNegativeButton("Confirm", null).show();
		}
	}
}
