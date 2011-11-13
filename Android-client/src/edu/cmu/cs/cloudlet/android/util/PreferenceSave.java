package edu.cmu.cs.cloudlet.android.util;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.RandomAccessFile;
import java.util.UUID;

import android.os.Environment;
import android.util.Log;

public class PreferenceSave {
    private static String serverURL = null;

    public synchronized static String serverURL(String defaultURL) {
        if (serverURL == null) {  
            File installation = CloudletEnv.instance().getFilePath(CloudletEnv.PREFERENCE);
            try {
                if (!installation.exists())
                    writepreferenceFile(installation, defaultURL);
                serverURL = readPreferenceFile(installation);
            } catch (Exception e) {
                throw new RuntimeException(e);
            }
        }
        return serverURL;
    }

    private static String readPreferenceFile(File installation) throws IOException {
        RandomAccessFile f = new RandomAccessFile(installation, "r");
        byte[] bytes = new byte[(int) f.length()];
        f.readFully(bytes);
        f.close();
        return new String(bytes);
    }

    private static void writepreferenceFile(File installation, String url) throws IOException {
        FileOutputStream out = new FileOutputStream(installation);
        out.write(url.getBytes());
        out.close();
        Log.d("krha", "Saving URL to File : " + url);
    }
    
    public static void updateServerURL(String url){
    	serverURL = url;
    	File installation = CloudletEnv.instance().getFilePath(CloudletEnv.PREFERENCE);
    	try {
			PreferenceSave.writepreferenceFile(installation, url);
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}    	
    }
}
