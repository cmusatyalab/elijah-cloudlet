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
import java.util.Iterator;
import java.util.Map;
import java.util.TreeMap;

import org.json.JSONObject;
import org.msgpack.MessagePack;
import org.msgpack.type.Value;
import org.msgpack.unpacker.BufferUnpacker;

import edu.cmu.cs.cloudlet.android.util.MessagePackUtils;

public class VMInfo {
	protected TreeMap<String, Value> data = new TreeMap<String, Value>();

	private String appName = "No application name";
	private File overlayMetaFile;

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
	}
	
	private void unpackMessagePack() throws IOException{
		MessagePack msgpack = new MessagePack();
		byte[] a = null;
		a = MessagePackUtils.readData(this.overlayMetaFile);
		BufferUnpacker au = msgpack.createBufferUnpacker().wrap(a);
		Map metaMap = au.readValue().asMapValue();
		Iterator keyIterator = metaMap.keySet().iterator();

		while (keyIterator.hasNext()) {
			Value key = (Value) keyIterator.next();
			Value value = (Value) keyIterator.next();
			this.data.put(key.toString(), value);
		}
	}
	
	public File getMetaFile(){
		return this.overlayMetaFile;
	}

	public String getAppName() {
		return this.appName;
	}

	public JSONObject toJSON() {
		JSONObject object = new JSONObject(this.data);
		return object;
	}

	public String getInfo(String key) throws IOException {
		if(this.data == null){
			this.unpackMessagePack();
		}
		Value value = this.data.get(key);
		return value.toString();
	}

	public File getOverlayFile(String overlayFileName) throws IOException {
		File dirPath = overlayMetaFile.getParentFile();
		File overlayFile = new File(dirPath + File.separator + overlayFileName);
		if (overlayFile.canRead() != true){
			throw new IOException("Cannot find overlay file : " + overlayFile.getCanonicalPath());
		}
		return overlayFile;
	}
}