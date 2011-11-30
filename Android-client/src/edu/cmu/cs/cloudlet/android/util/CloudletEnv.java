package edu.cmu.cs.cloudlet.android.util;

import java.io.File;
import java.util.ArrayList;

import edu.cmu.cs.cloudlet.android.data.VMInfo;

import android.os.Environment;
import android.util.Log;

public class CloudletEnv {
	public static final int INSTALLATION = 1;
	public static final int PREFERENCE = 2;
	public static final int SOCKET_MOCK_INPUT = 3;
	public static final int SOCKET_MOCK_OUTPUT = 4;
	public static final int ROOT_DIR = 5;
	
	protected File SD_ROOT = Environment.getExternalStorageDirectory();
	
	private String env_root 			= "Cloudlet";
	private String overlay_dir 			= "overlay";
    private String installation_file	= ".installed";
    private String preference_file		= ".preference";
    private String socket_mock_file 	= ".mock_output";    
	
	protected static CloudletEnv env = null;
	protected ArrayList<VMInfo> overlayVMList = null;
	protected String VM_Type;
	 
	
	public static CloudletEnv instance(){
		if(env == null){
			env = new CloudletEnv();
		}
		return env;
	}
	
	protected CloudletEnv(){
		SD_ROOT = Environment.getExternalStorageDirectory();
		File env_dir = new File(SD_ROOT + File.separator + env_root);
		File overay_dir = new File(env_dir.getAbsolutePath() + File.separator + overlay_dir);
		if(env_dir.exists() == false){
			// create cloudlet root directory
			if(env_dir.mkdir() == false){
				Log.e("krha", "Cannot create Folder");
			}
			
			// create overlay directory
			if(overay_dir.mkdir() == false){
				Log.e("krha", "Cannot create Folder");				
			}
		}
	}
	
	public File getFilePath(int id){
		String path = "";
		switch(id){
		case CloudletEnv.ROOT_DIR:
			path = "";
		case CloudletEnv.INSTALLATION:
			path = this.installation_file;
		case CloudletEnv.PREFERENCE:
			path = this.preference_file;
		case CloudletEnv.SOCKET_MOCK_INPUT:
			path = this.socket_mock_file;
		case CloudletEnv.SOCKET_MOCK_OUTPUT:
			path = this.socket_mock_file;
		}
		
		return new File(SD_ROOT + File.separator + env_root + File.separator + path);
	}

	public ArrayList<VMInfo> getOverlayDirectoryInfo() {
		if(overlayVMList != null && overlayVMList.size() > 0){
			return this.overlayVMList;
		}
		overlayVMList = new ArrayList<VMInfo>();
		
		// Get information From overlay directory
		File env_dir = new File(SD_ROOT + File.separator + env_root);
		File overay_root = new File(env_dir.getAbsolutePath() + File.separator + overlay_dir);
		File[] VMDirs = overay_root.listFiles();
		
		// Enumerate multiple VMs
		for(int i = 0; i < VMDirs.length; i++){
			File VMDir = VMDirs[i];
			File[] overlaydir = VMDir.listFiles();
			// Enumerate multiple Version of Overlay
			for(int j = 0; j < overlaydir.length; j++){
				File overlay = overlaydir[j];
				VMInfo newVM = new VMInfo(overlay, VMDir.getName());
				this.overlayVMList.add(newVM);
			}
		}
		
		return this.overlayVMList;
	}

	public void resetOverlayList() {
		if(overlayVMList != null){
			overlayVMList.clear();
			overlayVMList = null;
		}		
	}

}
