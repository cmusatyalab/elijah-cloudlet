package edu.cmu.cs.cloudlet.android.lib.discovery;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.EOFException;
import java.io.IOException;
import java.math.BigInteger;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.net.UnknownHostException;
import java.util.HashMap;
import java.util.Map;

import org.apache.http.util.ByteArrayBuffer;
import org.msgpack.MessagePack;
import org.msgpack.type.Value;
import org.msgpack.type.ValueType;
import org.msgpack.unpacker.BufferUnpacker;

import edu.cmu.cs.cloudlet.android.network.NetworkClientReceiver;
import edu.cmu.cs.cloudlet.android.network.NetworkClientSender;
import edu.cmu.cs.cloudlet.android.network.NetworkMsg;
import edu.cmu.cs.cloudlet.android.util.KLog;

public class CloudletDiscoveryAPI {
	public static final BigInteger INVALID_SESSION_ID = new BigInteger("0");
	public static String errMsg = "";

	private static Socket connect(String ipAddress, int port) {
		try {
			Socket socket = new Socket();
			socket.connect(new InetSocketAddress(ipAddress, port), 10 * 1000);
			return socket;
		} catch (UnknownHostException e) {
			errMsg = e.getLocalizedMessage();
			return null;
		} catch (IOException e) {
			errMsg = e.getLocalizedMessage();
			return null;
		}
	}

	private static void sendCommand(DataOutputStream writer, NetworkMsg msg) throws IOException {
		byte[] byteMsg = msg.toNetworkByte();
		if (byteMsg != null) {
			writer.write(byteMsg);
			writer.flush();
		}
	}

	public static ByteArrayBuffer receiveMsg(DataInputStream reader) throws EOFException, IOException {
		int messageLength = reader.readInt();
		if (messageLength == -1) {
			return null;
		}
		byte[] msgpackByte = new byte[messageLength];
		reader.read(msgpackByte, 0, msgpackByte.length);

		ByteArrayBuffer msgPackBuffer = new ByteArrayBuffer(messageLength);
		msgPackBuffer.append(msgpackByte, 0, msgpackByte.length);

		return msgPackBuffer;
	}

	private static HashMap convertMessagePackToMap(byte[] byteArray) throws IOException {
		HashMap<String, Object> messageMap = new HashMap<String, Object>();
		MessagePack msgpack = new MessagePack();
		BufferUnpacker au = msgpack.createBufferUnpacker().wrap(byteArray);
		Map<Value, Value> metaMap = au.readValue().asMapValue();
		for (Map.Entry<Value, Value> e : ((Map<Value, Value>) metaMap).entrySet()) {
			ValueType valueType = e.getValue().getType();
			String key = e.getKey().toString().replace("\"", "");
			switch (valueType) {
			case BOOLEAN:
				messageMap.put(key, e.getValue().asBooleanValue().getBoolean());
				break;
			case INTEGER:
				messageMap.put(key, e.getValue().asIntegerValue().getBigInteger());
				break;
			case FLOAT:
				messageMap.put(key, e.getValue().asFloatValue().getDouble());
				break;
			case ARRAY:
				messageMap.put(key, e.getValue().asArrayValue());
				break;
			case MAP:
				messageMap.put(key, e.getValue().asMapValue());
				break;
			case RAW:
				String rawValue = e.getValue().asRawValue().toString().replace("\"", "");
				messageMap.put(key, rawValue);
				break;
			case NIL:
				messageMap.put(key, e.getValue().asNilValue());
				break;
			}
		}
		return messageMap;
	}

	public static BigInteger asssociate(String ipAddress, int port) {

		Socket socket = connect(ipAddress, port);
		BigInteger sessionID = INVALID_SESSION_ID;
		try {
			if (socket == null)
				throw new IOException();
			DataInputStream networkReader = new DataInputStream(socket.getInputStream());
			DataOutputStream networkWriter = new DataOutputStream(socket.getOutputStream());
			
			NetworkMsg networkCommand = NetworkMsg.MSG_send_associateMessage();
			sendCommand(networkWriter, networkCommand);
			ByteArrayBuffer responseArray = receiveMsg(networkReader);
			String resString = new String(responseArray.toByteArray());
			HashMap messageMap = convertMessagePackToMap(responseArray.toByteArray());
			sessionID = (BigInteger) messageMap.get(NetworkMsg.KEY_SESSION_ID);
			
			networkReader.close();
			networkWriter.close();
			socket.close();
		} catch (IOException e) {
			errMsg = e.getMessage();
			KLog.printErr(e.getMessage());
		}
		return sessionID;
	}

}
