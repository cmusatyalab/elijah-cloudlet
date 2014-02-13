package edu.cmu.cs.cloudlet.android.application.graphics;

import java.io.BufferedInputStream;
import java.io.BufferedReader;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.IOException;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.TreeMap;
import java.util.Vector;

import android.content.Context;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.util.Log;

public class GNetworkClientSender extends Thread {
	private boolean isThreadRun = true;
	private Context mContext;
	private Handler mHandler;

	private String Server_ipAddress;
	private int Server_port;
	
	private GNetworkClientReceiver receiver;
	private Vector<GNetworkMessage> commandQueue = new Vector<GNetworkMessage>();		//thread safe
	private Socket mClientSocket;
	private DataOutputStream networkWriter;
	
	private GNetworkClient connector;
	private int messageCounter = 0;
	private int accIndex = 0;

	public GNetworkClientSender(Context context, Handler handler) {
		mContext = context;
		mHandler = handler;
	}
	
	public void setConnector(GNetworkClient connector){
		this.connector = connector;
	}

	public void setConnection(String ip, int port){
		this.Server_ipAddress = ip;
		this.Server_port = port;
	}
	

	private boolean initConnection(String server_ipAddress2, int server_port2) {
		try {
			mClientSocket = new Socket();
			mClientSocket.connect(new InetSocketAddress(this.Server_ipAddress, this.Server_port), 10*1000);
			networkWriter = new DataOutputStream(mClientSocket.getOutputStream());
			receiver = new GNetworkClientReceiver(new DataInputStream(mClientSocket.getInputStream()), mHandler);
		} catch (UnknownHostException e) {
			Log.e("krha", e.toString());
			Message msg = Message.obtain();
			msg.what = GNetworkClient.CONNECTION_ERROR;
			Bundle data = new Bundle();
			data.putString("message", "Cannot connect to " + this.Server_ipAddress + ":" + this.Server_port);
			msg.setData(data);
			mHandler.sendMessage(msg);
			return false;
		} catch (IOException e) {
			Log.e("krha", e.toString());
			Message msg = Message.obtain();
			msg.what = GNetworkClient.CONNECTION_ERROR;
			Bundle data = new Bundle();
			data.putString("message", "Error in connecting to " + this.Server_ipAddress + ":" + this.Server_port);
			msg.setData(data);
			mHandler.sendMessage(msg);
			return false;
		}
		
		return true;
	}
	
	public void run() {
		boolean ret = this.initConnection(this.Server_ipAddress, this.Server_port);
		if(ret == false)
			return;
		
		// Socket Receiver Thread Start 
		this.receiver.start();
		
		long lastSentTime = 0;
		while(isThreadRun == true){
			if(commandQueue.size() == 0){
				try {
					Thread.sleep(1);
				} catch (InterruptedException e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
				}
			}else{
				// Request to Server
				GNetworkMessage msg = commandQueue.remove(0);
				try {
					float[] accData = msg.getAccData();
					networkWriter.writeInt(this.accIndex);
					networkWriter.writeInt(this.receiver.getLastFrameID());
					networkWriter.writeFloat(accData[0]);
					networkWriter.writeFloat(accData[1]);
					networkWriter.flush();
					this.receiver.recordSentTime(this.accIndex, System.currentTimeMillis());
					this.accIndex++;
				} catch (IOException e) {
					e.printStackTrace();
					break;
				}
				lastSentTime = System.currentTimeMillis();
				
			}
		}
//		this.notifyFinishInfo();
	}
	
	private void notifyFinishInfo() {
		TreeMap<Integer, Long> timeStamps = this.receiver.getReceiverStamps();
		int numberOfMissedAcc = 0;
		for(int i = 0; i < this.accIndex; i++){
			if(timeStamps.get(i) == null){
				numberOfMissedAcc++;
			}
		}
		
		String message = "Number of missed acc ID: " + numberOfMissedAcc +
				"\nNumber of duplicated Acc ID: " + this.receiver.getDuplicatedAcc();
		Message msg = Message.obtain();
		msg.what = GNetworkClient.FINISH_MESSAGE;
		msg.arg1 = messageCounter++;
		msg.obj = "message";
		Bundle data = new Bundle();
		data.putString("message", message);
		msg.setData(data);
		mHandler.sendMessage(msg);
	}
	
	public void requestCommand(GNetworkMessage msg){
		this.commandQueue.add(msg);
	}

	public void close() {
		try {
			this.isThreadRun = false;
			
			if(this.receiver != null){
				this.receiver.close();
				this.receiver = null;
			}
			if(this.networkWriter != null){
				this.networkWriter.close();
				this.networkWriter = null;
			}
			if(this.mClientSocket != null){
				this.mClientSocket.close();
				this.mClientSocket = null;
			}
		} catch (IOException e) {
			Log.e("krha", e.toString());
		}
		Log.d("krha", "Socket Connection Closed");
	}
	
}
