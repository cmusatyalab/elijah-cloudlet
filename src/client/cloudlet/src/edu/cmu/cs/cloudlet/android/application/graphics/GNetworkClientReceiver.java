//
// Elijah: Cloudlet Infrastructure for Mobile Computing
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
package edu.cmu.cs.cloudlet.android.application.graphics;

import java.io.DataInputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.ArrayList;
import java.util.SortedMap;
import java.util.SortedSet;
import java.util.TreeMap;

import org.apache.http.util.ByteArrayBuffer;
import org.json.JSONException;

import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.util.Log;

public class GNetworkClientReceiver extends Thread {
	private Handler mHandler;
	private DataInputStream networkReader;
	private boolean isThreadRun = true;

	private int messageCounter = 0;
	protected int maxByteSize = 1024*1024*1;	// 1MB
	protected byte[] recvByte = new byte[maxByteSize];
	private int frameID = 0;
	private int clientID = 0;
	TreeMap<Integer, Long> receiver_stamps = new TreeMap<Integer, Long>(); 
	ArrayList<Long> reciver_time_list = new ArrayList<Long>();
	private int duplicated_client_id;
	
	public GNetworkClientReceiver(DataInputStream dataInputStream, Handler mHandler) {
		this.networkReader = dataInputStream;
		this.mHandler = mHandler;
	}
	
	public TreeMap<Integer, Long> getReceiverStamps(){
		return this.receiver_stamps;
	}
	
	public ArrayList<Long> getReceivedTimeList(){
		return this.reciver_time_list;
	}
	
	public int getDuplicatedAcc(){
		return this.duplicated_client_id;
	}

	@Override
	public void run() {
		while(isThreadRun == true){
			int recvSize = 0;
			
			try {
				recvSize = this.receiveMsg(networkReader);
				this.notifyStatus(GNetworkClient.PROGRESS_MESSAGE, "Received (accID: " + this.clientID + ", frameID:" + this.frameID + ")" , null);
			} catch (IOException e) {
				Log.e("krha", e.toString());
//				this.notifyStatus(GNetworkClient.NETWORK_ERROR, e.toString(), null);
				break;
			}
		}
	}

	private int receiveMsg(DataInputStream reader) throws IOException {
		this.clientID = reader.readInt();
		this.frameID = reader.readInt();
		int retLength = reader.readInt();
		
		int readSize = 0;
		while(readSize < retLength){
			int ret = reader.read(this.recvByte, 0, retLength-readSize);
			if(ret <= 0){
				break;
			}
			readSize += ret;
		}
		long currentTime = System.currentTimeMillis();
		Log.d("krha", "Received");
		if(this.receiver_stamps.get(this.clientID) == null){
			this.receiver_stamps.put(this.clientID, currentTime);
			Log.d("krha", "Save Client ID : " + this.clientID);			
		}else{
			duplicated_client_id++;
		}
		this.reciver_time_list.add(currentTime);		
		return readSize;
	}
	
	private void notifyStatus(int command, String string, byte[] recvMsg) {
		Message msg = Message.obtain();
		msg.what = command;
		msg.obj = recvMsg;
		Bundle data = new Bundle();
		data.putString("message", string);
		msg.setData(data);
		this.mHandler.sendMessage(msg);
	}
	
	public void close() {
		this.isThreadRun = false;		
		try {
			if(this.networkReader != null)
				this.networkReader.close();
		} catch (IOException e) {
			Log.e("krha", e.toString());
		}
	}

	public int getLastFrameID() {
		return this.frameID;
	}
}
