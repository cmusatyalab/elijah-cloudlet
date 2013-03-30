package edu.cmu.cs.cloudlet.application.esvmtrainer.network;

import java.io.BufferedInputStream;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.net.UnknownHostException;


import org.json.JSONObject;
import android.os.Handler;
import android.os.Message;
import android.util.Log;

public class ESVMNetworkClient extends Thread {
	public static final String JSON_HEADER_VIDEOSIZE = "video_file_size";
	
	public static final int CALLBACK_FAILED = 0;
	public static final int CALLBACK_SUCCESS = 1;
	public static final int CALLBACK_UPDATE = 2;
	

	private byte[] binarySendingBuffer = new byte[3 * 1024 * 1024];
	private JSONObject header = null;
	private File sendingFile;
	private String serverAddress = null;
	private int serverPort = -1;

	private Handler networkCallbackHandler = null;
	private Socket clientSocket = null;
	private DataOutputStream networkWriter = null;
	private DataInputStream networkReader = null;
	private File outputFile;

	public ESVMNetworkClient(Handler handler, JSONObject header, File sendingFile, String address, int port) {
		this.networkCallbackHandler = handler;
		this.header = header;
		this.sendingFile = sendingFile;
		this.serverAddress = address;
		this.serverPort = port;
	}

	public void run() {
		// Init connection
		boolean ret = initConnection(this.serverAddress, this.serverPort);
		if (ret == false) {
			this.close();
			this.handleFailure("Cannot Connect to " + this.serverAddress + ":"+ this.serverPort);
			return;
		}

		// HTTP post
		try {
			this.sendRequest(this.header, this.sendingFile);			
		} catch (IOException e) {
			Log.e("krha", e.getMessage());
			this.close();
			this.handleFailure("Not valid JSON return");
			return;
		}

		// clean-up
		this.close();
		this.handleSuccess("SUCCESS");
	}
	
	private boolean initConnection(String ipAddress, int port) {
		try {
			this.clientSocket = new Socket();
			this.clientSocket.connect(new InetSocketAddress(ipAddress, port),
					10 * 1000);
			this.networkWriter = new DataOutputStream(
					clientSocket.getOutputStream());
			this.networkReader = new DataInputStream(
					clientSocket.getInputStream());
		} catch (UnknownHostException e) {
			return false;
		} catch (IOException e) {
			return false;
		}

		return true;
	}

	private void sendRequest(JSONObject jsonHeader, File file) throws IOException {
		if (this.networkReader != null && this.networkReader != null){
			int headerSize = jsonHeader.toString().length();
			this.networkWriter.writeInt(headerSize);
			this.networkWriter.writeBytes(jsonHeader.toString());

			// sending binary file
			int sendByte = -1, totalByte = 0;
			BufferedInputStream bi = new BufferedInputStream(new FileInputStream(file));
			while ((sendByte = bi.read(binarySendingBuffer, 0, binarySendingBuffer.length)) > 0) {
				networkWriter.write(binarySendingBuffer, 0, sendByte);
				totalByte += sendByte;
				String statusMsg = "Sending movie file.. " + (int) (100.0 * totalByte / file.length()) + "%, (" + totalByte
						+ "/" + file.length() + ")";
				this.handleNotification(statusMsg);
			}
			bi.close();
			networkWriter.flush();
		}
	}
	

	private void handleNotification(final String messageString) {
		Message msg = Message.obtain();
		msg.what = ESVMNetworkClient.CALLBACK_UPDATE;
		msg.obj = messageString;
		this.networkCallbackHandler.sendMessage(msg);
	}
	
	private void handleSuccess(String retMsg) {
		Message msg = Message.obtain();
		msg.what = ESVMNetworkClient.CALLBACK_SUCCESS;
		msg.obj = retMsg;
		this.networkCallbackHandler.sendMessage(msg);
	}

	private void handleFailure(String reason) {
		Message msg = Message.obtain();
		msg.what = ESVMNetworkClient.CALLBACK_FAILED;
		msg.obj = reason;
		this.networkCallbackHandler.sendMessage(msg);
	}

	public void close() {
		try {
			if (this.networkWriter != null)
				this.networkWriter.close();
		} catch (IOException e) {
			Log.e("error", e.getLocalizedMessage());
		}
		try {
			if (this.networkReader != null)
				this.networkReader.close();
		} catch (IOException e) {
			Log.e("error", e.getLocalizedMessage());
		}

		try {
			if (this.clientSocket != null)
				this.clientSocket.close();
		} catch (IOException e) {
			Log.e("error", e.getLocalizedMessage());
		}

		if (outputFile != null && outputFile.canRead()) {
			outputFile.delete();
		}
	}
}
