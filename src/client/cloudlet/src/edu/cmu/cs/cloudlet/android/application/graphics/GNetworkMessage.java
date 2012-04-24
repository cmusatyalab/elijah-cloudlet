package edu.cmu.cs.cloudlet.android.application.graphics;

import java.io.File;

import org.json.JSONException;
import org.json.JSONObject;

import android.util.Log;

public class GNetworkMessage {
    public final static int COMMAND_REQ_TRANSFER_START                      = 0x0021;
    public final static int COMMAND_ACK_TRANSFER_START                      = 0x0022;
    
    protected int commandNumber = -1;
    protected float[] accData = null;
        
	public GNetworkMessage(int cmd, float[] accData) {
		this.commandNumber = cmd;
		this.accData = accData;
	}
	
	public int getCmdNumber(){
		return this.commandNumber;
	}

	public float[] getAccData() {
		return this.accData;
	}

}
