package edu.cmu.cs.cloudlet.android.util;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.util.Iterator;
import java.util.Map;

import org.msgpack.MessagePack;
import org.msgpack.type.Value;
import org.msgpack.unpacker.BufferUnpacker;

import android.util.Log;
import edu.cmu.cs.cloudlet.android.data.VMInfo;

public class MessagePackUtils {

	public static byte[] readData(File path) throws IOException {
        ByteArrayOutputStream bo = new ByteArrayOutputStream();
        FileInputStream input = new FileInputStream(path);
        byte[] buffer = new byte[32*1024];
        while(true) {
            int count = input.read(buffer);
            if(count < 0) {
                break;
            }
            bo.write(buffer, 0, count);
        }
        return bo.toByteArray();
    }

	
	public static boolean isValidOverlayMeta(File file){
		if(file.isFile() != true){
			return false;
		}
		
		boolean hasSha256Value = false;
		boolean hasResumedDiskSize = false;
		boolean hasResumedMemorySize = false;
		boolean hasOverlayFileNames = false;
		
		MessagePack msgpack = new MessagePack();
		byte[] a = null;
		try{
			a = MessagePackUtils.readData(file);
			BufferUnpacker au = msgpack.createBufferUnpacker().wrap(a);
			Map metaMap = au.readValue().asMapValue();
			Iterator keyIterator = metaMap.keySet().iterator();
			while(keyIterator.hasNext()){
				String key = ((Value) keyIterator.next()).toString().replace("\"", "");
				Log.v("krha", "(" + key + ")");
				if(key.equals(VMInfo.META_BASE_VM_SHA256)){
					hasSha256Value = true;
				}else if(key.equals(VMInfo.META_RESUME_VM_DISK_SIZE)){
					hasResumedDiskSize = true;
				}else if(key.equals(VMInfo.META_RESUME_VM_MEMORY_SIZE)){
					hasResumedMemorySize = true;
				}else if(key.equals(VMInfo.META_OVERLAY_FILES)){
					hasOverlayFileNames = true;
					metaMap.get(key);
				}
			}
		} catch(IOException e){
			return false;
		}

		if(hasSha256Value && hasResumedDiskSize && hasResumedMemorySize && hasOverlayFileNames){
			return true;
		}
		return false;
			
	}

}
