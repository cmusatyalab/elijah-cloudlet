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

import edu.cmu.cs.cloudlet.android.data.VMInfo;
import edu.cmu.cs.cloudlet.android.util.KLog;

import android.content.Context;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;

public class NetworkClientSender extends Thread {
	private boolean isThreadRun = true;
	private static boolean IS_MOCK = true;
	private Context mContext;
	private Handler mHandler;

	private String Server_ipAddress;
	private int Server_port;
	
	private NetworkClientReceiver receiver;
	private Vector<NetworkMsg> commandQueue = new Vector<NetworkMsg>();		//thread safe
	private Socket mClientSocket;
	private DataOutputStream networkWriter;
	
	private byte[] imageSendingBuffer = new byte[1024*1024];
	

	public NetworkClientSender(Context context, Handler handler) {
		mContext = context;
		mHandler = handler;
	}

	public void setConnection(String ip, int port){
		this.Server_ipAddress = ip;
		this.Server_port = port; 

		/*
		// for mock test, read from file
		FileOutputStream mockOutputFile = new FileOutputStream(CloudletEnv.instance().getFilePath(CloudletEnv.SOCKET_MOCK_OUTPUT));
		networkWriter = new DataOutputStream(mockOutputFile);
		receiver = new NetworkClientReceiver(new DataInputStream(mClientSocket.getInputStream()), mHandler);
		Thread receiverThread = new Thread(receiver);
		receiverThread.start();
		 */
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
			
			NetworkMsg command = commandQueue.remove(0);
			this.sendCommand(command);
			if(command.commandNumber == NetworkMsg.COMMAND_REQ_TRANSFER_START){
				// send data
				VMInfo overlayVM = null;
				ArrayList<VMInfo> vmList = command.getVMList();
				for(int i = 0; i < vmList.size(); i++){
					if(vmList.get(i).getInfo(VMInfo.JSON_KEY_TYPE).equalsIgnoreCase("overlay") == true){
						overlayVM = vmList.get(i);
					} 
				}
				if(overlayVM != null){
					File image = new File(overlayVM.getInfo(VMInfo.JSON_KEY_DISKIMAGE_PATH));
					File mem = new File(overlayVM.getInfo(VMInfo.JSON_KEY_MEMORYSNAPSHOT_PATH));
					this.sendOverlayImage(image, mem);
				}
			}
		}
	}
	
	private void sendCommand(NetworkMsg msg) {
		try {
			byte[] byteMsg = msg.toNetworkByte();
			networkWriter.write(byteMsg);
			networkWriter.flush(); // flush for accurate time measure
			KLog.println("Send Message " + msg);			
		} catch (IOException e) {
			e.printStackTrace();
		}
	}


	private void sendOverlayImage(File image, File mem) {
		try {			
			int sendByte = -1;
			//sending disk image
			BufferedInputStream bi = new BufferedInputStream(new FileInputStream(image));
			while((sendByte = bi.read(imageSendingBuffer, 0, imageSendingBuffer.length)) > 0){
				networkWriter.write(imageSendingBuffer, 0, sendByte);
				KLog.println("Sending " + image.getName() + ".. " + sendByte);
			}
			bi.close();

			//sending memory snapshot
			bi = new BufferedInputStream(new FileInputStream(mem));
			while((sendByte = bi.read(imageSendingBuffer, 0, imageSendingBuffer.length)) > 0){
				networkWriter.write(imageSendingBuffer, 0, sendByte);
				KLog.println("Sending " + image.getName() + ".. " + sendByte);
			}
			bi.close();

			networkWriter.flush();
		} catch (FileNotFoundException e) {
			KLog.printErr(e.toString());
		} catch (IOException e) {
			KLog.printErr(e.toString());
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
