package edu.cmu.cs.cloudlet.android.application.graphics;

import java.io.DataInputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.SortedMap;
import java.util.SortedSet;
import java.util.TreeMap;

import org.apache.http.util.ByteArrayBuffer;
import org.json.JSONException;
import org.teleal.common.util.ByteArray;

import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.util.Log;

public class GNetworkClientReceiver extends Thread {
	private Handler mHandler;
	private DataInputStream networkReader;
	private boolean isThreadRun = true;

	private int messageCounter = 0;
	protected byte[] recvByte = null;
	ArrayList<Particle> particleList = new ArrayList<Particle>();
	private int startFrameID = 0;
	private int currentFrameID = 0;
	private int clientID = 0;
	private TreeMap<Integer, Long> receiver_stamps = new TreeMap<Integer, Long>(); 
	private ArrayList<Long> reciver_time_list = new ArrayList<Long>();
	private int duplicated_client_id;
	
	private HashMap<Integer, Long> latencyRecords = new HashMap<Integer, Long>();
	private long totalLatency = 0;
	
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
		while(isThreadRun == false){
			try {
				Thread.sleep(1);
			} catch (InterruptedException e) {
			}
		}
		
		// Recv initial simulation information
		try {
			int containerWidth = networkReader.readInt();
			int containerHeight = networkReader.readInt();
			Log.d("krha", "container size : " + containerWidth + ", " + containerHeight);
			VisualizationStaticInfo.containerWidth = containerWidth;
			VisualizationStaticInfo.containerHeight = containerHeight;
			
		} catch (IOException e1) {
			e1.printStackTrace();
		}
		
		long startTime = System.currentTimeMillis();
		while(isThreadRun == true){
			int recvSize = 0;
			
			try {
				recvSize = this.receiveMsg(networkReader);
				long currentTime = System.currentTimeMillis();
				long duration = currentTime - startTime;
				long latency = currentTime - this.getSentTime(this.clientID);
				if(latency > 0)
					totalLatency += latency; 
				int totalFrameNumber = this.getLastFrameID()-this.startFrameID;
				if(totalFrameNumber > 0 && latency > 0){
					String message = "FPS: " + this.roundDigit(1000.0*totalFrameNumber/duration) + 
							", ACC: " + this.roundDigit(1000.0*this.clientID/duration) + 
							", Latency: "  + this.roundDigit(1.0*totalLatency/totalFrameNumber) + 
							" / " + latency; 
					this.notifyStatus(GNetworkClient.PROGRESS_MESSAGE, message, recvByte);
				}
			} catch (IOException e) {
				Log.e("krha", e.toString());
//				this.notifyStatus(GNetworkClient.NETWORK_ERROR, e.toString(), null);
				break;
			}
		}
	}

	private int receiveMsg(DataInputStream reader) throws IOException {
		this.clientID = reader.readInt();
		this.currentFrameID = reader.readInt();
		int retLength = reader.readInt();
		if(this.startFrameID == 0)
			this.startFrameID = this.currentFrameID;
		
		if(recvByte == null || recvByte.length < retLength){
			recvByte = new byte[retLength];
		}
		
		int readSize = 0;
		while(readSize < retLength){
			int ret = reader.read(this.recvByte, readSize, retLength-readSize);
			if(ret <= 0){
				break;
			}
			readSize += ret;
		}
		
		long currentTime = System.currentTimeMillis();
		if(this.receiver_stamps.get(this.clientID) == null){
			this.receiver_stamps.put(this.clientID, currentTime);
//			Log.d("krha", "Save Client ID : " + this.clientID);			
		}else{
			duplicated_client_id++;
		}
		this.reciver_time_list.add(currentTime);		
		return readSize;
	}
	
	private void notifyStatus(int command, String string, byte[] recvData) {
		// Copy data with endian switching
        ByteBuffer buf = ByteBuffer.allocate(recvData.length);
        buf.order(ByteOrder.LITTLE_ENDIAN);
        buf.put(recvData);
		buf.flip();
		buf.compact();
		
		Message msg = Message.obtain();
		msg.what = command;
		msg.obj = buf;
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
		return this.currentFrameID;
	}

	public void recordSentTime(int accIndex, long currentTimeMillis) {
		this.latencyRecords.put(accIndex, System.currentTimeMillis());		
	}

	public long getSentTime(int accID){
		if(this.latencyRecords.containsKey(accID) == false){
			return Long.MAX_VALUE;
		}else{
			long sentTime = this.latencyRecords.remove(accID);
			return sentTime;			
		}
	}
	
	public static String roundDigit(double paramFloat) {
	    return String.format("%.2f", paramFloat);
	}
}
