package edu.cmu.cs.cloudlet.android.util;

import java.io.File;
import java.io.FileFilter;
import java.io.IOException;
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
	public static final int SPEECH_LOG_DIR = 6;
	public static final int SPEECH_INPUT_DIR = 7;
	public static final int FACE_LOG_DIR = 8;
	public static final int FACE_INPUT_DIR = 9;
	public static final int OVERLAY_DIR = 10;

	protected File SD_ROOT = Environment.getExternalStorageDirectory();

	public final static String env_root = "Cloudlet";
	public final static String overlay_dir = "overlay";
	private String speech_log_dir = "SPEECH" + File.separator + "log";
	private String speech_input_dir = "SPEECH" + File.separator + "myrecordings";
	private String face_log_dir = "FACE" + File.separator + "log";
	private String face_input_dir = "FACE" + File.separator + "faceinput";
	private String installation_file = ".installed";
	private String preference_file = ".preference";
	private String socket_mock_file = "cloudlet-";

	protected static CloudletEnv env = null;
	protected ArrayList<VMInfo> overlayVMList = null;
	protected String VM_Type;
	
	private File overlayDir;
	private File envDir;

	public static CloudletEnv instance() {
		if (env == null) {
			env = new CloudletEnv();
		}
		return env;
	}

	protected CloudletEnv() {
		SD_ROOT = Environment.getExternalStorageDirectory();
		this.envDir = new File(SD_ROOT + File.separator + env_root);
		this.overlayDir = new File(envDir.getAbsolutePath() + File.separator + overlay_dir);
		if (envDir.exists() == false) {
			// create cloudlet root directory
			if (envDir.mkdir() == false) {
				Log.e("krha", "Cannot create Folder");
			}
			// create overlay directory
			if (overlayDir.mkdir() == false) {
				Log.e("krha", "Cannot create Folder");
			}
		}

		File speech_dir1 = new File(envDir.getAbsolutePath() + File.separator + speech_log_dir);
		File speech_dir2 = new File(envDir.getAbsolutePath() + File.separator + speech_input_dir);
		// create speech sub directory1
		if (speech_dir1.exists() == false) {
			if (speech_dir1.mkdirs() == false) {
				Log.e("krha", "Cannot create Folder");
			}
		}
		if (speech_dir2.exists() == false) {
			if (speech_dir2.mkdirs() == false) {
				Log.e("krha", "Cannot create Folder");
			}
		}

		// create face sub dirs
		File face_dir1 = new File(envDir.getAbsolutePath() + File.separator + face_log_dir);
		File face_dir2 = new File(envDir.getAbsolutePath() + File.separator + face_input_dir);
		if (face_dir1.exists() == false) {
			if (face_dir1.mkdirs() == false) {
				Log.e("krha", "Cannot create Folder" + face_dir1.getAbsolutePath());
			}
		}
		if (face_dir2.exists() == false) {
			if (face_dir2.mkdirs() == false) {
				Log.e("krha", "Cannot create Folder" + face_dir2.getAbsolutePath());
			}
		}

	}	

	public File getFilePath(int id) {
		String path = "";
		switch (id) {
		case CloudletEnv.ROOT_DIR:
			path = "";
			break;
		case CloudletEnv.INSTALLATION:
			path = this.installation_file;
			break;
		case CloudletEnv.PREFERENCE:
			path = this.preference_file;
			break;
		case CloudletEnv.SOCKET_MOCK_INPUT:
			path = this.socket_mock_file;
			break;
		case CloudletEnv.SOCKET_MOCK_OUTPUT:
			path = this.socket_mock_file;
			break;
		case CloudletEnv.SPEECH_INPUT_DIR:
			path = this.speech_input_dir;
			break;
		case CloudletEnv.SPEECH_LOG_DIR:
			path = this.speech_log_dir;
			break;
		case CloudletEnv.FACE_INPUT_DIR:
			path = this.face_input_dir;
			break;
		case CloudletEnv.FACE_LOG_DIR:
			path = this.face_log_dir;
			break;
		case CloudletEnv.OVERLAY_DIR:
			path = this.overlay_dir;
			break;
		}

		return new File(SD_ROOT + File.separator + env_root + File.separator + path);
	}

	public ArrayList<VMInfo> getOverlayDirectoryInfo() {
		if (overlayVMList != null && overlayVMList.size() > 0) {
			return this.overlayVMList;
		}
		overlayVMList = new ArrayList<VMInfo>();

		// Get information From overlay directory
		File env_dir = new File(SD_ROOT + File.separator + env_root);
		File ovelray_root = new File(env_dir.getAbsolutePath() + File.separator + overlay_dir);
		File[] overlayVMDirs = ovelray_root.listFiles();

		// Enumerate base VMs
		for (int i = 0; i < overlayVMDirs.length; i++) {
			File overlayDir = overlayVMDirs[i];
			File overlayMetaFile = findOverlayMetaFile(overlayDir);
			if (overlayMetaFile != null) {
				try {
					VMInfo newVM = new VMInfo(overlayMetaFile);
					this.overlayVMList.add(newVM);
				} catch (IOException e) {
					e.printStackTrace();
				}
			}
		}

		return this.overlayVMList;
	}

	private File findOverlayMetaFile(File overlayDir) {
		if (overlayDir.isDirectory() != true) {
			return null;
		}
		File[] candidateMetaFiles = overlayDir.listFiles(new FileFilter() {
			@Override
			public boolean accept(File pathname) {
				if (pathname.getName().endsWith("xz") == true) {
					return false;
				} else {
					return true;
				}
			}
		});

		// Unpacking messagepack take a long time. Do lazy checking when we
		// transfer overlay.
		/*
		 * for(File candiateFile : candidateMetaFiles){ long start_time =
		 * System.currentTimeMillis(); if(MessagePackUtils.gma(candiateFile) ==
		 * true){ Log.v("krha", "measuremed time : " +
		 * (System.currentTimeMillis() - start_time)); return candiateFile; } }
		 * return null;
		 */
		if (candidateMetaFiles.length == 1) {
			return candidateMetaFiles[0];
		} else if (candidateMetaFiles.length == 0) {
			KLog.printErr("Cannot find valid meta file.");
			return null;
		} else {
			KLog.printErr("Multiple overlay-meta files.");
			return null;
		}
	}

	public void resetOverlayList() {
		if (overlayVMList != null) {
			overlayVMList.clear();
			overlayVMList = null;
		}
	}

}
