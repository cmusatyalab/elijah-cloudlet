package edu.cmu.cs.cloudlet.android.network;

import java.util.ArrayList;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import edu.cmu.cs.cloudlet.android.data.VMInfo;
import edu.cmu.cs.cloudlet.android.util.CloudletEnv;

public class NetworkMsg {
	public final static int COMMAND_REQ_VMLIST					= 0x0011;
	public final static int COMMAND_ACK_VMLIST					= 0x0012;
	public final static int COMMAND_REQ_TRANSFER_START			= 0x0021;
	public final static int COMMAND_ACK_TRANSFER_START			= 0x0022;
	public final static int COMMAND_REQ_VM_LAUNCH				= 0x0031;
	public final static int COMMAND_ACK_VM_LAUNCH				= 0x0032;
	public final static int COMMAND_REQ_VM_STOP					= 0x0041;
	public final static int COMMAND_ACK_VM_STOP					= 0x0042;
	
	protected int commandNumber = -1;
	protected int payloadLength = -1;
	protected byte[] payload = null;
	protected JSONObject jsonPayload = null;
	
	public NetworkMsg(int command) {
		this.commandNumber = command;
	}

	public static NetworkMsg makeOverlayList() {
		NetworkMsg msg = new NetworkMsg(COMMAND_REQ_VMLIST);
		ArrayList<VMInfo> overlays = CloudletEnv.instance().getOverlayInformation();
		JSONObject json = NetworkMsg.generateJSON(overlays);
		
		// additional values
		try {
			json.put("number_of_core", "4");
		} catch (JSONException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		
		msg.jsonPayload = json;
		byte[] data = json.toString().getBytes();
		msg.payload = data;
		msg.payloadLength = data.length;
		return msg;
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
	
}

class VMList{	
}

