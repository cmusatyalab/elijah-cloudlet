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
				Log.d("krha", "sending image");

				//time stamp
				dataSendStart = System.currentTimeMillis();
				
				// upload image
				if(networkWriter != null){
					networkWriter.writeInt(totalSize);
					networkWriter.write(mData);
					networkWriter.flush(); // flush for accurate time measure					
				}else{
					try {
						Thread.sleep(1000);
					} catch (InterruptedException e) {
						// TODO Auto-generated catch block
						e.printStackTrace();
					}
				}

				//time stamp
				dataReceiveStart = dataSendEnd = System.currentTimeMillis();
				Log.d("krha_app", "[DATA_SEND]\t" + dataSendEnd + " - " + dataSendStart + " = " + (dataSendEnd-dataSendStart));
				
				int numberOfPeople = -1;
				if(networkReader != null){
					// receive results
					numberOfPeople = networkReader.readInt();					
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
				Log.d("krha_app", "[DATA_RECEIVE]\t" + dataReceiveEnd + " - " + dataReceiveStart + " = " + (dataReceiveEnd-dataReceiveStart));
				
				// callback
				Message msg = Message.obtain();
				msg.what = NetworkClient.FEEDBACK_RECEIVED;
				Bundle data = new Bundle();
				data.putInt("number_of_people", numberOfPeople);
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
}
