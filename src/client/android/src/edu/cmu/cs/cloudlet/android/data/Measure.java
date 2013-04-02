package edu.cmu.cs.cloudlet.android.data;

import java.util.Vector;

public class Measure {
	private static final String SEPERATOR = ":";
	public static final String OVERLAY_TRANSFER_START = "OVERLAY_TRANSFER_START";
	public static final String OVERLAY_TRANSFER_END = "OVERLAY_TRANSFER_END";
	public static final String VM_LAUNCHED = "VM_LAUNCHED";
	public static final String APP_START = "APP_START";
	public static final String APP_END = "APP_END";

	protected static Vector<String> timeline;
	protected static long imageSize = 0, memorySize = 0;

	public static void clear() {
		timeline = new Vector<String>();
	}

	public static void record(String timeString) {
//		timeline.add(timeString + SEPERATOR
//				+ (System.currentTimeMillis() / 1000));
		// Log.d("krha", "Measurement : " + timeString + SEPERATOR +
		// System.currentTimeMillis());
	}

	public static void setOverlaySize(long imgSize, long memSize) {
		imageSize = imgSize;
		memorySize = memSize;
	}

	public static String print() {
		StringBuffer sb = new StringBuffer();
		for (int i = 0; i < timeline.size(); i++) {
			sb.append(timeline.get(i) + "\n");
		}
		return sb.toString();
	}
}
