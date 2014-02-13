
package com.server.app;


import android.app.Activity;
import android.content.Intent;

import android.view.View;

import android.widget.Button;
import android.widget.ScrollView;
import android.widget.TextView;

import android.os.Bundle;
import android.os.Handler;

import java.util.ArrayList;

import java.util.Collections;

import java.util.Timer;
import java.util.TimerTask;

// for the back key trap
import android.view.KeyEvent;
import android.app.AlertDialog;
import android.content.DialogInterface;

// to optain IP address
import java.net.*;
import java.util.*;   
import org.apache.http.conn.util.InetAddressUtils;

//  command to run via ADB
// platform-tools>adb shell am start -a android.intent.action.MAIN -n com.server.app/.Server

public class Server extends Activity
{
	

	private TextView mText;
	private static String mTextData = "";
	private static int mLineCount = 13;
	static String mServer = "";
	static String mUser = "";
	static String mPassword = "";
	static String mDeviceName = "";
	
	static String mEnableHTTP = "yes";
	static String mHTTPHomePath = "/sdcard/home/";
	static String mHTTPIndex = "index.html";
	static String mServerLogFile = "/sdcard/ServerLog.txt";
	
	static String mConnectTrace = "no";
	
	
	
	private ScrollView mScrollView;
	private Handler myHandler = new Handler();
	ArrayList<String> my_array = new ArrayList<String>();
	
	 public static String getIPAddress(boolean useIPv4) {
	        try {
	            List<NetworkInterface> interfaces = Collections.list(NetworkInterface.getNetworkInterfaces());
	            for (NetworkInterface intf : interfaces) {
	                List<InetAddress> addrs = Collections.list(intf.getInetAddresses());
	                for (InetAddress addr : addrs) {
	                    if (!addr.isLoopbackAddress()) {
	                        String sAddr = addr.getHostAddress().toUpperCase();
	                        boolean isIPv4 = InetAddressUtils.isIPv4Address(sAddr); 
	                        if (useIPv4) {
	                            if (isIPv4) 
	                                return sAddr;
	                        } else {
	                            if (!isIPv4) {
	                                int delim = sAddr.indexOf('%'); // drop ip6 port suffix
	                                return delim<0 ? sAddr : sAddr.substring(0, delim);
	                            }
	                        }
	                    }
	                }
	            }
	        } catch (Exception ex) { } // for now eat exceptions
	        return "";
	    }
	
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        
        setContentView(R.layout.log_text_box_1);

        mScrollView = ((ScrollView) findViewById(R.id.scrollview));
        mText = (TextView)findViewById(R.id.textview);

	    
	    
		if (mServer.length() == 0)
			mServer = "swb.itcdatapro.com";
		if (mEnableHTTP.length() == 0)
			mEnableHTTP = "no";
		if (mHTTPHomePath.length() == 0)
			mHTTPHomePath = "/sdcard/home";
		if (mHTTPIndex.length() == 0)
			mHTTPIndex = "index.html";
			
	
	
        if (mTextData.length() > 0)
        {
        	mText.setText(mTextData);
        }
        else
        {
              // Log our IP address and switchboard connect path at startup
	 	    String strIP = "Connect to Direct IP: " + getIPAddress(true);
		    messageMe(strIP);
		    
		    
        }
        

		
      // Declare the timer to print messages off the array.
        Timer t = new Timer();
        //Set the schedule function and rate
        t.scheduleAtFixedRate(new TimerTask() {

            @Override
            public void run() {
                //Called each time when 1000 milliseconds (1 second) (the period parameter)
            	runOnUiThread(new Runnable() {

            	    @Override
            	    public void run() {
            	    	
            	    	myHandler.postDelayed(r, 500);

            	    	
	            }
            	    	
            	});
            	
            }
                 
        },
        //Set how long before to start calling the TimerTask (in milliseconds)
        1000,
        //Set the amount of time between each execution (in milliseconds)
        1000);
   
        
     
        
        
