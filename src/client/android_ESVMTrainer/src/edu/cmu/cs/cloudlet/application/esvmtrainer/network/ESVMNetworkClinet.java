package edu.cmu.cs.cloudlet.application.esvmtrainer.network;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.UnsupportedEncodingException;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.net.UnknownHostException;

import org.apache.http.HttpEntity;
import org.apache.http.HttpResponse;
import org.apache.http.client.ClientProtocolException;
import org.apache.http.client.HttpClient;
import org.apache.http.client.methods.HttpGet;
import org.apache.http.client.methods.HttpPost;
import org.apache.http.conn.ConnectTimeoutException;
import org.apache.http.entity.StringEntity;
import org.apache.http.entity.mime.HttpMultipartMode;
import org.apache.http.entity.mime.MultipartEntity;
import org.apache.http.entity.mime.content.FileBody;
import org.apache.http.entity.mime.content.InputStreamBody;
import org.apache.http.entity.mime.content.StringBody;
import org.apache.http.impl.client.BasicResponseHandler;
import org.apache.http.impl.client.DefaultHttpClient;
import org.apache.http.message.BasicHeader;
import org.apache.http.protocol.HTTP;
import org.json.JSONException;
import org.json.JSONObject;

import edu.cmu.cs.cloudlet.application.esvmtrainer.util.ZipUtility;

import android.os.Environment;
import android.os.Handler;
import android.os.Message;
import android.util.Log;

public class ESVMNetworkClinet extends Thread {
	public static final int CALLBACK_FAILED = 0;
	public static final int CALLBACK_SUCCESS = 1;

	private JSONObject header = null;
	private File[] imageFiles;
	private String serverURL = null;

	private Handler networkCallbackHandler = null;
	private Socket clientSocket = null;
	private DataOutputStream networkWriter = null;
	private DataInputStream networkReader = null;
	private File outputFile;

	public ESVMNetworkClinet(Handler handler, JSONObject header, File[] imageFiles, String serverURL) {
		this.networkCallbackHandler = handler;
		this.header = header;
		this.imageFiles = imageFiles;
		this.serverURL = serverURL;
	}

	public void run() {
		// Zip target files
		/*
		 * File outputFile = null; try { this.outputFile =
		 * File.createTempFile("ESVMImages", ".zip",
		 * Environment.getExternalStorageDirectory());
		 * ZipUtility.zipFiles(this.imageFiles, outputFile); } catch
		 * (IOException e) { this.close();
		 * this.handleFailure("Cannot Create Zip file at " +
		 * outputFile.getAbsolutePath()); return; }
		 */

		// HTTP post
		boolean ret = false;
		try {
			ret = this.sendHTTPPost(this.serverURL, this.header, this.imageFiles[0]);			
		} catch (ClientProtocolException e) {
			Log.e("krha", e + "");
			this.close();
			this.handleFailure("Cannot send POST message to : " + this.serverURL);
			return;
		} catch (IOException e) {
			Log.e("krha", e.getMessage());
			this.close();
			this.handleFailure("Cannot send POST message to : " + this.serverURL);
			return;
		} catch (JSONException e) {
			Log.e("krha", e.getMessage());
			this.close();
			this.handleFailure("Not valid JSON return");
			return;
		}

		// clean-up
		this.close();
		if (ret == true){
			this.handleSuccess("SUCCESS");
		}else{
			this.handleFailure("Return Failed : " + this.serverURL);				
		}
	}

	public static void connect(String url) {
		HttpClient httpclient = new DefaultHttpClient();
		HttpGet httpget = new HttpGet(url);

		// Execute the request
		HttpResponse response;
		try {
			response = httpclient.execute(httpget);
			HttpEntity entity = response.getEntity();

			if (entity != null) {
				InputStream instream = entity.getContent();
				instream.close();
			}

		} catch (Exception e) {
		}
	}

	private boolean sendHTTPPost(String httpURL, JSONObject jsonHeader, File zipFile) throws ClientProtocolException,
			IOException, ConnectTimeoutException, JSONException {

		// Create a new HttpClient and Post Header
		HttpClient httpclient = new DefaultHttpClient();
		httpclient.getParams().setParameter("http.connection.timeout", 5000);
		httpclient.getParams().setParameter("http.socket.timeout", 5000);
		HttpPost httppost = new HttpPost(httpURL);
		MultipartEntity entity = new MultipartEntity(HttpMultipartMode.BROWSER_COMPATIBLE);

		entity.addPart("info", new StringBody(jsonHeader.toString()));
		entity.addPart("data", new FileBody(zipFile));
		httppost.setEntity(entity);

		// Execute HTTP Post Request
		HttpResponse response = httpclient.execute(httppost);
		BasicResponseHandler myHandler = new BasicResponseHandler();
		String endResult = myHandler.handleResponse(response);
		JSONObject jsonRet = new JSONObject(endResult);
		String retString = jsonRet.getString("return");
		if (retString.toLowerCase().equals("success")){
			return true;
		}else{
			return false;
		}
	}

	private void handleSuccess(String retMsg) {
		Message msg = Message.obtain();
		msg.what = ESVMNetworkClinet.CALLBACK_SUCCESS;
		msg.obj = retMsg;
		this.networkCallbackHandler.sendMessage(msg);
	}

	private void handleFailure(String reason) {
		Message msg = Message.obtain();
		msg.what = ESVMNetworkClinet.CALLBACK_FAILED;
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

		if (outputFile != null && outputFile.canRead()) {
			outputFile.delete();
		}
	}
}
