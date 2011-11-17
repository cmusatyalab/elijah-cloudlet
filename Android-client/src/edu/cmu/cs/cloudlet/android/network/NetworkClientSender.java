package edu.cmu.cs.cloudlet.android.network;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.net.Socket;
import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.Vector;

import edu.cmu.cs.cloudlet.android.util.CloudletEnv;
import edu.cmu.cs.cloudlet.android.util.KLog;

import android.content.Context;
import android.os.Handler;

public class NetworkClientSender extends Thread {
	private boolean isThreadRun = true;
	private static boolean IS_MOCK = true;
	private Context mContext;
	private Handler mHandler;

	private NetworkClientReceiver receiver;
	private Vector<NetworkMsg> commandQueue = new Vector<NetworkMsg>();		//thread safe
	private Socket mClientSocket;
	private DataOutputStream networkWriter;
	

	public NetworkClientSender(Context context, Handler handler) {
		mContext = context;
		mHandler = handler;
	}

	public void initConnection(String ip, int port) throws UnknownHostException, IOException {
		mClientSocket = new Socket(ip, port);
		networkWriter = new DataOutputStream(mClientSocket.getOutputStream());
		receiver = new NetworkClientReceiver(new DataInputStream(mClientSocket.getInputStream()), mHandler);
			

		/*
		// for mock test, read from file
		FileOutputStream mockOutputFile = new FileOutputStream(CloudletEnv.instance().getFilePath(CloudletEnv.SOCKET_MOCK_OUTPUT));
		networkWriter = new DataOutputStream(mockOutputFile);
		receiver = new NetworkClientReceiver(new DataInputStream(mClientSocket.getInputStream()), mHandler);
		Thread receiverThread = new Thread(receiver);
		receiverThread.start();
		 */
	}
		
	public void run() {
		if(mClientSocket == null || networkWriter == null){
			return;
		}
		// Socket Receiver Thread Start 
		Thread receiverThread = new Thread(receiver);
		receiverThread.start();
		
		while(isThreadRun == true){
			if(commandQueue.size() == 0){
				try {
					Thread.sleep(100);
				} catch (InterruptedException e) {
					e.printStackTrace();
				}
				continue;
			}
			
			this.sendCommand(commandQueue.remove(0));
		}
	}

	private void sendCommand(NetworkMsg msg) {
		// upload image
		try {
			byte[] byteMsg = msg.toNetworkByte();
			networkWriter.write(byteMsg);
			networkWriter.flush(); // flush for accurate time measure
			KLog.println("Send Message " + msg);
		} catch (IOException e) {
			e.printStackTrace();
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
