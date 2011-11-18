package edu.cmu.cs.cloudlet.android.data;

import java.io.File;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Set;
import java.util.TreeMap;

import org.json.JSONException;
import org.json.JSONObject;

import edu.cmu.cs.cloudlet.android.util.KLog;

import android.util.Log;

public class VMInfo {
	protected TreeMap<String, String> data = new TreeMap<String, String>();
	protected File rootDirectory = null;
	
	public static final String JSON_KEY_NAME 					= "name";
	public static final String JSON_KEY_TYPE 					= "type";
	public static final String JSON_KEY_UUID 					= "uuid";
	public static final String JSON_KEY_DISKIMAGE_PATH 			= "diskimg_path";
	public static final String JSON_KEY_DISKIMAGE_SIZE 			= "diskimg_size";
	public static final String JSON_KEY_MEMORYSNAPSHOT_PATH 	= "memorysnapshot_path";
	public static final String JSON_KEY_MEMORYSNAPSHOT_SIZE 	= "memorysnapshot_size";
	public static final String JSON_KEY_VERSION					= "version";

	public static final String JSON_KEY_ERROR					= "Error";
	
	public static final String JSON_KEY_CLOUDLET_CPU_CLOCK		= "CPU-Clock";
	public static final String JSON_KEY_CLOUDLET_CPU_CORE		= "CPU-Core";
	public static final String JSON_KEY_CLOUDLET_MEMORY_SIZE	= "Memory-Size";

	public static final String JSON_KEY_LAUNCH_VM_IP 			= "LaunchVM-IP";

	public static final String JSON_VALUE_VM_TYPE_BASE			= "baseVM";
	public static final String JSON_VALUE_VM_TYPE_OVERLAY		= "overlay";

//	protected String uuid;
//	protected String version;
	
	
	public VMInfo(File overlayDirectory, String vmName){
		rootDirectory = overlayDirectory;
		File[] files = rootDirectory.listFiles();
		
		this.data.put(JSON_KEY_NAME, vmName);
		this.data.put(JSON_KEY_TYPE, "overlay");
		for(int i = 0; i < files.length; i++){
			String filename = files[i].getAbsolutePath();
			if(filename.endsWith("mem") == true){
				this.data.put(JSON_KEY_MEMORYSNAPSHOT_PATH, filename);
				this.data.put(JSON_KEY_MEMORYSNAPSHOT_SIZE, "" + files[i].length());
			}else if(filename.endsWith("img") == true){
				this.data.put(JSON_KEY_DISKIMAGE_PATH, filename);
				this.data.put(JSON_KEY_DISKIMAGE_SIZE, "" + files[i].length());
			}
		}
	}

	public VMInfo(JSONObject jsonVM) throws JSONException {
		Iterator keys = jsonVM.keys();
		while(keys.hasNext()){
			String key = (String) keys.next();
			Object value = jsonVM.get(key);
			if(value instanceof String){
				String value_string = (String)value;
				this.data.put(key, value_string);
			}else{
				KLog.printErr("json value is not String type : " + key);
			}
		}
	}

	public JSONObject toJSON(){
		JSONObject object = new JSONObject(this.data);
		return object;
	}
	
	public String getInfo(String key){
		return this.data.get(key);		
	}
}
