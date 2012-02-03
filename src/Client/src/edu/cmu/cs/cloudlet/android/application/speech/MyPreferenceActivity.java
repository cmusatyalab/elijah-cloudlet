//
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
package edu.cmu.cs.cloudlet.android.application.speech;

import edu.cmu.cs.cloudlet.android.R;
import android.content.SharedPreferences;
import android.content.SharedPreferences.OnSharedPreferenceChangeListener;
import android.os.Bundle;
import android.preference.EditTextPreference;
import android.preference.PreferenceActivity;

public class MyPreferenceActivity extends PreferenceActivity implements OnSharedPreferenceChangeListener
{
	public static final String KEY_SERVER_ADDRESS_PREFERENCE = "serveraddresspreference";
	public static final String KEY_SERVER_PORT_PREFERENCE = "serverportpreference";
	
	private EditTextPreference serverAddressPreference;
	private EditTextPreference portAddressPreference;
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		 addPreferencesFromResource(R.xml.preferences);
		
		 serverAddressPreference = (EditTextPreference)getPreferenceScreen().findPreference( KEY_SERVER_ADDRESS_PREFERENCE );
		 portAddressPreference = (EditTextPreference)getPreferenceScreen().findPreference( KEY_SERVER_PORT_PREFERENCE );
		 
		 updateSharedPreferenceSummaries();
		 
		 getPreferenceScreen().getSharedPreferences().registerOnSharedPreferenceChangeListener( this );
		 
		 
	}

	@Override
	public void onSharedPreferenceChanged( SharedPreferences sharedPreferences, String key )
	{
		updateSharedPreferenceSummaries();
	}
	
	
	public void updateSharedPreferenceSummaries()
	{
		 String defa = getPreferenceScreen().getSharedPreferences().getString(KEY_SERVER_ADDRESS_PREFERENCE, "127.0.0.1");
		 serverAddressPreference.setSummary(defa);
		 
		 String defb = getPreferenceScreen().getSharedPreferences().getString(KEY_SERVER_PORT_PREFERENCE, "6789");
		 portAddressPreference.setSummary(defb);
	}
}
