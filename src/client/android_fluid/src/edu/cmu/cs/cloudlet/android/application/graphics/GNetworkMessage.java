//
// Elijah: Cloudlet Infrastructure for Mobile Computing
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
