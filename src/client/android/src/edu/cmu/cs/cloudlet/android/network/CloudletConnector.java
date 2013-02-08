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

import java.io.File;
import java.io.IOException;
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

public class CloudletConnector {
	public static final int CONNECTION_ERROR = 1;
	public static final int NETWORK_ERROR = 2;
	public static final int PROGRESS_MESSAGE = 3;
	public static final int FINISH_MESSAGE = 4;
	public static final int PROGRESS_MESSAGE_TRANFER = 5;

	protected CloudletActivity activity;
	protected Context mContext;
	protected NetworkClientSender networkClient;
	protected ProgressDialog mDialog;

	private VMInfo overlayVM;

	public CloudletConnector(CloudletActivity activity, Context context) {
		this.activity = activity;
		this.mContext = context;
	}

	public void startConnection(String ipAddress, int port, VMInfo overlayVM) {
		if (mDialog == null) {
			mDialog = ProgressDialog.show(this.mContext, "Info", "Connecting to " + ipAddress, true);
			mDialog.setIcon(R.drawable.ic_launcher);
		} else {
			mDialog.setMessage("Connecting to " + ipAddress);
		}
		mDialog.show();

		if (networkClient != null) {
			networkClient.close();
			networkClient.stop();
		}
		networkClient = new NetworkClientSender(this.mContext, eventHandler);
		networkClient.setConnector(this);
		networkClient.setConnection(ipAddress, port);
		networkClient.start();

		// Send Synthesis start message
		mDialog.setMessage("Step 1. Requesting VM Synthesis");
		this.overlayVM = overlayVM;
		NetworkMsg networkMsg = NetworkMsg.MSG_send_overlaymeta(this.overlayVM);
		networkClient.requestCommand(networkMsg);
	}

	public void closeRequest() {
		NetworkMsg networkMsg = NetworkMsg.MSG_send_finishMessage();
		networkClient.requestCommand(networkMsg);
	}

	public void updateMessage(String dialogMessage) {
		if (mDialog != null && mDialog.isShowing()) {
			mDialog.setMessage(dialogMessage);
		}
	}

	public void close() {
		if (this.networkClient != null)
			this.networkClient.close();
	}

	/*
	 * Server Message Handler
	 */
	Handler eventHandler = new Handler() {
		public void handleMessage(Message msg) {
			if (msg.what == CloudletConnector.CONNECTION_ERROR) {
				if (mDialog != null) {
					mDialog.dismiss();
				}

				String message = msg.getData().getString("message");
				showAlertDialog(message);
			} else if (msg.what == CloudletConnector.NETWORK_ERROR) {
				if (mDialog != null) {
					mDialog.dismiss();
				}
				if (networkClient != null) {
					networkClient.close();
				}
				String message = msg.getData().getString("message");

				showAlertDialog(message);
			} else if (msg.what == CloudletConnector.PROGRESS_MESSAGE) {
				// Response parsing
				ByteArrayBuffer responseArray = (ByteArrayBuffer) msg.obj;
				String resString = new String(responseArray.toByteArray());
				// KLog.println(resString);

				// unpack messsage-pack
				HashMap messageMap = null;
				try {
					messageMap = convertMessagePackToMap(responseArray.toByteArray());
				} catch (IOException e) {
					KLog.printErr(e.toString());
					showAlertDialog("Invalid message-pack response : " + resString);
					return;
				}

				// Handle Response
				Object value = messageMap.get(NetworkMsg.KEY_COMMAND);
				Integer command = (Integer) messageMap.get(NetworkMsg.KEY_COMMAND);
				switch (command.intValue()) {
				case NetworkMsg.MESSAGE_COMMAND_SUCCESS:
					KLog.println("3. Synthesis finished successfully");
					updateMessage("Step 3. Synthesis is done..");
					Measure.put(Measure.NET_ACK_OVERLAY_TRASFER);
					if (mDialog != null) {
						mDialog.dismiss();
					}
					handleSucessSynthesis();
					break;
				case NetworkMsg.MESSAGE_COMMAND_FAIELD:
					KLog.println("Synthesis failed");
					updateMessage("Synthesis failed..");
					break;
				case NetworkMsg.MESSAGE_COMMAND_ON_DEMAND:
					handleOverlayRequest(messageMap);
					break;
				case NetworkMsg.MESSAGE_COMMAND_FINISH_SUCCESS:
					handleFinishSuccess(messageMap);
					break;
				}
			}
		}

	};

	private HashMap convertMessagePackToMap(byte[] byteArray) throws IOException {
		HashMap<String, Object> messageMap = new HashMap<String, Object>();
		MessagePack msgpack = new MessagePack();
		BufferUnpacker au = msgpack.createBufferUnpacker().wrap(byteArray);
		Map<Value, Value> metaMap = au.readValue().asMapValue();
		for (Map.Entry<Value, Value> e : ((Map<Value, Value>) metaMap).entrySet()) {
			ValueType valueType = e.getValue().getType();
			String key = e.getKey().toString().replace("\"", "");
			switch (valueType) {
			case BOOLEAN:
				messageMap.put(key, e.getValue().asBooleanValue().getBoolean());
				break;
			case INTEGER:
				messageMap.put(key, e.getValue().asIntegerValue().getInt());
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
				String rawValue = e.getValue().asRawValue().toString().replace("\"", "");
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
		boolean isSuccess = activity.runStandAlone(this.overlayVM.getAppName());
		if (isSuccess == false) {
			// If we cannot find matching application,
			// then ask user to close VM

			String message = "VM synthesis is successfully finished, but cannot find matching Android application named ("
					+ this.overlayVM.getAppName() + ")\n\nClose VM at Cloudlet?";
			new AlertDialog.Builder(this.activity).setTitle("Info").setMessage(message)
					.setPositiveButton("Yes", new DialogInterface.OnClickListener() {
						public void onClick(DialogInterface dialog, int which) {
							closeRequest();
						}
					}).setNegativeButton("No", null).show();
		}
	}

	private void handleOverlayRequest(HashMap messageMap) {
		// add requested overlay to queue
		String ovelraySegmentURI = (String) messageMap.get(NetworkMsg.KEY_REQUEST_SEGMENT);
		try {
			File overlayFile = overlayVM.getOverlayFile(ovelraySegmentURI.toString());
			NetworkMsg networkMsg = NetworkMsg.MSG_send_overlayfile(overlayFile);
			networkClient.requestCommand(networkMsg);
		} catch (IOException e) {
			showAlertDialog(e.getMessage());
			e.printStackTrace();
		}
	}

	private void handleFinishSuccess(HashMap messageMap) {
		this.close();
//		new AlertDialog.Builder(mContext).setTitle("Info").setIcon(R.drawable.ic_launcher)
//				.setMessage("Finished gracefully").setNegativeButton("Confirm", null).show();
	}

	// End Command Handler

	private void showAlertDialog(String errorMsg) {
		if (activity.isFinishing() == false) {
			new AlertDialog.Builder(mContext).setTitle("Error").setIcon(R.drawable.ic_launcher).setMessage(errorMsg)
					.setNegativeButton("Confirm", null).show();
		}
	}
}