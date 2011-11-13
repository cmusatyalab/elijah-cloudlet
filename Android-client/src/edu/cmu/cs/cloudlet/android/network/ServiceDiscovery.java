package edu.cmu.cs.cloudlet.android.network;

import android.os.Bundle;
import android.os.Handler;
import android.os.Message;

public class ServiceDiscovery extends Thread {
	public static final int SERVICE_FOUND = 0;
	public static final String KEY_SERVICE_IP_ADDRESS = "cloudlet_ipaddress";
	public static final String KEY_SERVICE_PORT = "cloudlet_port";
	
	protected Handler discoveryHandler = null;

	public void setHandler(Handler discoveryHandler) {
		this.discoveryHandler = discoveryHandler;
	}
	
	public void run(){
		// Find Service
		while(true){
			try {
				Thread.sleep(1000);
			} catch (InterruptedException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
		}
		
		/*
		if(this.discoveryHandler != null){
			// callback
			Message msg = Message.obtain();
			msg.what = ServiceDiscovery.SERVICE_FOUND;
			Bundle data = new Bundle();
			data.putString(KEY_SERVICE_IP_ADDRESS, "localhost");
			data.putInt(KEY_SERVICE_PORT, 9999);
			msg.setData(data);
			discoveryHandler.sendMessage(msg);
		}
		*/
	}

}
