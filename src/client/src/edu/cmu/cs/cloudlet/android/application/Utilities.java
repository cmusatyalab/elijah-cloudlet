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
package edu.cmu.cs.cloudlet.android.application;

import android.app.AlertDialog;
import android.content.Context;
import android.util.Log;

public class Utilities {
	public static void showError(Context context, String title, String msg) {
		Log.e("krha", title + ":" + msg);
		new AlertDialog.Builder(context).setTitle(title).setMessage(msg).setPositiveButton("Confirm", null).show();
	}
}
