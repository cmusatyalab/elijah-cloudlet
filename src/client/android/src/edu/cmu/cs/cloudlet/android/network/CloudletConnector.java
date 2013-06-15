package edu.cmu.cs.cloudlet.android.network;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.File;
import java.io.IOException;
import java.math.BigInteger;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;
import java.util.Vector;
import java.util.Map.Entry;

import org.apache.http.util.ByteArrayBuffer;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;
import org.msgpack.MessagePack;
import org.msgpack.packer.BufferPacker;
import org.msgpack.type.ArrayValue;
import org.msgpack.type.Value;
import org.msgpack.type.ValueType;
import org.msgpack.unpacker.BufferUnpacker;

import edu.cmu.cs.cloudlet.android.CloudletActivity;
import edu.cmu.cs.cloudlet.android.R;
import edu.cmu.cs.cloudlet.android.application.CloudletCameraActivity;
import edu.cmu.cs.cloudlet.android.data.Measure;
import edu.cmu.cs.cloudlet.android.data.VMInfo;
import edu.cmu.cs.cloudlet.android.discovery.CloudletDiscovery;
import edu.cmu.cs.cloudlet.android.lib.discovery.CloudletDiscoveryAPI;
import edu.cmu.cs.cloudlet.android.util.KLog;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.util.TypedValue;
import android.view.View;
import android.widget.TextView;

public class CloudletConnector extends Thread {
	// Network Message
	public static final int CONNECTION_ERROR = 1;
	public static final int NETWORK_ERROR = 2;
	public static final int PROGRESS_MESSAGE = 3;
	public static final int FINISH_MESSAGE = 4;
	public static final int PROGRESS_MESSAGE_TRANFER = 5;

	// Synthesis callback mmessage
	public static final int SYNTHESIS_SUCCESS = 10;
	public static final int SYNTHESIS_FAILED = 11;
	public static final int SYNTHESIS_PROGRESS_MESSAGE = 21;
	public static final int SYNTHESIS_PROGRESS_PERCENT = 22;

	protected Handler synthesisCallbackHandler;
	protected Context mContext;
	private NetworkClientReceiver receiver;
	protected NetworkClientSender sender;

	private VMInfo overlayVM;
	private Socket mClientSocket;
	private DataOutputStream networkWriter;
	private DataInputStream networkReader;
	private int port;
	private String ipAddress;
	private BigInteger sessionID;

	public CloudletConnector(Context context, Handler handler) {
		this.synthesisCallbackHandler = handler;
		this.mContext = context;
	}

	public void setConnection(String ipAddress, int port, VMInfo overlayVM) {
		this.ipAddress = ipAddress;
		this.port = port;
		this.overlayVM = overlayVM;
	}

	public void run() {
		// Create Session (Associate)
		this.sessionID = CloudletDiscoveryAPI.asssociate(this.ipAddress,
				this.port);

		// Make connection
		boolean ret = this.initConnection(this.ipAddress, this.port);
		if (ret == false) {
			this.close();
			handleFailSynthesis("Cannot Connect to " + this.ipAddress + ":"
					+ this.port);
			return;
		}

		// socket sender/receiver thread
		this.sender.setConnector(this);
		this.sender.start();
		this.receiver.start();

		// Send Synthesis start message
		NetworkMsg networkMsg = NetworkMsg.MSG_send_overlaymeta(this.overlayVM,
				this.sessionID);
		this.sender.requestCommand(networkMsg);

		// wait for finishing transfer
		while (true){
			if (this.overlayVM.transferFinish() == true){
				Measure.record(Measure.OVERLAY_TRANSFER_END);
				break;
			}
			try {
				sleep(100);
			} catch (InterruptedException e) {
				e.printStackTrace();
			}
		}
	}

	private boolean initConnection(String ipAddress, int port) {
		try {
			this.mClientSocket = new Socket();
			this.mClientSocket.connect(new InetSocketAddress(ipAddress, port),
					10 * 1000);
			this.networkWriter = new DataOutputStream(
					mClientSocket.getOutputStream());
			this.networkReader = new DataInputStream(
					mClientSocket.getInputStream());
			this.sender = new NetworkClientSender(this.networkWriter,
					eventHandler);
			this.receiver = new NetworkClientReceiver(this.networkReader,
					eventHandler);
		} catch (UnknownHostException e) {
			return false;
		} catch (IOException e) {
			return false;
		}

		return true;
	}

	public void closeRequest() {
		try {
			// Request Finishing synchronously
			NetworkMsg networkCommand = NetworkMsg
					.MSG_send_finishMessage(this.sessionID);
			this.sender.sendCommand(this.networkWriter, networkCommand);
			ByteArrayBuffer responseArray = this.receiver
					.receiveMsg(this.networkReader);
			HashMap messageMap = convertMessagePackToMap(responseArray
					.toByteArray());
			NetworkMsg retMsg = new NetworkMsg(messageMap);
			int command = retMsg.getCommandType();

			// TODO: disassociate session
		} catch (IOException e) {
			// KLog.printErr(e.getMessage());
		}
		this.close();
	}

