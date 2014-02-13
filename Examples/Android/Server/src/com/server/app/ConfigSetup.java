package com.server.app;

import android.app.Activity;
import android.os.Bundle;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.TextView;
import java.util.UUID;


public class ConfigSetup  extends Activity implements OnClickListener{

	private TextView mServer;
	private TextView mUser;
	private TextView mPass;
	private TextView mDeviceName;

    @Override
    public void onClick(View src) {

    	Server.mServer = mServer.getText().toString();
    	Server.mUser = mUser.getText().toString();
    	Server.mPassword = mPass.getText().toString();
    	Server.mDeviceName = mDeviceName.getText().toString();
    	
    	
    	//Intent i = new Intent(this, Server.class);
    	//startActivity(i);
    	finish();
    }
    
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.setup);

        mServer = (TextView)findViewById(R.id.txtServer);
        mUser = (TextView)findViewById(R.id.txtUser);
        mPass = (TextView)findViewById(R.id.txtPassword);
        mDeviceName = (TextView)findViewById(R.id.txtDeviceName);

        mDeviceName.setText(Server.mDeviceName);
        mPass.setText(Server.mPassword);
        mUser.setText(Server.mUser);
        mServer.setText(Server.mServer);

        if ( Server.mDeviceName.length() == 0)
        {
        	DeviceUuidFactory f = new DeviceUuidFactory(getApplicationContext());
        	UUID uu = f.getDeviceUuid();
        	mDeviceName.setText(uu.toString());
        }
        
        Button button = (Button) findViewById(R.id.btnSave);
        button.setOnClickListener(this);
    }
}