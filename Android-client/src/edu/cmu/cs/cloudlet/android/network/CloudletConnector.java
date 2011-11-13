package edu.cmu.cs.cloudlet.android.network;

import java.io.IOException;
import java.net.UnknownHostException;

import edu.cmu.cs.cloudlet.android.CloudletActivity;

import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.os.Handler;
import android.os.Message;

public class CloudletConnector {
	protected Context mContext;
	NetworkClientSender networkClient;

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
		
		// Send REQ VM List
		NetworkMsg msg = NetworkMsg.makeOverlayList();
		
		
	}

}
