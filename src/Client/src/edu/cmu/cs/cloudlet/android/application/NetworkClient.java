package edu.cmu.cs.cloudlet.android.application;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStreamWriter;
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
	public static final int FEEDBACK_RECEIVED = 1;

	private Socket mClientSocket = null;
	private DataInputStream networkReader = null;
	private DataOutputStream networkWriter = null;

	private byte[] mData;
	private Context mContext;
	private Handler mHandler;
	private Looper mLoop;

	//time stamp
	protected long dataSendStart;
	protected long dataSendEnd;
	protected long dataReceiveStart;
	protected long dataReceiveEnd;
	protected String sendingLog = "N/A";
	protected String receivingLog = "N/A";
	
	public NetworkClient(Context context, Handler handler) {
		mContext = context;
		mHandler = handler;
	}

	public void initConnection(String ip, int port) throws UnknownHostException, IOException {
		mClientSocket = new Socket();
		mClientSocket.connect(new InetSocketAddress(ip, port), 10*1000);
		networkWriter = new DataOutputStream(mClientSocket.getOutputStream());
		networkReader = new DataInputStream(mClientSocket.getInputStream());
	}

	public void run() {

		while(true){
			if(mData == null){
				try {
					Thread.sleep(100);
				} catch (InterruptedException e) {
					e.printStackTrace();
				}
				continue;
			}
			
			try {
				int totalSize = mData.length;

				//time stamp
				dataSendStart = System.currentTimeMillis();
				
				// upload image
				if(networkWriter != null){
					networkWriter.writeInt(totalSize);
					networkWriter.write(mData);
					networkWriter.flush(); // flush for accurate time measure					
				}else{
					try {
						Thread.sleep(100);
					} catch (InterruptedException e) {
						// TODO Auto-generated catch block
						e.printStackTrace();
					}
				}

				//time stamp
				dataReceiveStart = dataSendEnd = System.currentTimeMillis();
				this.sendingLog  = "[Image Sending]\t" + (dataSendEnd-dataSendStart) + " (ms)";
				Log.d("krha_app", this.sendingLog);
				
				String ret_string = "None";
				if(networkReader != null){
					// receive results
					int ret_size = networkReader.readInt();
					Log.d("krha", "ret data size : " + ret_size);
					byte[] ret_byte = new byte[ret_size];
					networkReader.read(ret_byte);
					ret_string = new String(ret_byte, "UTF-8");					
				}else{
					try {
						Thread.sleep(1000);
					} catch (InterruptedException e) {
						// TODO Auto-generated catch block
						e.printStackTrace();
					}
				}

				// time stamp
				dataReceiveEnd = System.currentTimeMillis();
				this.receivingLog = "[Result Waiting]\t" + (dataReceiveEnd-dataReceiveStart) + " (ms)"; 
				Log.d("krha_app", this.receivingLog);
				
				// callback
				Message msg = Message.obtain();
				msg.what = NetworkClient.FEEDBACK_RECEIVED;
				Bundle data = new Bundle();
				data.putString("objects", ret_string);
				msg.setData(data);
				mHandler.sendMessage(msg);
				
				//delete current data
				mData = null;
			} catch (IOException e) {
			}
		}
	}

	public void uploadImage(byte[] data) {
		mData = data;
	}
	
	public String getTimeLog(){
		return this.sendingLog + "\n" + this.receivingLog;
	}

	public void close() {
		// TODO Auto-generated method stub
		try {
			if(mClientSocket != null)
				mClientSocket.close();
			if(networkReader != null)
				networkReader.close();
			if(networkWriter != null)
				networkWriter.close();
		} catch (IOException e) {
			e.printStackTrace();
		}
	}
}
