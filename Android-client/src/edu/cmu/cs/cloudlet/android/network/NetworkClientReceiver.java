package edu.cmu.cs.cloudlet.android.network;

import java.io.DataInputStream;

import android.os.Bundle;
import android.os.Handler;
import android.os.Message;

public class NetworkClientReceiver implements Runnable {

	private Handler mHandler;
	private DataInputStream networkReader;
	
	public NetworkClientReceiver(DataInputStream dataInputStream, Handler mHandler) {
		this.networkReader = dataInputStream;
	}

	@Override
	public void run() {
		// TODO Auto-generated method stub
		// callback
		/*
		Message msg = Message.obtain();
		msg.what = NetworkClientRec.FEEDBACK_RECEIVED;
		Bundle data = new Bundle();
		data.putInt("number_of_people", numberOfPeople);
		msg.setData(data);
		mHandler.sendMessage(msg);
		*/

	}

}
