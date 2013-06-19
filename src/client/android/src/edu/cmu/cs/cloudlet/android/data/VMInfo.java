package edu.cmu.cs.cloudlet.android.data;

import java.io.File;
import java.io.IOException;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;
import java.util.TreeMap;
import java.util.Vector;

import org.json.JSONObject;
import org.msgpack.MessagePack;
import org.msgpack.type.ArrayValue;
import org.msgpack.type.MapValue;
import org.msgpack.type.Value;
import org.msgpack.type.ValueType;
import org.msgpack.unpacker.BufferUnpacker;

import edu.cmu.cs.cloudlet.android.network.CloudletConnector;
import edu.cmu.cs.cloudlet.android.util.KLog;
import edu.cmu.cs.cloudlet.android.util.MessagePackUtils;

public class VMInfo {
	protected HashMap<String, Object> data = new HashMap<String, Object>();

	private String appName = "No application name";
	private File overlayMetaFile;
	private Vector<File> sentOverlayFileList = new Vector<File>();

	private int totalOverlayFiles;

	public static final String META_BASE_VM_SHA256 = "base_vm_sha256";
	public static final String META_RESUME_VM_DISK_SIZE = "resumed_vm_disk_size";
	public static final String META_RESUME_VM_MEMORY_SIZE = "resumed_vm_memory_size";
	public static final String META_OVERLAY_FILES = "overlay_files";
	public static final String META_OVERLAY_FILE_NAME = "overlay_name";
	public static final String META_OVERLAY_FILE_SIZE = "overlay_size";
	public static final String META_OVERLAY_FILE_DISK_CHUNKS = "disk_chunk";
	public static final String META_OVERLAY_FILE_MEMORY_CHUNKS = "memory_chunk";

	public VMInfo(File overlayMetaFile) throws IOException {
	
		// get application name to display
		String[] temp = overlayMetaFile.getParent().split("/");
		String dirName = temp[temp.length-1];
		this.appName = dirName;
		this.overlayMetaFile = overlayMetaFile;
		this.unpackMessagePack();
	}	
		
	private void unpackMessagePack() throws IOException{
		MessagePack msgpack = new MessagePack();
		byte[] a = null;
		a = MessagePackUtils.readData(this.overlayMetaFile);
		BufferUnpacker unpacker = msgpack.createBufferUnpacker().wrap(a);		
		
		// optimized to get only overlay size 
	    int size = unpacker.readMapBegin();                                                                                                                                       
	    Map<String, Value> ret = new HashMap<String, Value>(size);                                                                                                                                  
	    for (int i = 0; i < size; ++i) {                                                                                                                                          
	        String key = unpacker.read(String.class);
	        if (key.equals(META_OVERLAY_FILES) == true){
	        	this.totalOverlayFiles = unpacker.readArrayBegin();
	        	break;
	        }else{
		        Value value = unpacker.read(Value.class);
	        }
	    }
	    unpacker.close();	     
	}
	
	public File getMetaFile(){
		return this.overlayMetaFile;
	}

	public String getAppName() {
		return this.appName;
	}

	public void setAppName(String appName2) {
		this.appName = appName2;		
	}

	public JSONObject toJSON() {
		JSONObject object = new JSONObject(this.data);
		return object;
	}

	public File getOverlayFile(String overlayFileName) throws IOException {
		File dirPath = overlayMetaFile.getParentFile();
		File overlayFile = new File(dirPath + File.separator + overlayFileName);
		if (overlayFile.canRead() != true){
			throw new IOException("Cannot find overlay file : " + overlayFile.getCanonicalPath());
		}
		return overlayFile;
	}

	public boolean transferFinish() {
		if(this.sentOverlayFileList.size() == this.totalOverlayFiles){
			return true;
		}
		return false;
	}
	
	public void addTransferredOverlay(File overlayFile) {
		// First remove it to avoid duplicated overlay file element
		this.sentOverlayFileList.removeElement(overlayFile);
		this.sentOverlayFileList.addElement(overlayFile);
	}
}
