package edu.cmu.cs.cloudlet.android.discovery;

import java.io.IOException;
import org.apache.http.HttpResponse;
import org.apache.http.client.ClientProtocolException;
import org.apache.http.client.HttpClient;
import org.apache.http.client.methods.HttpGet;
import org.apache.http.impl.client.BasicResponseHandler;
import org.apache.http.impl.client.DefaultHttpClient;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import android.app.Activity;
import android.widget.ArrayAdapter;
import edu.cmu.cs.cloudlet.android.CloudletActivity;
import edu.cmu.cs.cloudlet.android.util.KLog;

public class CloudletDirectoryClient extends Thread {
	public static String GLOBAL_DISCOVERY_SERVER = "http://register.findcloudlet.org/api/v1/Cloudlet/?format=json&status=RUN"; //specify URL if you want to find Cloudlet from global search

	private Activity activity;
	private ArrayAdapter<CloudletDevice> listAdapter;;

	public CloudletDirectoryClient(Activity activity, ArrayAdapter<CloudletDevice> listAdapter) {
		this.activity = activity;
		this.listAdapter = listAdapter;
	}

	@Override
	public void run() {
		if (CloudletDirectoryClient.GLOBAL_DISCOVERY_SERVER.startsWith("http") == false)
			return;
		
		String retString = httpGet(CloudletDirectoryClient.GLOBAL_DISCOVERY_SERVER);
		if (retString != null) {
			try {
				JSONObject jsonObject = new JSONObject(retString);
				JSONArray cloudletArray = jsonObject.getJSONArray("objects");
				for (int i = 0; i < cloudletArray.length(); i++) {
					JSONObject cloudletObject = cloudletArray.getJSONObject(i);
					this.deviceAdded(cloudletObject);
				}
			} catch (JSONException e) {
				KLog.printErr("Error parsing data " + e.toString());
			}
		}

	}

	private String httpGet(String httpURL) {
		HttpClient httpclient = new DefaultHttpClient();
		HttpGet httpGet = new HttpGet(httpURL);
		String endResult = null;
		try {
			HttpResponse response = httpclient.execute(httpGet);
			BasicResponseHandler myHandler = new BasicResponseHandler();
			endResult = myHandler.handleResponse(response);
		} catch (ClientProtocolException e) {
			e.printStackTrace();
		} catch (IOException e) {
			e.printStackTrace();
		}
		return endResult;
	}

	public void deviceAdded(final JSONObject cloudletObject) {
		activity.runOnUiThread(new Runnable() {
			public void run() {
				CloudletDevice device = new CloudletDevice(cloudletObject);
				int position = listAdapter.getPosition(device);
				if (position >= 0) {
					// Device already in the list, re-set new value at same
					// position
					listAdapter.remove(device);
					listAdapter.insert(device, position);
				} else {
					listAdapter.add(device);
				}
			}
		});
	}

	public void close() {
	}
}
