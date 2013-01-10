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
	public static final int FEEDBACK= 1;

	private Socket mClientSocket = null;
	private DataInputStream networkReader = null;
	private DataOutputStream networkWriter = null;

	private CloudletCameraActivity mActivity;
	private Context mContext;
	private Handler mHandler;
	private Looper mLoop;

	//time stamp
	protected long dataSendStart;
	protected long dataSendEnd;
	protected long dataReceiveStart;
	protected long dataReceiveEnd;
	private ArrayList<File> mFileList = new ArrayList<File>();
	private byte[] mCapturedImage = null;
	
	public NetworkClient(CloudletCameraActivity activity, Context context, Handler handler) {
		mActivity = activity;
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
			if(mFileList.size() == 0 && mCapturedImage == null){
				try {
					Thread.sleep(100);
				} catch (InterruptedException e) {
					e.printStackTrace();
				}
				continue;
			}

			// automated test
			long processStartTime = System.currentTimeMillis();		
			while(mFileList.size() > 0){
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
		}
	}


	private void requestServer(byte[] testImageData, String imageName) {
		
		try {
			int totalSize = testImageData.length;

			//time stamp
			dataSendStart = System.currentTimeMillis();					
			// upload image
			networkWriter.writeInt(totalSize);
			networkWriter.write(testImageData);
			networkWriter.flush(); // flush for accurate time measure
			
			int ret_size = networkReader.readInt();
			Log.d("krha", "ret data size : " + ret_size);
			byte[] ret_byte = new byte[ret_size];
			networkReader.read(ret_byte);
			String ret = new String(ret_byte, "UTF-8");

			// time stamp
			dataReceiveEnd = System.currentTimeMillis();
			if(ret.trim().length() == 0){
				ret = "Nothing";
			}
//			String message = imageName + "\t" + dataSendStart + "\t" + dataReceiveEnd + "\t" + (dataReceiveEnd-dataSendStart) + "\t" + ret;
			String message = ret;
			Log.d("krha_app", message);

			// callback
			Message msg = Message.obtain();
			msg.what = NetworkClient.FEEDBACK;
			Bundle data = new Bundle();
			data.putString("message", message);
			msg.setData(data);
			mHandler.sendMessage(msg);			
			
		} catch (IOException e) {
		}
		
	}

	public void uploadImageList(ArrayList<File> imageList) {
		mFileList = imageList;
	}

	public void uploadImage(byte[] imageData) {
		mCapturedImage = imageData;
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
