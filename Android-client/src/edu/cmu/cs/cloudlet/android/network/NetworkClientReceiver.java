package edu.cmu.cs.cloudlet.android.network;

import java.io.DataInputStream;
import java.io.IOException;

import org.json.JSONArray;
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
	}

	@Override
	public void run() {
		while(isThreadRun == true){
			try {
				this.notifyStatus("reading...");				
				Thread.sleep(100);
			} catch (InterruptedException e) {
			}
			continue;
			
			/*
			NetworkMsg msg = this.receiveMsg(networkReader);
			if(msg == null){
				try {
					Thread.sleep(100);
				} catch (InterruptedException e) {
				}
				continue;
			}
			switch(msg.getCommandNumber()){
			case NetworkMsg.COMMAND_ACK_VMLIST:
				handleVMList(msg);
				break;
			case NetworkMsg.COMMAND_ACK_TRANSFER_START:
				handleTrasferStart(msg);
				break;
			case NetworkMsg.COMMAND_ACK_VM_LAUNCH:
				handleVMLaunch(msg);
				break;
			case NetworkMsg.COMMAND_ACK_VM_STOP:
				handleVMStop(msg);
				break;
			default:
				KLog.printErr("Cannot parse network command : " + msg);
				break;			
			}
			*/
		}
	}

	private void notifyStatus(String string) {
		Message msg = Message.obtain();
		msg.what = CloudletConnector.PROGRESS_MESSAGE;
		Bundle data = new Bundle();
		data.putString("message", string);
		msg.setData(data);
		this.mHandler.sendMessage(msg);
	}

	private NetworkMsg receiveMsg(DataInputStream reader) {
		NetworkMsg receiveMsg = null;		
		try {
			int msgNumber = reader.readInt();
			int payloadLength = reader.readInt();
			byte[] jsonByte = new byte[payloadLength];
			reader.read(jsonByte, 0, jsonByte.length);			
			receiveMsg = new NetworkMsg(msgNumber, payloadLength, jsonByte);
		} catch (IOException e) {
			KLog.printErr("Cannot read from network");
			KLog.printErr(e.toString());
		}
		return receiveMsg;
	}
	

	/*
	 * Network Command Handler method
	 */
	private void handleVMList(NetworkMsg msg) {
		JSONObject json = msg.getJsonPayload();
		try {
			String version = json.getString("Protocol-version");
			JSONArray vms = json.getJSONArray("VM");
			for(int i = 0; i < vms.length(); i++){
				JSONObject vm = (JSONObject) vms.get(i);
				KLog.println(vm.getString("type"));
				KLog.println(vm.getString("memorysnapshot_name"));
				KLog.println(vm.getString("name"));
				KLog.println(vm.getString("diskimg_name"));
			}
			KLog.println(version);
		} catch (JSONException e) {
			e.printStackTrace();
		}
		
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
	
	private void handleTrasferStart(NetworkMsg msg) {
	}

	private void handleVMLaunch(NetworkMsg msg) {
	}

	private void handleVMStop(NetworkMsg msg) {
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
