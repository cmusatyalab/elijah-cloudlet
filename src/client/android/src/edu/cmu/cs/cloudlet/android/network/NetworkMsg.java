//
// Copyright (C) 2011-2012 Carnegie Mellon University
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of version 2 of the GNU General Public License as published
// by the Free Software Foundation.  A copy of the GNU General Public License
// should have been distributed along with this program in the file
// LICENSE.GPL.

// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
// or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
// for more details.
//
package edu.cmu.cs.cloudlet.android.network;

import java.io.UnsupportedEncodingException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
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
	
	
	public String toString(){
		return jsonToString(4);
	}
	
	public byte[] toNetworkByte(){
		String jsonString = jsonToString(0);
		byte[] jsonBytes = jsonString.getBytes();
		if(jsonString != null){
			ByteBuffer byteBuffer = ByteBuffer.allocate(4 + jsonBytes.length);
			byteBuffer.order(ByteOrder.BIG_ENDIAN);
			byteBuffer.putInt(jsonBytes.length);
			byteBuffer.put(jsonBytes);
			return byteBuffer.array();
		}
		return null;
	}
	
	public int getCommandType() {
		int command = -1;
		try {
			command = this.jsonHeader.getInt(JSON_COMMAND_TYPE);
		} catch (JSONException e) {
			e.printStackTrace();
		}
		KLog.println("Recived Command : " + command);
		return command;
	}
}