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
import java.util.Vector;

import edu.cmu.cs.cloudlet.android.data.Measure;
import edu.cmu.cs.cloudlet.android.data.VMInfo;
import edu.cmu.cs.cloudlet.android.util.KLog;

import android.content.Context;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;

public class NetworkClientSender extends Thread {
	private boolean isThreadRun = true;
	private Context mContext;
	private Handler mHandler;

	private String Server_ipAddress;
	private int Server_port;
	
	private NetworkClientReceiver receiver;
	private Vector<NetworkMsg> commandQueue = new Vector<NetworkMsg>();		//thread safe
	private Socket mClientSocket;
	private DataOutputStream networkWriter;
	
	private byte[] imageSendingBuffer = new byte[3*1024*1024];
	private CloudletConnector connector;

	public NetworkClientSender(Context context, Handler handler) {
		mContext = context;
		mHandler = handler;
	}
	
	// this is only for sending status of VM transfer.
	// I know this is bad approach, but make my life easier.
	// Do not use this method for other usage.
	public void setConnector(CloudletConnector connector){
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
			receiver = new NetworkClientReceiver(new DataInputStream(mClientSocket.getInputStream()), mHandler);
		} catch (UnknownHostException e) {
			Message msg = Message.obtain();
			msg.what = CloudletConnector.CONNECTION_ERROR;
			Bundle data = new Bundle();
			data.putString("message", "Cannot Connect to " + this.Server_ipAddress + ":" + this.Server_port);
			msg.setData(data);
			mHandler.sendMessage(msg);
			return false;
		} catch (IOException e) {
			Message msg = Message.obtain();
			msg.what = CloudletConnector.CONNECTION_ERROR;
			Bundle data = new Bundle();
			data.putString("message", "Cannot Connect to " + this.Server_ipAddress + ":" + this.Server_port);
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
		
		while(isThreadRun == true){
			if(commandQueue.size() == 0){
				try {
					Thread.sleep(100);
				} catch (InterruptedException e) {
					e.printStackTrace();
				}
				continue;
			}
			
			NetworkMsg networkCommand = commandQueue.remove(0);
			this.sendCommand(networkCommand);
			
			switch(networkCommand.getCommandType()){
			case NetworkMsg.COMMAND_REQ_TRANSFER_START:
				// Send overlay binary
				VMInfo overlayVMInfo = networkCommand.getOverlayInfo();
				if(overlayVMInfo != null){
					File image = new File(overlayVMInfo.getInfo(VMInfo.JSON_KEY_DISKIMAGE_PATH));
					File mem = new File(overlayVMInfo.getInfo(VMInfo.JSON_KEY_MEMORYSNAPSHOT_PATH));
					Measure.setOverlaySize(image.length(), mem.length());
					Measure.put(Measure.OVERLAY_TRANSFER_START);
					this.sendOverlayImage(image, mem);
					Measure.put(Measure.OVERLAY_TRANSFER_END);
				}
				
				Measure.put(Measure.NET_REQ_OVERLAY_TRASFER);
				break;				
			}
		}
	}
	
	private void sendCommand(NetworkMsg msg) {
		try {
			byte[] byteMsg = msg.toNetworkByte();
			networkWriter.write(byteMsg);
			networkWriter.flush(); 		// flush everytime for accurate time measure
//			KLog.println("Send Message " + msg);
		} catch (IOException e) {
			e.printStackTrace();
		}
	}


	private void sendOverlayImage(File image, File mem) {
		try {			
			int sendByte = -1, totalByte = 0;
			//sending disk image
			BufferedInputStream bi = new BufferedInputStream(new FileInputStream(image));
			while((sendByte = bi.read(imageSendingBuffer, 0, imageSendingBuffer.length)) > 0){
				networkWriter.write(imageSendingBuffer, 0, sendByte);
				totalByte += sendByte;
				String statusMsg = "Sending Disk.. " + (int)(100.0*totalByte/image.length()) + "%, (" + totalByte + "/" + image.length() + ")";
//				KLog.println(statusMsg);
				this.notifyTransferStatus("Step 2. Sending overlay VM ..\n" + statusMsg);
			}
			bi.close();

			//sending memory snapshot
			totalByte = 0;
			bi = new BufferedInputStream(new FileInputStream(mem));
			while((sendByte = bi.read(imageSendingBuffer, 0, imageSendingBuffer.length)) > 0){
				networkWriter.write(imageSendingBuffer, 0, sendByte);
				totalByte += sendByte;
				String statusMsg = "Sending Memory.. " + (int)(100.0*totalByte/mem.length()) + "%, (" + totalByte + "/" + mem.length() + ")";
//				KLog.println(statusMsg);
				this.notifyTransferStatus("Step 2. Sending overlay VM ..\n" + statusMsg);
			}
			bi.close();

			networkWriter.flush();
		} catch (FileNotFoundException e) {
			KLog.printErr(e.toString());
		} catch (IOException e) {
			KLog.printErr(e.toString());
		}
		
	}

	private void notifyTransferStatus(final String messageString) {
		if(this.connector != null){
			mHandler.post(new Runnable(){
				@Override
				public void run() {
					connector.updateMessage(messageString);
					
				}				
			});
		}
	}
	
	public void requestCommand(NetworkMsg command){
		this.commandQueue.add(command);
	}

	public void close() {
		try {
			this.isThreadRun = false;
			
			if(this.receiver != null)
				this.receiver.close();
			if(this.networkWriter != null)
				this.networkWriter.close();
			if(this.mClientSocket != null)
				this.mClientSocket.close();
		} catch (IOException e) {
			KLog.printErr(e.toString());
		}
		KLog.println("Socket Connection Closed");
	}

}