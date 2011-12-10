package edu.cmu.cs.cloudlet.android.network;

import java.io.File;
import java.io.IOException;

import org.apache.http.HttpResponse;
import org.apache.http.client.ClientProtocolException;
import org.apache.http.client.HttpClient;
import org.apache.http.client.methods.HttpPost;
import org.apache.http.entity.mime.MultipartEntity;
import org.apache.http.entity.mime.content.ByteArrayBody;
import org.apache.http.entity.mime.content.StringBody;
import org.apache.http.impl.client.BasicResponseHandler;
import org.apache.http.impl.client.DefaultHttpClient;
import org.json.JSONException;
import org.json.JSONObject;

import edu.cmu.cs.cloudlet.android.CloudletActivity;
import edu.cmu.cs.cloudlet.android.R;
import edu.cmu.cs.cloudlet.android.upnp.DeviceDisplay;
import edu.cmu.cs.cloudlet.android.upnp.UPnPDiscovery;
import edu.cmu.cs.cloudlet.android.util.KLog;

import android.app.Activity;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.Intent;
import android.os.Handler;
import android.os.Message;
import android.widget.Toast;

public class HTTPCommandSender extends Thread {
	protected static final int SUCCESS = 0;
	protected static final int FAIL = 0;
	
	protected ProgressDialog mDialog;
	protected String httpURL = "";
	
	private CloudletActivity activity = null;
	protected Context context;
	private String command = null;
	private String applicationName = null;

	public HTTPCommandSender(CloudletActivity activity, Context context, String command, String application) {
		this.activity = activity;
		this.context = context;
		this.command = command;
		this.applicationName = application;
	}

	public void initSetup(String url) {
		httpURL = "http://" + CloudletActivity.TEST_CLOUDLET_SERVER_IP + ":" + CloudletActivity.TEST_CLOUDLET_SERVER_PORT_ISR + "/" + url;
		mDialog = ProgressDialog.show(context, "Info", "Connecting to " + httpURL + "\nwaiting for " + applicationName + " to run at " + command, true);		
		mDialog.show();
	}
	
	Handler networkHandler = new Handler() {
		public void handleMessage(Message msg) {			
			if (msg.what == HTTPCommandSender.SUCCESS) {
				String applicationName = (String)msg.obj;
				activity.runApplication(applicationName);
			}else if(msg.what == HTTPCommandSender.FAIL){
				String ret = (String)msg.obj;
				activity.showAlert("Error", "Failed to connect to " + httpURL + "\n" + ret);
			}
		}
	};

	public void run() {
		String ret = httpCommand(httpURL, this.command, this.applicationName);
		if(ret.indexOf("SUCCESS") != -1){
			if(ret != null && ret.equalsIgnoreCase("SUCCESS")){
				Message message = Message.obtain();
				message.what = HTTPCommandSender.SUCCESS;
				message.obj = this.applicationName;
				networkHandler.sendMessage(message);
			}			
		}else{
			Message message = Message.obtain();
			message.what = HTTPCommandSender.FAIL;
			message.obj = ret;
			networkHandler.sendMessage(message);
		}
		
		
		if(mDialog != null){
			mDialog.dismiss();	
		}
	}

	private String httpCommand(String httpURL, String runType, String applicationName) {
		// Create a new HttpClient and Post Header		
		HttpClient httpclient = new DefaultHttpClient();
		HttpPost httppost = new HttpPost(httpURL);

		try {
			// Set HTTP Parameter as JSON and Image Binary
			JSONObject json = new JSONObject();
			try {
				json.put("application", applicationName);
				json.put("run-type", runType);
			} catch (JSONException e) {
				KLog.printErr(e.toString());
			}

			KLog.println("connecting to " + httpURL);			
			MultipartEntity entity = new MultipartEntity();
			entity.addPart("info", new StringBody(json.toString()));
			httppost.setEntity(entity);			

			// Execute HTTP Post Request
			HttpResponse response = httpclient.execute(httppost);
			BasicResponseHandler myHandler = new BasicResponseHandler();
			String endResult = myHandler.handleResponse(response);
			
			return endResult;			
		} catch (ClientProtocolException e) {
			KLog.printErr(e.toString());
			return e.toString();
		} catch (IOException e) {
			KLog.printErr(e.toString());
			return e.toString();
		} catch (IllegalStateException e){
			KLog.printErr(e.toString());
			return e.toString();			
		}
	}
}
