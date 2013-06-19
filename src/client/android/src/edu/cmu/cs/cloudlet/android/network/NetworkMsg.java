package edu.cmu.cs.cloudlet.android.network;

import java.io.File;
import java.io.IOException;
import java.io.UnsupportedEncodingException;
import java.math.BigInteger;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;
import java.util.Map.Entry;

import org.apache.http.util.ByteArrayBuffer;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;
import org.msgpack.MessagePack;
import org.msgpack.packer.BufferPacker;
import org.msgpack.type.Value;

import edu.cmu.cs.cloudlet.android.data.VMInfo;
import edu.cmu.cs.cloudlet.android.util.KLog;

public class NetworkMsg {

	// client -> server
	public static final int MESSAGE_COMMAND_SEND_META = 0x11;
	public static final int MESSAGE_COMMAND_SEND_OVERLAY = 0x12;
	public static final int MESSAGE_COMMAND_FINISH = 0x13;
	public static final int MESSAGE_COMMAND_GET_RESOURCE_INFO   = 0x14;
	public static final int MESSAGE_COMMAND_SESSION_CREATE      = 0x15;
	public static final int MESSAGE_COMMAND_SESSION_CLOSE       = 0x16;
	
	// server -> client
	public static final int MESSAGE_COMMAND_SUCCESS = 0x01;
	public static final int MESSAGE_COMMAND_FAIELD = 0x02;
	public static final int MESSAGE_COMMAND_ON_DEMAND = 0x03;
	public static final int MESSAGE_COMMAND_SYNTHESIS_DONE = 0x04;

	// key 
	public static final String KEY_ERROR = "error";
	public static final String KEY_COMMAND = "command";
	public static final String KEY_META_SIZE = "meta_size";
	public static final String KEY_REQUEST_SEGMENT = "blob_uri";
	public static final String KEY_REQUEST_SEGMENT_SIZE = "blob_size";
	public static final String KEY_FAILED_REAONS = "reasons";
	public static final String KEY_PAYLOAD = "payload";
	public static final String KEY_SESSION_ID = "session_id";
	
	// synthesis option
	public static final String KEY_SYNTHESIS_OPTION = "synthesis_option";
	public static final String SYNTHESIS_OPTION_DISPLAY_VNC = "option_display_vnc";
	public static final String SYNTHESIS_OPTION_EARLY_START = "option_early_start";
	public static final String SYNTHESIS_OPTION_SHOW_STATISTICS = "option_show_statistics";

	private File selectedfile;
	private byte[] networkBytes;
	private Map messageMap;

	public <K, V> NetworkMsg(Map<K, V> message) throws IOException {
		this.messageMap = message;
		MessagePack msgpack = new MessagePack();
		BufferPacker packer = msgpack.createBufferPacker();
		packer.writeMapBegin(message.size());
		for (Map.Entry<K, V> e : ((Map<K, V>) message).entrySet()) {
			packer.write(e.getKey());
			packer.write(e.getValue());
		}
		packer.writeMapEnd();
		this.networkBytes = packer.toByteArray();
	}
	
	public static NetworkMsg MSG_send_associateMessage() {
		HashMap<String, Object> overlay_header = new HashMap<String, Object>();
		overlay_header.put(KEY_COMMAND, MESSAGE_COMMAND_SESSION_CREATE);

		NetworkMsg msg;
		try {
			msg = new NetworkMsg(overlay_header);
			return msg;
		} catch (IOException e) {
			e.printStackTrace();
		}
		return null;
	}

	
	public static NetworkMsg MSG_send_overlaymeta(VMInfo overlay, BigInteger sessionID) {
		HashMap<String, Object> overlay_header = new HashMap<String, Object>();
		overlay_header.put(KEY_COMMAND, MESSAGE_COMMAND_SEND_META);
		overlay_header.put(KEY_META_SIZE, (int) overlay.getMetaFile().length());
		overlay_header.put(KEY_SESSION_ID, sessionID);

		// synthesis option
		HashMap<String, Boolean> options = new HashMap<String, Boolean>();		
		options.put(SYNTHESIS_OPTION_DISPLAY_VNC, true);
		options.put(SYNTHESIS_OPTION_EARLY_START, false);
		overlay_header.put(KEY_SYNTHESIS_OPTION, options);
		NetworkMsg msg;
		try {
			msg = new NetworkMsg(overlay_header);
			msg.selectedfile = overlay.getMetaFile();
			return msg;
		} catch (IOException e) {
			e.printStackTrace();
		}
		return null;
	}


	public static NetworkMsg MSG_send_overlayfile(File overlayFile, BigInteger sessionID) {
		HashMap<String, Object> overlay_header = new HashMap<String, Object>();
		overlay_header.put(KEY_COMMAND, MESSAGE_COMMAND_SEND_OVERLAY);
		overlay_header.put(KEY_REQUEST_SEGMENT, overlayFile.getName());
		overlay_header.put(KEY_REQUEST_SEGMENT_SIZE, overlayFile.length());
		overlay_header.put(KEY_SESSION_ID, sessionID);

		NetworkMsg msg = null;
		try {
			msg = new NetworkMsg(overlay_header);
			msg.selectedfile = overlayFile;
		} catch (IOException e) {
			e.printStackTrace();
		}
		return msg;
		
	}

	public static NetworkMsg MSG_send_finishMessage(BigInteger sessionID) {
		HashMap<String, Object> finishHeader = new HashMap<String, Object>();
		finishHeader.put(NetworkMsg.KEY_COMMAND, NetworkMsg.MESSAGE_COMMAND_FINISH);
		finishHeader.put(NetworkMsg.KEY_SESSION_ID, sessionID);
		finishHeader.put("Measure", "Not available yet");

		NetworkMsg msg = null;
		try {
			msg = new NetworkMsg(finishHeader);
		} catch (IOException e) {
			e.printStackTrace();
		}
		return msg;
	}
	
	public File getSelectedFile() {
		return this.selectedfile;
	}
	
	public Map getMessageMap(){
		return this.messageMap;
	}

	public byte[] toNetworkByte() {
		byte[] bytes = this.networkBytes;
		ByteBuffer byteBuffer = ByteBuffer.allocate(4 + bytes.length);
		byteBuffer.order(ByteOrder.BIG_ENDIAN);
		byteBuffer.putInt(bytes.length);
		byteBuffer.put(bytes);
		return byteBuffer.array();
	}

	public int getCommandType() {
		Object obj = this.messageMap.get(KEY_COMMAND);
		int command = -1;
		if (obj instanceof Value) {
			command = ((Value) obj).asIntegerValue().intValue();
		}else{
			command = ((Integer) this.messageMap.get(KEY_COMMAND)).intValue();
		}
		return command;
	}

}
