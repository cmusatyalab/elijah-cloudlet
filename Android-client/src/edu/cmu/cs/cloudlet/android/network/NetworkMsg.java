package edu.cmu.cs.cloudlet.android.network;

import java.io.UnsupportedEncodingException;
import java.nio.ByteBuffer;
import java.util.ArrayList;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import edu.cmu.cs.cloudlet.android.data.VMInfo;
import edu.cmu.cs.cloudlet.android.util.CloudletEnv;
import edu.cmu.cs.cloudlet.android.util.KLog;

public class NetworkMsg {
	public final static int COMMAND_REQ_VMLIST					= 0x0011;
	public final static int COMMAND_ACK_VMLIST					= 0x0012;
	public final static int COMMAND_REQ_TRANSFER_START			= 0x0021;
	public final static int COMMAND_ACK_TRANSFER_START			= 0x0022;
	public final static int COMMAND_REQ_VM_LAUNCH				= 0x0031;
	public final static int COMMAND_ACK_VM_LAUNCH				= 0x0032;
	public final static int COMMAND_REQ_VM_STOP					= 0x0041;
	public final static int COMMAND_ACK_VM_STOP					= 0x0042;
	
	private static final String PROTOCOL_VERSION = "0.1";
	
	protected int commandNumber = -1;
	protected int payloadLength = -1;
	protected byte[] payload = null;
	protected JSONObject jsonPayload = null;
	
	
	public NetworkMsg(int command) {
		this.commandNumber = command;
	}
	
	public NetworkMsg(int command, int payloadLength, byte[] payload){
		this.commandNumber = command;
		this.payloadLength = payloadLength;
		this.payload = payload;
		String jsonString;
		try {
			jsonString = new String(payload, "UTF-8");
			this.jsonPayload = new JSONObject(jsonString);
		} catch (UnsupportedEncodingException e) {
			KLog.printErr(e.toString());
		} catch (JSONException e) {
			KLog.printErr(e.toString());
		}
	}
	
	/*
	 * Getter and Setter
	 */
	public int getCommandNumber() {
		return commandNumber;
	}
	public void setCommandNumber(int commandNumber) {
		this.commandNumber = commandNumber;
	}
	public JSONObject getJsonPayload() {
		return jsonPayload;
	}
	public void setJsonPayload(JSONObject jsonPayload) {
		this.jsonPayload = jsonPayload;
	}	
	
	/*
	 * Generating Sending Message
	 */
	public static NetworkMsg MSG_OverlayList() {
		NetworkMsg msg = new NetworkMsg(COMMAND_REQ_VMLIST);
		ArrayList<VMInfo> overlays = CloudletEnv.instance().getOverlayDirectoryInfo();
		JSONObject json = NetworkMsg.generateJSON(overlays);
		
		// additional values
		try {
			json.put("Protocol-version", NetworkMsg.PROTOCOL_VERSION);
			json.put("Request_synthesis_core", "4");
		} catch (JSONException e) {
			e.printStackTrace();
		}		
		saveJSON(msg, json);		
		return msg;
	}

	public static NetworkMsg MSG_SelectedVM(VMInfo vm) {
		NetworkMsg msg = new NetworkMsg(COMMAND_REQ_TRANSFER_START);
		ArrayList<VMInfo> overlay = new ArrayList<VMInfo>();
		overlay.add(vm);
		JSONObject json = NetworkMsg.generateJSON(overlay);
		
		// additional values
		try {
			json.put("Protocol-version", NetworkMsg.PROTOCOL_VERSION);
		} catch (JSONException e) {
			e.printStackTrace();
		}

		saveJSON(msg, json);		
		return msg;
	}

	public static NetworkMsg MSG_LaunchVM(VMInfo vm, int cpuNumber, int memSize) {
		NetworkMsg msg = new NetworkMsg(COMMAND_REQ_TRANSFER_START);
		ArrayList<VMInfo> overlay = new ArrayList<VMInfo>();
		overlay.add(vm);
		JSONObject json = NetworkMsg.generateJSON(overlay);
		
		// additional values
		try {
			json.put("Protocol-version", NetworkMsg.PROTOCOL_VERSION);
			if(cpuNumber > 0)
				json.put("memory_size", memSize + "");
			if(memSize > 0)
				json.put("vcpu_number", cpuNumber + "");			
		} catch (JSONException e) {
			e.printStackTrace();
		}

		saveJSON(msg, json);
		return msg;
	}

	public static NetworkMsg MSG_StopVM(VMInfo vm) {
		NetworkMsg msg = new NetworkMsg(COMMAND_REQ_TRANSFER_START);
		ArrayList<VMInfo> overlay = new ArrayList<VMInfo>();
		overlay.add(vm);
		JSONObject json = NetworkMsg.generateJSON(overlay);
		
		// additional values
		try {
			json.put("Protocol-version", NetworkMsg.PROTOCOL_VERSION);			
		} catch (JSONException e) {
			e.printStackTrace();
		}

		saveJSON(msg, json);		
		return msg;
	}
	
	/*
	 * JSON Utility
	 */
	public void printJSON(){
		if(this.jsonPayload != null){
			String jsonString = null;
			try {
				jsonString = this.jsonPayload.toString(2);
			} catch (JSONException e) {
				KLog.printErr(e.toString());
			}
			KLog.println(jsonString);
		}else{
			KLog.printErr("json is null");			
		}
	}
	
	/*
	 * JSON Generation 
	 */
	private static JSONObject generateJSON(ArrayList<VMInfo> overlays) {
		JSONObject rootObject = new JSONObject();
		JSONArray vmArray = new JSONArray();
		try {
			for(VMInfo vm : overlays){
				vmArray.put(vm.toJSON());
			}
			rootObject.put("VM", vmArray);
		} catch (JSONException e) {
			e.printStackTrace();
		}

		return rootObject;
	}

	private static void saveJSON(NetworkMsg msg, JSONObject json) {
		msg.jsonPayload = json;
		byte[] data = json.toString().getBytes();
		msg.payload = data;
		msg.payloadLength = data.length;
	}
	
	public String toString(){
		return this.commandNumber + " " + this.payloadLength + " " + this.jsonPayload;
	}
	
	public byte[] toNetworkByte(){
		ByteBuffer byteBuffer = ByteBuffer.allocate(4 + 4 + this.payload.length);
		byteBuffer.putInt(this.commandNumber);
		byteBuffer.putInt(this.payloadLength);
		byteBuffer.put(this.payload);
		return byteBuffer.array();
	}
	
}