	public void close() {
		if (this.sender != null)
			this.sender.close();

		if (this.receiver != null)
			this.receiver.close();

		try {
			if (this.networkWriter != null)
				this.networkWriter.close();
			if (this.networkReader != null)
				this.networkReader.close();
		} catch (IOException e) {
			KLog.printErr(e.getLocalizedMessage());
		}

		KLog.println("Socket Connection Closed");
	}

	/*
	 * Server Message Handler
	 */
	Handler eventHandler = new Handler() {
		public void handleMessage(Message msg) {
			if (msg.what == CloudletConnector.NETWORK_ERROR) {
				if (sender != null) {
					sender.close();
				}
				String message = msg.getData().getString("message");
			} else if (msg.what == CloudletConnector.PROGRESS_MESSAGE) {
				// Response parsing
				ByteArrayBuffer responseArray = (ByteArrayBuffer) msg.obj;
				String resString = new String(responseArray.toByteArray());

				// unpack messsage-pack
				HashMap messageMap = null;
				try {
					messageMap = convertMessagePackToMap(responseArray
							.toByteArray());
				} catch (IOException e) {
					KLog.printErr(e.toString());
					handleFailSynthesis("Synthesis message failed");
					return;
				}

				// Handle Response
				Object value = messageMap.get(NetworkMsg.KEY_COMMAND);
				BigInteger command = (BigInteger) messageMap
						.get(NetworkMsg.KEY_COMMAND);
				switch (command.intValue()) {
				case NetworkMsg.MESSAGE_COMMAND_SUCCESS:
					break;
				case NetworkMsg.MESSAGE_COMMAND_SYNTHESIS_DONE:
					KLog.println("3. Synthesis finished successfully");
					Measure.record(Measure.VM_LAUNCHED);
					handleSucessSynthesis();
					break;
				case NetworkMsg.MESSAGE_COMMAND_FAIELD:
					KLog.println("Synthesis failed");
					handleFailSynthesis("Synthesis protocol failed");
					break;
				case NetworkMsg.MESSAGE_COMMAND_ON_DEMAND:
					handleOverlayRequest(messageMap);
					break;
				}
			}
		}

	};

	public static HashMap convertMessagePackToMap(byte[] byteArray)
			throws IOException {
		HashMap<String, Object> messageMap = new HashMap<String, Object>();
		MessagePack msgpack = new MessagePack();
		BufferUnpacker au = msgpack.createBufferUnpacker().wrap(byteArray);
		Map<Value, Value> metaMap = au.readValue().asMapValue();
		for (Map.Entry<Value, Value> e : ((Map<Value, Value>) metaMap)
				.entrySet()) {
			ValueType valueType = e.getValue().getType();
			String key = e.getKey().toString().replace("\"", "");
			switch (valueType) {
			case BOOLEAN:
				messageMap.put(key, e.getValue().asBooleanValue().getBoolean());
				break;
			case INTEGER:
				messageMap.put(key, e.getValue().asIntegerValue()
						.getBigInteger());
				break;
			case FLOAT:
				messageMap.put(key, e.getValue().asFloatValue().getDouble());
				break;
			case ARRAY:
				messageMap.put(key, e.getValue().asArrayValue());
				break;
			case MAP:
				messageMap.put(key, e.getValue().asMapValue());
				break;
			case RAW:
				String rawValue = e.getValue().asRawValue().toString()
						.replace("\"", "");
				messageMap.put(key, rawValue);
				break;
			case NIL:
				messageMap.put(key, e.getValue().asNilValue());
				break;
			}
		}
		return messageMap;
	}

	/*
	 * Command Handler
	 */
	private void handleSucessSynthesis() {
		Message msg = Message.obtain();
		msg.what = CloudletConnector.SYNTHESIS_SUCCESS;
		msg.obj = (String) this.overlayVM.getAppName();
		synthesisCallbackHandler.sendMessage(msg);

	}

	private void handleFailSynthesis(String reason) {
		Message msg = Message.obtain();
		msg.what = CloudletConnector.SYNTHESIS_FAILED;
		msg.obj = reason;
		synthesisCallbackHandler.sendMessage(msg);
	}

	private void handleOverlayRequest(HashMap messageMap) {
		// add requested overlay to queue
		String ovelraySegmentURI = (String) messageMap
				.get(NetworkMsg.KEY_REQUEST_SEGMENT);
		try {
			File overlayFile = overlayVM.getOverlayFile(ovelraySegmentURI
					.toString());
			NetworkMsg networkMsg = NetworkMsg.MSG_send_overlayfile(
					overlayFile, this.sessionID);
			sender.requestCommand(networkMsg);
		} catch (IOException e) {
			e.printStackTrace();
		}
	}

	public void updateMessage(String messageString) {
		Message msg = Message.obtain();
		msg.what = CloudletConnector.SYNTHESIS_PROGRESS_MESSAGE;
		msg.obj = messageString;
		synthesisCallbackHandler.sendMessage(msg);
	}
	
	public void updateStatus(int percent) {
		Message msg = Message.obtain();
		msg.what = CloudletConnector.SYNTHESIS_PROGRESS_PERCENT;
		msg.obj = new Integer(percent);
		synthesisCallbackHandler.sendMessage(msg);
	}

	public void updateTransferredOverlay(File overlayFile) {
		this.overlayVM.addTransferredOverlay(overlayFile);
	}
}
