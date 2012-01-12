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
	public final static int COMMAND_REQ_TRANSFER_START			= 0x0021;
	public final static int COMMAND_ACK_TRANSFER_START			= 0x0022;
	
	private static final String JSON_PROTOCOL_VERSION = "protocol-version";
	private static final String JSON_COMMAND_TYPE = "command";
	
	protected JSONObject jsonHeader = null;
	private VMInfo selecteOverlayInfo;
	
	public NetworkMsg(JSONObject jsonHeader) {
		this.jsonHeader = jsonHeader;
	}
	/*
	 * Getter and Setter
	 */
	public JSONObject getJsonPayload() {
		return jsonHeader;
	}
	
	/*
	 * Generating Sending Message
	 */
	public static NetworkMsg MSG_SelectedVM(VMInfo selecteOverlayInfo) {
		ArrayList<VMInfo> baseVMList = new ArrayList<VMInfo>();
		baseVMList.add(selecteOverlayInfo);
		
		// JSON Creation
		JSONObject json = NetworkMsg.generateJSON(baseVMList);
		try {
			json.put(JSON_PROTOCOL_VERSION, NetworkMsg.JSON_PROTOCOL_VERSION);
			json.put(JSON_COMMAND_TYPE, COMMAND_REQ_TRANSFER_START);
			json.put("Request_synthesis_core", "4");
		} catch (JSONException e) {
			e.printStackTrace();
		}

		NetworkMsg msg = new NetworkMsg(json);
		msg.setOverlayInfo(selecteOverlayInfo);
		return msg;
	}

	private void setOverlayInfo(VMInfo selecteOverlayInfo) {
		this.selecteOverlayInfo = selecteOverlayInfo;
		
	}
	
	public VMInfo getOverlayInfo() {
		return this.selecteOverlayInfo;
	}
	
	/*
	 * JSON Utility
	 */
	public String jsonToString(int indentSize){
		if(this.jsonHeader != null){
			String jsonString = null;
			try {
				jsonString = this.jsonHeader.toString(indentSize);
			} catch (JSONException e) {
				KLog.printErr(e.toString());
			}
			return jsonString;
		}else{
			return null;
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
	
	public String toString(){
		return jsonToString(4);
	}
	
	public byte[] toNetworkByte(){
		String jsonString = jsonToString(0);
		if(jsonString != null){
			ByteBuffer byteBuffer = ByteBuffer.allocate(4 + jsonString.length());
			byteBuffer.putInt(jsonString.length());
			return byteBuffer.array();			
		}
		return null;
	}
	
	public int getCommandType() {
		int command = -1;
		try {
			command = this.jsonHeader.getInt(JSON_COMMAND_TYPE);
		} catch (JSONException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		return command;
	}
}
