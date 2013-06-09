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

import org.json.JSONException;
import org.json.JSONObject;

import edu.cmu.cs.cloudlet.application.esvmtrainer.util.ZipUtility;
import android.os.Environment;
import android.os.Handler;
import android.os.Message;
import android.util.Log;

public class ESVMNetworkClient extends Thread {
	public static final String JSON_HEADER_ZIPFILE_SIZE = "zip_file_size";
	public static final String JSON_HEADER_MODEL_NAME = "model_name";
	
	public static final String JSON_RET_COMMAND = "return";
	public static final String JSON_RET_SUCCESS = "success";
	public static final String JSON_RET_FAILED = "failed";
	public static final String JSON_RET_REASONS = "reasons";

	public static final int CALLBACK_FAILED = 0;
	public static final int CALLBACK_SUCCESS = 1;
	public static final int CALLBACK_UPDATE = 2;

	private byte[] binarySendingBuffer = new byte[3 * 1024 * 1024];
	private JSONObject header = null;
	private File[] imageFiles;
	private String serverAddress = null;
	private int serverPort = -1;

	private Handler networkCallbackHandler = null;
	private Socket clientSocket = null;
	private DataOutputStream networkWriter = null;
	private DataInputStream networkReader = null;
	private File outputFile;

	public ESVMNetworkClient(Handler handler, JSONObject header, File[] imageFiles, String address, int port) {
		this.networkCallbackHandler = handler;
		this.header = header;
		this.imageFiles = imageFiles;
		this.serverAddress = address;
		this.serverPort = port;
	}

	public void run() {
		// Zip target files
		this.outputFile = null;
		File zipDir = imageFiles[0].getParentFile();
		try {
			this.outputFile = File.createTempFile("ESVMImages", ".zip", zipDir);
			ZipUtility.zipFiles(this.imageFiles, outputFile);
		} catch (IOException e) {
			this.close();
			this.handleFailure("Cannot Create Zip file at " + outputFile.getAbsolutePath());
			return;
		}

		// update json header
		try {
			this.header.put(ESVMNetworkClient.JSON_HEADER_ZIPFILE_SIZE, this.outputFile.length());
		} catch (JSONException e) {
			this.close();
			this.handleFailure("Cannot Update JSON header");
			return;
		}
		
		// Init connection
		boolean ret = initConnection(this.serverAddress, this.serverPort);
		if (ret == false) {
			this.close();
			this.handleFailure("Cannot Connect to " + this.serverAddress + ":" + this.serverPort);
			return;
		}

		// TCP request and response
		long processStartTime = System.currentTimeMillis();
		JSONObject retJson = null;
		try {
			retJson = this.sendRequest(this.header, this.outputFile);			
		} catch (IOException e) {
			Log.e("krha", e.getMessage());
			this.close();
			this.handleFailure("Network error while sending data");
			return;
		} catch (JSONException e) {
			Log.e("krha", e.getMessage());
			this.close();
			this.handleFailure("Not valid JSON return : " + e.getMessage());
			return;
		}
		long processEndTime = System.currentTimeMillis();
		Log.d("krha", "response time : " + (processEndTime-processStartTime));
		
		// handle return message
		this.close();
		if (retJson == null){
			this.handleFailure("Failed receiving return message");
		}else{
			try {
				String command = retJson.getString(JSON_RET_COMMAND);				
				if (command.toLowerCase().equals(JSON_RET_SUCCESS.toLowerCase())){
					String reasons = retJson.getString(JSON_RET_REASONS);
					this.handleSuccess("SUCCESS : " + reasons);				
				} else{
					String reasons = retJson.getString(JSON_RET_REASONS);
					this.handleFailure(reasons);
				}
			} catch (JSONException e) {
				e.printStackTrace();
				this.handleFailure("Not valid JSON return : " + e.getMessage());
			}
		}
	}

	private boolean initConnection(String ipAddress, int port) {
		try {
			this.clientSocket = new Socket();
			this.clientSocket.connect(new InetSocketAddress(ipAddress, port), 10 * 1000);
			this.networkWriter = new DataOutputStream(clientSocket.getOutputStream());
			this.networkReader = new DataInputStream(clientSocket.getInputStream());
		} catch (UnknownHostException e) {
			return false;
		} catch (IOException e) {
			return false;
		}

		return true;
	}

	private JSONObject sendRequest(JSONObject jsonHeader, File file) throws IOException, JSONException {
		if (this.networkReader != null && this.networkReader != null) {
			int headerSize = jsonHeader.toString().length();
			this.networkWriter.writeInt(headerSize);
			this.networkWriter.writeBytes(jsonHeader.toString());

			// sending binary file
			int sendByte = -1, totalByte = 0;
			BufferedInputStream bi = new BufferedInputStream(new FileInputStream(file));
			while ((sendByte = bi.read(binarySendingBuffer, 0, binarySendingBuffer.length)) > 0) {
				networkWriter.write(binarySendingBuffer, 0, sendByte);
				totalByte += sendByte;
				String statusMsg = "Sending movie file.. " + (int) (100.0 * totalByte / file.length()) + "%, ("
						+ totalByte + "/" + file.length() + ")";
				this.handleNotification(statusMsg);
			}
			bi.close();
			networkWriter.flush();
			
			// recv server message			
			int messageLength = this.networkReader.readInt();
			byte[] readBuffer = new byte[messageLength];
			this.networkReader.read(readBuffer, 0, readBuffer.length);
			JSONObject recvJson = new JSONObject(new String(readBuffer));
			return recvJson;
		}
		return null;
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
			if (this.networkReader != null)
				this.networkReader.close();
			if (this.clientSocket != null)
				this.clientSocket.close();
		} catch (IOException e) {
			Log.e("error", e.getLocalizedMessage());
		}
		
		if (this.outputFile != null && this.outputFile.canRead()){
			this.outputFile.delete();
		}

		if (outputFile != null && outputFile.canRead()) {
			outputFile.delete();
		}
	}
}
