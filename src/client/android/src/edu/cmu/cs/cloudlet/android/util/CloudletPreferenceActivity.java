package edu.cmu.cs.cloudlet.android.util;

import edu.cmu.cs.cloudlet.android.R;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.SharedPreferences.OnSharedPreferenceChangeListener;
import android.os.Bundle;
import android.preference.EditTextPreference;
import android.preference.Preference;
import android.preference.Preference.OnPreferenceClickListener;
import android.text.InputType;

public class CloudletPreferenceActivity extends android.preference.PreferenceActivity implements
		OnSharedPreferenceChangeListener, OnPreferenceClickListener {

	private EditTextPreference ipaddressPref;
	private EditTextPreference portnumberPref;

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		addPreferencesFromResource(R.xml.cloudlet_preferences);

		ipaddressPref = (EditTextPreference) getPreferenceScreen().findPreference(getString(R.string.synthesis_pref_address));
		portnumberPref = (EditTextPreference) getPreferenceScreen().findPreference(getString(R.string.synthesis_pref_port));
//		portnumberPref.getEditText().setInputType(InputType.TYPE_CLASS_NUMBER);
		updatePreferneces();
		getPreferenceScreen().getSharedPreferences().registerOnSharedPreferenceChangeListener(this);

	}
	
	public void updatePreferneces()
	{
		String ipAddress = getPreferenceScreen()
		.getSharedPreferences()
		.getString( getString(R.string.synthesis_pref_address), getString(R.string.synthesis_default_ip_address));
		ipaddressPref.setSummary(ipAddress);

		String portNumber = getPreferenceScreen()
		.getSharedPreferences()
		.getString(getString(R.string.synthesis_pref_port), getString(R.string.synthesis_default_port));
		portnumberPref.setSummary(portNumber);
	}
	
	@Override
	public boolean onPreferenceClick(Preference arg0) {
		// TODO Auto-generated method stub
		return false;
	}

	@Override
	public void onSharedPreferenceChanged(SharedPreferences sharedPreferences, String key) {
		updatePreferneces();	
	}
	
	@Override
	protected void onActivityResult(int requestCode, int resultCode, Intent data) 
	{
		super.onActivityResult(requestCode, resultCode, data);
	}

}
