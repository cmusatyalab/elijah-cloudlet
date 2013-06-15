package edu.cmu.cs.cloudlet.android.network;

import java.io.DataInputStream;
import java.io.EOFException;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;

import org.apache.http.util.ByteArrayBuffer;
import org.json.JSONException;
import org.json.JSONObject;

import edu.cmu.cs.cloudlet.android.util.KLog;

import android.os.Bundle;
import android.os.Handler;
import android.os.Message;

public class NetworkClientReceiver extends Thread {
	private Handler mHandler;
	private DataInputStream networkReader;
	private boolean isThreadRun = true;
	
	public NetworkClientReceiver(DataInputStream dataInputStream, Handler mHandler) {
		this.networkReader = dataInputStream;
		this.mHandler = mHandler;
	}

	@Override
	public void run() {
		while(isThreadRun == true){
			ByteArrayBuffer msg;
			try {
				msg = this.receiveMsg(networkReader);
			} catch (EOFException e){
				break;								
			} catch (IOException e) {
				KLog.printErr(e.toString());
				this.notifyStatus(CloudletConnector.NETWORK_ERROR, e.toString(), null);
				break;
			}
			
			if(msg == null){			
				try { Thread.sleep(1);} catch (InterruptedException e) {}
				continue;
			}else{
				this.notifyStatus(CloudletConnector.PROGRESS_MESSAGE, "received..", msg);				
			}
		}
	}
	
	private void notifyStatus(int command, String string, ByteArrayBuffer recvMsg) {
		Message msg = Message.obtain();
		msg.what = command;
		msg.obj = recvMsg;
		Bundle data = new Bundle();
		data.putString("message", string);
		msg.setData(data);
		this.mHandler.sendMessage(msg);
	}

	public ByteArrayBuffer receiveMsg(DataInputStream reader) throws EOFException, IOException {
		int messageLength = reader.readInt();
		if(messageLength == -1){
			return null;
		}
		byte[] msgpackByte = new byte[messageLength];
		reader.read(msgpackByte, 0, msgpackByte.length);
		
		ByteArrayBuffer msgPackBuffer = new ByteArrayBuffer(messageLength);
		msgPackBuffer.append(msgpackByte, 0, msgpackByte.length);
	
		return msgPackBuffer;
	}
	
	public void close() {
		this.isThreadRun = false;		
		try {
			if(this.networkReader != null)
				this.networkReader.close();
		} catch (IOException e) {
			KLog.printErr(e.toString());
		}
	}
}
