package edu.cmu.cs.cloudlet.android.application.esvm;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.net.UnknownHostException;
import java.util.ArrayList;


import android.app.ProgressDialog;
import android.content.Context;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.util.Log;

public class NetworkClient extends Thread {
	public static final String TAG = "krha_app";
	public static final int CALLBACK_FAILED = 0;
	public static final int CALLBACK_SUCCESS = 1;
	public static final int CALLBACK_UPDATE = 2;

	private Socket mClientSocket = null;
	private DataInputStream networkReader = null;
	private DataOutputStream networkWriter = null;

	private Context context;
	private Handler networkCallbackHandler;
	private Looper mLoop;

	//time stamp
	protected long dataSendStart;
	protected long dataSendEnd;
	protected long dataReceiveStart;
	protected long dataReceiveEnd;
	private ArrayList<File> mFileList = new ArrayList<File>();
	private byte[] mCapturedImage = null;
	private String serverIP = null;
	private int serverPort = -1;
	
	public NetworkClient(Context context, Handler handler, String ip, int port) {
		this.context = context;
		this.networkCallbackHandler = handler;
		this.serverIP = ip;
		this.serverPort = port;
	}

	public void initConnection(String ip, int port) throws UnknownHostException, IOException {
		mClientSocket = new Socket();
		mClientSocket.connect(new InetSocketAddress(ip, port), 10*1000);
		networkWriter = new DataOutputStream(mClientSocket.getOutputStream());
		networkReader = new DataInputStream(mClientSocket.getInputStream());
	}

	public void run() {
		
		// init connection
		try {
			this.initConnection(this.serverIP, this.serverPort);
		} catch (UnknownHostException e) {
			this.handleFailure("Cannot connect to " + this.serverIP + ":" + this.serverPort + "\nReason : " + e.getLocalizedMessage());
			this.close();
			return;
		} catch (IOException e) {
			this.handleFailure("Cannot connect to " + this.serverIP+ ":" + this.serverPort + "\nReason : " + e.getLocalizedMessage());
			this.close();
			return;
		}

		// processing request
		while(true){
			if(mFileList.size() == 0 && mCapturedImage == null){
				try {
					Thread.sleep(10);
				} catch (InterruptedException e) {
					e.printStackTrace();
				}
				continue;
			}

			//  test
			long processStartTime = System.currentTimeMillis();		
			if(mFileList.size() > 0){
				File testFile = mFileList.remove(0);
				byte[] testImageData = new byte[(int) testFile.length()];
				try {
					FileInputStream fs = new FileInputStream(testFile);
					fs.read(testImageData , 0, testImageData.length);
				} catch (FileNotFoundException e) {
					e.printStackTrace();
				} catch (IOException e) {
					e.printStackTrace();
				}				
				requestServer(testImageData, testFile.getName());
			}
			
			// handle single image
			if (mCapturedImage != null){
				requestServer(mCapturedImage, "camera");
				mCapturedImage = null;
			}
			long processEndTime = System.currentTimeMillis();
			Log.d("krha", "response time : " + (processEndTime-processStartTime));
		}
	}

	private void requestServer(byte[] testImageData, String imageName) {
		
		try {
			int totalSize = testImageData.length;		
			// upload image
			networkWriter.writeInt(totalSize);
			networkWriter.write(testImageData);
			networkWriter.flush(); // flush for accurate time measure
			
			// recv 
			int ret_size = networkReader.readInt();
			Log.d("krha", "ret data size : " + ret_size);
			byte[] ret_byte = new byte[ret_size];
			networkReader.read(ret_byte);
			String ret = new String(ret_byte, "UTF-8");
			if(ret.trim().length() == 0){
				ret = "Nothing:0";
			}
			String message = ret;
			Log.d("krha_app", message);

			// callback
			this.handleSuccess(message);
		} catch (IOException e) {
		}
		
	}

	public void uploadImageList(ArrayList<File> imageList) {
		mFileList = imageList;
	}

	public void uploadImage(byte[] imageData) {
		mCapturedImage = imageData;
	}


	private void handleNotification(final String messageString) {
		Message msg = Message.obtain();
		msg.what = NetworkClient.CALLBACK_UPDATE;
		msg.obj = messageString;
		this.networkCallbackHandler.sendMessage(msg);
	}
	
	private void handleSuccess(String retMsg) {
		Message msg = Message.obtain();
		msg.what = NetworkClient.CALLBACK_SUCCESS;
		msg.obj = retMsg;
		this.networkCallbackHandler.sendMessage(msg);
	}

	private void handleFailure(String reason) {
		Message msg = Message.obtain();
		msg.what = NetworkClient.CALLBACK_FAILED;
		msg.obj = reason;
		this.networkCallbackHandler.sendMessage(msg);
	}
	
	public void close() {
		try {
			if(networkReader != null)
				networkReader.close();
			if(networkWriter != null)
				networkWriter.close();
			if(mClientSocket != null)
				mClientSocket.close();
		} catch (IOException e) {
			e.printStackTrace();
		}
	}
}
