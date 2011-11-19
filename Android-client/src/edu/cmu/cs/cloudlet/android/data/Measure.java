package edu.cmu.cs.cloudlet.android.data;

import java.util.Vector;

import android.util.Log;


public class Measure {
	private static final String SEPERATOR = ":";
	public static final String NET_REQ_VMLIST = "REQ_VMLIST";
	public static final String NET_REQ_OVERLAY_TRASFER = "REQ_OVERLAY";
	public static final String NET_ACK_VMLIST = "ACK_VMLIST";
	public static final String NET_ACK_OVERLAY_TRASFER = "ACK_OVERLAY";
	public static final String OVERLAY_TRANSFER_START = "TRANSFER_START";
	public static final String OVERLAY_TRANSFER_END = "TRANSFER_END";
	public static final String VM_LAUNCHED = "VM_LAUNCHED";
	
	protected static Vector<String> timeline = new Vector<String>();
	protected static long imageSize = 0, memorySize = 0;
	
	public static void put(String timeString){
		timeline.add(timeString + SEPERATOR + (System.currentTimeMillis()/1000));
//		Log.d("krha", "Measurement : " + timeString + SEPERATOR + System.currentTimeMillis());
	}
	
	public static void setOverlaySize(long imgSize, long memSize){
		imageSize = imgSize;
		memorySize = memSize;
	}
	
	public static String print(){
		StringBuffer sb = new StringBuffer();
		for(int i = 0; i < timeline.size(); i++){
			sb.append(timeline.get(i) + "\n");
		}
		return sb.toString();
	}
	
	public static String printInfo(){
		if(timeline.size() <= 1)
			return "No diff";
		
		long overlayStart = 0, overlayEnd = 0;
		long startConnection = 0, endConnection = 0;
		StringBuffer sb = new StringBuffer();
		for(int i = 1; i < timeline.size(); i++){
			String[] prev = timeline.get(i-1).split(SEPERATOR);
			String[] curr = timeline.get(i).split(SEPERATOR);
			long timeDiff = Long.parseLong(curr[1]) - Long.parseLong(prev[1]);
			String msg = curr[0] + "\t-\t" + prev[0] + "\t= " + timeDiff + "\n"; 
//			sb.append(msg);
			
			if(curr[0].equals(Measure.OVERLAY_TRANSFER_START) == true){
				overlayStart = Long.parseLong(curr[1]);
			}else if(curr[0].equals(Measure.OVERLAY_TRANSFER_END) == true){
				overlayEnd = Long.parseLong(curr[1]);				
			}else if(curr[0].equals(Measure.NET_REQ_VMLIST) == true){
				startConnection = Long.parseLong(curr[1]);				
			}else if(curr[0].equals(Measure.VM_LAUNCHED) == true){
				endConnection = Long.parseLong(curr[1]);				
			}
		}
		startConnection = Long.parseLong(timeline.get(0).split(SEPERATOR)[1]);
		endConnection = Long.parseLong(timeline.get(timeline.size()-1).split(SEPERATOR)[1]);
		sb.append("-----------------------------------\n");
		sb.append("End-to-End : " + endConnection + "-" + startConnection +"=" + (endConnection-startConnection) + " s \n");
		sb.append("Overlay Trans : " + (overlayEnd-overlayStart) + " s \n");
		sb.append("Overlay Size: : " + (imageSize + memorySize)/1000/1000 + " MB \n");
		sb.append("Bandwidth : " + (imageSize+memorySize)/1000/(overlayEnd-overlayStart) + " MB/s\n");
		return sb.toString();
	}
	
}
