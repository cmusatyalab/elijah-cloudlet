package edu.cmu.cs.cloudlet.android.network;

import java.io.File;
import java.io.IOException;

import org.apache.http.HttpResponse;
import org.apache.http.client.ClientProtocolException;
import org.apache.http.client.HttpClient;
import org.apache.http.client.methods.HttpPost;
import org.apache.http.entity.mime.MultipartEntity;
import org.apache.http.entity.mime.content.StringBody;
import org.apache.http.impl.client.BasicResponseHandler;
import org.apache.http.impl.client.DefaultHttpClient;
import org.json.JSONException;
import org.json.JSONObject;

import edu.cmu.cs.cloudlet.android.CloudletActivity;
import edu.cmu.cs.cloudlet.android.R;
import edu.cmu.cs.cloudlet.android.util.KLog;

import android.app.Activity;
import android.app.ProgressDialog;
import android.content.Context;
import android.widget.Toast;

public class HTTPCommandSender extends Thread {
	protected ProgressDialog mDialog;
	
	private Activity activity = null;
	protected Context context;
	private String command = null;
	private String applicationName = null;

	public HTTPCommandSender(Activity activity, Context context, String command, String application) {
		this.activity = activity;
		this.context = context;
		this.command = command;
		this.applicationName = application;
	}

	public void run() {
		final String httpURL = CloudletActivity.TEST_CLOUDLET_SERVER_IP + File.separator + "isr_run";
		activity.runOnUiThread(new Runnable() {
			public void run() {
				mDialog = ProgressDialog.show(context, "Info", "Connecting to " + httpURL + "\nwaiting for " + applicationName + " to run at " + command, true);		
				mDialog.show();
			}
		});
		while(true){
			try {
				Thread.sleep(1000);
			} catch (InterruptedException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
		}

//		String ret = httpCommand(httpURL, this.command, this.applicationName);
//		if(mDialog != null){
//			mDialog.dismiss();			
//		}
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
			entity.addPart("metadata", new StringBody(json.toString()));
			httppost.setEntity(entity);

			// Execute HTTP Post Request
			HttpResponse response = httpclient.execute(httppost);
			BasicResponseHandler myHandler = new BasicResponseHandler();
			String endResult = myHandler.handleResponse(response);
			KLog.println("Get Reponse from Server : " + endResult);
			return endResult;
		} catch (ClientProtocolException e) {
			KLog.printErr(e.toString());
		} catch (IOException e) {
			KLog.printErr(e.toString());
		}
		return null;
	}
}