        // "Start" button handler
        Button startButton = (Button) findViewById(R.id.btnStart);
        startButton.setOnClickListener(new View.OnClickListener() {

            public void onClick(View v) {
            
            	
            	String args = BuildServerArgList();
               	
            	serverStart(args);
           
            
            } });
        

        // "Stop" button handler
        Button stopButton = (Button) findViewById(R.id.btnStop);
        stopButton.setOnClickListener(new View.OnClickListener() {

            public void onClick(View v) {
            finish();
            } });
        
        
        // "Setup" button handler
        Button setupButton = (Button) findViewById(R.id.btnSetup);
        setupButton.setOnClickListener(new View.OnClickListener() {

       	
            public void onClick(View v) {
// Uncomment this for a framework User interface to get more variables from the user
//                Intent i = new Intent(Server.this, ConfigSetup.class);
//                startActivityForResult(i, 777);
        	 }
        });


        
        // "HTTP" button handler
        Button HTTPButton = (Button) findViewById(R.id.btnHTTP);
        HTTPButton.setOnClickListener(new View.OnClickListener() {

            public void onClick(View v) {
                Intent i = new Intent(Server.this, HTTPSetup.class);
                startActivityForResult(i, 777);

            } });

        
        // "Clear" button handler
        Button clearButton = (Button) findViewById(R.id.btnClear);
        clearButton.setOnClickListener(new View.OnClickListener() {

            public void onClick(View v) {
            	
            	clearView();
            	
  
            } });
        
        
    }

    private void clearView()
    {
	  	mTextData = "";
		mText.setText("");
	
	    // Log our IP address and switchboard connect path at startup
	 	String strIP = "Connect to Direct IP: " + getIPAddress(true);
		messageMe(strIP);
		mLineCount = 13;
    }
    
    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
      if ( requestCode == 777) {

    	//save new settings

    	clearView();
    	
      }
    }     
    
    
    public String BuildServerArgList()
    {
    	String strResult = "";

    	// go look in Server.cpp to see where this goes
    	
    	// 1. Log File
    	// 2. Enable HTTP
    	// 3. HTTP Index.html
    	// 4. HTTP Home Dir
    	// 5. Connect Trace
     	String AND = "&&";
      	strResult = mServerLogFile + AND + mEnableHTTP + AND + mHTTPIndex + AND + mHTTPHomePath + AND + mConnectTrace;

    	return strResult;
    }

    
    
    final Runnable r = new Runnable()
    {
        public void run() 
        {
        	if (my_array.size() > 0)
        	{
	        	while(my_array.size() > 0)
	           	{
	            		String s = my_array.remove(0);
	            		mText.append(s);
	                    mText.setLines(mLineCount++);
	            }
		    	mText.invalidate();
		    	mScrollView.post(new Runnable() {
        	    @Override
        	    public void run() {
        	    	mScrollView.fullScroll(ScrollView.FOCUS_DOWN);
        	    }
		    	});
        	}
        }
    };

    
    
    @Override 
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        //Handle the back button
        if(keyCode == KeyEvent.KEYCODE_BACK) {
            //Ask the user if they want to quit
            new AlertDialog.Builder(this)

            .setTitle("Stop Server?")
            .setMessage("You are about to Stop Server." + 
                         "Do you really want to exit?")
            .setPositiveButton("Yes", new DialogInterface.OnClickListener() {
                 @Override
                public void onClick(DialogInterface dialog, int which) {
                    //Stop the activity
                     //maintenancetabs.this.finish();
                     int pid = android.os.Process.myPid();
                     android.os.Process.killProcess(pid);
                    }
             })
            .setNegativeButton("No", null)
            .show();
             return true;
        }     else {
            return super.onKeyDown(keyCode, event);
        }
     }
    
   
    
    public void messageMe(String text)
    {
    	text += "\n";
        my_array.add(text);          
    	mTextData += text;
    }


	public native String serverInteract( int nOperation, String strArg1, String strArg2 );
    public native int serverStart(String strStartMessage);
    static {
        System.loadLibrary("Server");
    }
}


