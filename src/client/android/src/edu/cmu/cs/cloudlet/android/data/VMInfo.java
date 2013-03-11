//
// Copyright (C) 2011-2012 Carnegie Mellon University
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of version 2 of the GNU General Public License as published
// by the Free Software Foundation.  A copy of the GNU General Public License
// should have been distributed along with this program in the file
// LICENSE.GPL.

// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
// or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
// for more details.
//
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
import org.msgpack.type.Value;
import org.msgpack.unpacker.BufferUnpacker;

import edu.cmu.cs.cloudlet.android.network.CloudletConnector;
import edu.cmu.cs.cloudlet.android.util.MessagePackUtils;

public class VMInfo {
	protected HashMap<String, Object> data = new HashMap<String, Object>();

	private String appName = "No application name";
	private File overlayMetaFile;

	private Vector<File> leftOverlayList = new Vector<File>();

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
		byte[] byteArray = null;
		byteArray = MessagePackUtils.readData(this.overlayMetaFile);
		this.data = CloudletConnector.convertMessagePackToMap(byteArray);
		
		ArrayValue overlayFileArray = (ArrayValue) this.data.get(META_OVERLAY_FILES);
		Iterator<Value> overlayIter = overlayFileArray.iterator();
		File dirPath = overlayMetaFile.getParentFile();
		while(overlayIter.hasNext()){
			String overlayFileName = overlayIter.next().asRawValue().toString();
			File overlayFile = new File(dirPath + File.separator + overlayFileName);
			if (overlayFile.canRead() != true){
				throw new IOException("Cannot find overlay file : " + overlayFile.getCanonicalPath());
			}
			this.leftOverlayList.add(overlayFile);
		}
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

	public boolean transferFinish(){
		if(this.leftOverlayList.size() == 0){
			return true;
		}
		return false;
	}
	
	public void addTransferredOverlay(File overlayFile) {
		this.leftOverlayList.removeElement(overlayFile);
	}
}