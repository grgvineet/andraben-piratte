package com.example.provajni;

import com.example.cpabe.NativeCPABE;

import android.app.Activity;
import android.app.NativeActivity;
import android.os.Bundle;
import android.view.Menu;
import android.view.MenuItem;

public class MainActivity extends Activity {

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);
		
		
		NativeCPABE c = new NativeCPABE();
		
		c.setup("sdcard/pub.pub", "sdcard/mskey",NativeCPABE.TYPE1R_160Q_512);
		c.enc("sdcard/pub.pub", "A and B and C","sdcard/to_enc.txt");
		c.keygen("sdcard/pub.pub", "sdcard/mskey", "sdcard/priv_user.prv", "A B C D", 4);
		c.dec("sdcard/pub.pub", "sdcard/priv_user.prv", "sdcard/to_enc.txt.cpabe", "sdcard/to_enc.txt.dec");
	}

	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		// Inflate the menu; this adds items to the action bar if it is present.
		getMenuInflater().inflate(R.menu.main, menu);
		return true;
	}

	@Override
	public boolean onOptionsItemSelected(MenuItem item) {
		// Handle action bar item clicks here. The action bar will
		// automatically handle clicks on the Home/Up button, so long
		// as you specify a parent activity in AndroidManifest.xml.
		int id = item.getItemId();
		if (id == R.id.action_settings) {
			return true;
		}
		return super.onOptionsItemSelected(item);
	}
}
