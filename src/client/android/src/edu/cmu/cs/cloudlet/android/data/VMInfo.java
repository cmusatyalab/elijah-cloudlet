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
import java.util.Set;
import java.util.TreeMap;

import org.json.JSONException;
import org.json.JSONObject;
import org.msgpack.MessagePack;
import org.msgpack.type.Value;
import org.msgpack.unpacker.BufferUnpacker;

import android.annotation.SuppressLint;
import android.util.Log;
import edu.cmu.cs.cloudlet.android.util.KLog;
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

	// legacy for synthesis using http - start
	public static final String JSON_KEY_BASE_NAME = "base_name";
	public static final String JSON_KEY_NAME = "overlay_name";
	public static final String JSON_KEY_TYPE = "type";
	public static final String JSON_KEY_UUID = "uuid";
	public static final String JSON_KEY_DISKIMAGE_PATH = "diskimg_path";
	public static final String JSON_KEY_DISKIMAGE_SIZE = "diskimg_size";
	public static final String JSON_KEY_MEMORYSNAPSHOT_PATH = "memory_snapshot_path";
	public static final String JSON_KEY_MEMORYSNAPSHOT_SIZE = "memory_snapshot_size";
	public static final String JSON_KEY_VERSION = "version";
	public static final String JSON_KEY_ERROR = "Error";
	public static final String JSON_KEY_CLOUDLET_CPU_CLOCK = "CPU-Clock";
	public static final String JSON_KEY_CLOUDLET_CPU_CORE = "CPU-Core";
	public static final String JSON_KEY_CLOUDLET_MEMORY_SIZE = "Memory-Size";
	public static final String JSON_KEY_LAUNCH_VM_IP = "LaunchVM-IP";
	public static final String JSON_VALUE_VM_TYPE_BASE = "baseVM";
	public static final String JSON_VALUE_VM_TYPE_OVERLAY = "overlay";
	// legacy for synthesis using http - end

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
}