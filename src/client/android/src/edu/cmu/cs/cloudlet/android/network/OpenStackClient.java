package edu.cmu.cs.cloudlet.android.network;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.Vector;

import org.json.JSONException;
import org.json.JSONObject;

import android.content.Context;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;

public class OpenStackClient extends Thread {
	public static final String TAG = "krha_app";
	public static final int CALLBACK_FAILED = 210;
	public static final int CALLBACK_SUCCESS = 211;
	public static final int CALLBACK_UPDATE = 212;

	private Socket mClientSocket = null;
	private DataInputStream networkReader = null;
	private DataOutputStream networkWriter = null;

	private Context context;
	private Handler networkCallbackHandler;
	private Looper mLoop;

	// time stamp
	protected long dataSendStart;
	protected long dataSendEnd;
	protected long dataReceiveStart;
	protected long dataReceiveEnd;
	private Vector<JSONObject> requestList = new Vector<JSONObject>();
	private String serverIP = null;
	private int serverPort = -1;
	private boolean is_running = false;

	public OpenStackClient(Context context, Handler handler, String ip, int port) {
		this.context = context;
		this.networkCallbackHandler = handler;
		this.serverIP = ip;
		this.serverPort = port;
	}

	public void initConnection(String ip, int port) throws UnknownHostException, IOException {
		mClientSocket = new Socket();
		mClientSocket.connect(new InetSocketAddress(ip, port), 10 * 1000);
		networkWriter = new DataOutputStream(mClientSocket.getOutputStream());
		networkReader = new DataInputStream(mClientSocket.getInputStream());
	}

	public void run() {
		// init connection
		try {
			this.initConnection(this.serverIP, this.serverPort);
		} catch (UnknownHostException e) {
			this.handleFailure("Cannot connect to " + this.serverIP + ":" + this.serverPort + "\nReason : "
					+ e.getLocalizedMessage());
			this.close();
			return;
		} catch (IOException e) {
			this.handleFailure("Cannot connect to " + this.serverIP + ":" + this.serverPort + "\nReason : "
					+ e.getLocalizedMessage());
			this.close();
			return;
		}

		// processing request
		this.is_running = true;
		while (this.is_running) {
			if (requestList.size() == 0) {
				try {
					Thread.sleep(10);
				} catch (InterruptedException e) {
					e.printStackTrace();
				}
				continue;
			}

			JSONObject request = requestList.remove(0);
			try {
				requestServer(request);
			} catch (IOException e) {
				this.handleFailure("Failed to process request" + this.serverIP + ":" + this.serverPort + "\nReason : "
						+ e.getLocalizedMessage());
				e.printStackTrace();
			} catch (JSONException e) {
				this.handleFailure("Failed to process request" + this.serverIP + ":" + this.serverPort + "\nReason : "
						+ e.getLocalizedMessage());
				e.printStackTrace();
			}
		}
	}
	
	public void queueRequest(JSONObject request){
		this.requestList.add(request);
	}

	private void requestServer(JSONObject jsonRequest) throws IOException, JSONException {
		// send
		int headerSize = jsonRequest.toString().length();
		this.networkWriter.writeInt(headerSize);
		this.networkWriter.writeBytes(jsonRequest.toString());

		// recv
		int messageLength = this.networkReader.readInt();
		byte[] readBuffer = new byte[messageLength];
		this.networkReader.read(readBuffer, 0, readBuffer.length);
		JSONObject recvJson = new JSONObject(new String(readBuffer));

		// callback
		String result = recvJson.getString("result").toLowerCase();
		if (result.equals("success")){
			this.handleSuccess(recvJson);			
		} else {
			String error = recvJson.getString("error");
			this.handleFailure(error + "\ncheck floating-ip availability");
		}

	}

	private void handleNotification(final String messageString) {
		Message msg = Message.obtain();
		msg.what = OpenStackClient.CALLBACK_UPDATE;
		msg.obj = messageString;
		this.networkCallbackHandler.sendMessage(msg);
	}

	private void handleSuccess(JSONObject retJson) {
		Message msg = Message.obtain();
		msg.what = OpenStackClient.CALLBACK_SUCCESS;
		msg.obj = retJson;
		this.networkCallbackHandler.sendMessage(msg);
	}

	private void handleFailure(String reason) {
		Message msg = Message.obtain();
		msg.what = OpenStackClient.CALLBACK_FAILED;
		msg.obj = reason;
		this.networkCallbackHandler.sendMessage(msg);
	}

	public void close() {
		this.is_running = false;
		try {
			if (networkReader != null)
				networkReader.close();
			if (networkWriter != null)
				networkWriter.close();
			if (mClientSocket != null)
				mClientSocket.close();
		} catch (IOException e) {
		}
	}
}
