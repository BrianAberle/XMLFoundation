package com.server.app;


import android.app.Activity;
import android.os.Bundle;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.TextView;



public class HTTPSetup extends Activity implements OnClickListener
{


	private TextView mEnableHTTP;
	private TextView mHTTPHomePath;
	private TextView mHTTPIndex;
	private TextView mServerLogFile;

    @Override
    public void onClick(View src) {

    	Server.mEnableHTTP = mEnableHTTP.getText().toString();
    	Server.mHTTPHomePath = mHTTPHomePath.getText().toString();
    	Server.mHTTPIndex = mHTTPIndex.getText().toString();
    	Server.mServerLogFile = mServerLogFile.getText().toString();
    	

    	finish();
    }
    
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.httpsetup);

        mEnableHTTP = (TextView)findViewById(R.id.txtEnableHTTP);
        mHTTPHomePath = (TextView)findViewById(R.id.txHTTPHome);
        mHTTPIndex = (TextView)findViewById(R.id.txtHTMLIndex);
        mServerLogFile = (TextView)findViewById(R.id.txtServerLogFile);

        mEnableHTTP.setText(Server.mEnableHTTP);
        mHTTPHomePath.setText(Server.mHTTPHomePath);
        mHTTPIndex.setText(Server.mHTTPIndex);
        mServerLogFile.setText(Server.mServerLogFile);
     
        Button button = (Button) findViewById(R.id.btnSave);
        button.setOnClickListener(this);
    }
}