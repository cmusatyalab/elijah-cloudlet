package edu.cmu.cs.cloudlet.android.network;

import java.io.BufferedInputStream;
import java.io.BufferedReader;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.IOException;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.net.UnknownHostException;
import java.text.NumberFormat;
import java.util.ArrayList;
import java.util.Vector;

import edu.cmu.cs.cloudlet.android.data.Measure;
import edu.cmu.cs.cloudlet.android.data.VMInfo;
import edu.cmu.cs.cloudlet.android.util.KLog;

import android.os.Bundle;
import android.os.Handler;
import android.os.Message;

public class NetworkClientSender extends Thread {
	private boolean isThreadRun = true;
	private Handler mHandler;

	private String Server_ipAddress;
	private int Server_port;

	private Vector<NetworkMsg> commandQueue = new Vector<NetworkMsg>(); // thread

	private DataOutputStream networkWriter;

	private byte[] imageSendingBuffer = new byte[3 * 1024 * 1024];
	private CloudletConnector connector;

	public NetworkClientSender(DataOutputStream networkWriter, Handler handler) {
		this.networkWriter = networkWriter;
		this.mHandler = handler;
	}

	// this is only for sending status of VM transfer.
	// I know this is bad approach, but make my life easier.
	// Do not use this method for other usage.
	public void setConnector(CloudletConnector connector) {
		this.connector = connector;
	}

	public void run() {

		while (isThreadRun == true) {
			if (commandQueue.size() == 0) {
				try {
					Thread.sleep(1);
				} catch (InterruptedException e) {
					e.printStackTrace();
				}
				continue;
			}

			NetworkMsg networkCommand = commandQueue.remove(0);
			try {
				this.sendCommand(this.networkWriter, networkCommand);
				switch (networkCommand.getCommandType()) {
				case NetworkMsg.MESSAGE_COMMAND_SEND_META:
					// Send overlay meta file
					File metaFile = networkCommand.getSelectedFile();
					Measure.record(Measure.OVERLAY_TRANSFER_START);
					this.sendBinaryFile(metaFile, false);
					break;
				case NetworkMsg.MESSAGE_COMMAND_SEND_OVERLAY:
					// Send overlay file
					File overlayFile = networkCommand.getSelectedFile();
					this.sendBinaryFile(overlayFile, true);
					this.connector.updateTransferredOverlay(overlayFile);
					break;
				}
			} catch (IOException e) {
				KLog.printErr(e.getMessage());
			}
		}
	}

	public void sendCommand(DataOutputStream writer, NetworkMsg msg) throws IOException {
		byte[] byteMsg = msg.toNetworkByte();
		if (byteMsg != null) {
			writer.write(byteMsg);
			writer.flush(); // flush everytime for accurate time
							// measure
		}
		// KLog.println("Send Message " + msg);
	}

	private void sendBinaryFile(File image, boolean notify) throws IOException {
		int sendByte = -1, totalByte = 0;
		// sending disk image
		BufferedInputStream bi = new BufferedInputStream(new FileInputStream(image));
		long imageLength = image.length();
		long startTime = System.currentTimeMillis();
		String duration = "";
		String statusMsg = "";
		NumberFormat format = NumberFormat.getInstance();
		format.setMaximumFractionDigits(2);
		format.setMinimumFractionDigits(2);
		while ((sendByte = bi.read(imageSendingBuffer, 0, imageSendingBuffer.length)) > 0) {
			networkWriter.write(imageSendingBuffer, 0, sendByte);
			totalByte += sendByte;
			if (notify) {
				duration = format.format((System.currentTimeMillis() - startTime) / 1000.0);
				statusMsg = "Overlay(" + format.format(imageLength/1024.0/1024) + " MB) - ";
//						+ duration + " s";
				this.notifyMessage(statusMsg);
				this.notifyTransferStatus((int) (100.0 * totalByte / imageLength));
			}
		}
		bi.close();
		networkWriter.flush();

	}

	private void notifyTransferStatus(final int percent) {
		if (this.connector != null) {
			mHandler.post(new Runnable() {
				@Override
				public void run() {
					connector.updateStatus(percent);
				}
			});
		}
	}

	private void notifyMessage(final String message) {
		if (this.connector != null) {
			mHandler.post(new Runnable() {
				@Override
				public void run() {
					connector.updateMessage(message);
				}
			});
		}
	}

	public void requestCommand(NetworkMsg command) {
		this.commandQueue.add(command);
	}

	public void close() {
		this.isThreadRun = false;
	}

}
