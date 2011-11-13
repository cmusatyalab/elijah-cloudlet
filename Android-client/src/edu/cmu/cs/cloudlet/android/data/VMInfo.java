package edu.cmu.cs.cloudlet.android.data;

import java.io.File;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Set;
import java.util.TreeMap;

import org.json.JSONException;
import org.json.JSONObject;

import android.util.Log;

public class VMInfo {
	protected TreeMap<String, String> data = new TreeMap<String, String>();
	protected File rootDirectory = null;
	
	protected String KEY_NAME = "name";
	protected String KEY_TYPE = "type";
	protected String KEY_DISKIMAGE_NAME = "diskimg_name";
	protected String KEY_MEMORYSNAPSHOT_NAME = "memorysnapshot_name";
//	protected String uuid;
//	protected String version;
	
	public VMInfo(File overlayDirectory){
		rootDirectory = overlayDirectory;
		String name = rootDirectory.getName();
		File[] files = rootDirectory.listFiles();
		
		this.data.put(KEY_NAME, name);
		this.data.put(KEY_TYPE, "overlay");		
		for(int i = 0; i < files.length; i++){
			String filename = files[i].getName();
			
			if(filename.endsWith("mem") == true){
				this.data.put(KEY_MEMORYSNAPSHOT_NAME, filename);
			}else if(filename.endsWith("img") == true){
				this.data.put(KEY_DISKIMAGE_NAME, filename);
			}
		}
	}
	
	public JSONObject toJSON(){
		JSONObject object = new JSONObject(this.data);
		return object;
		/*
		Iterator<String> keySet = this.data.keySet().iterator();
		while(keySet.hasNext()){ 
			try {
				String key = keySet.next();
				object.put(key, this.data.get(key));
			} catch (JSONException e) {
				Log.e("krha", e.toString());
			}
		}
		*/
	}
	
}
