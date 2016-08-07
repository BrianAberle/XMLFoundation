package a777.root.GApp;

import android.os.Environment;
import android.view.View;
import android.widget.TextView;

import java.io.BufferedReader;
//import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
//import java.util.ArrayList;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;


public class GAppGlobal
{
    public static View tabServer;
    public static View tabConfig;
    public static View tabHTTP;
    public static View tabNext;
    public static View tabSystem;
    public static View tabXML;
    public static View tabEmail;

    static String mTextData;
    static boolean bUpdateTextData = true;
    public static int mLineCount = 0;

    private static final Lock _mutex = new ReentrantLock(true);


    public static void messageMe(String text)
    {

        text += "\n";

        // C++ code and Java code calls this global static method, lets not update the same variable at the same time from two different threads.
        _mutex.lock();

            GAppGlobal.mLineCount += GAppGlobal.lineCount(text);
            GAppGlobal.mTextData += text;
            bUpdateTextData = true;

        _mutex.unlock();
    }


    static public String ShellExec(String command) {
        String strResult = "";
        String line;
        OutputStream stdin = null;
        InputStream stderr = null;
        InputStream stdout = null;
        try {
            // launch EXE and grab stdin/stdout and stderr
            Process process = null;
//          process = Runtime.getRuntime().exec("su"); // su is superuser aka root
            process = Runtime.getRuntime().exec("sh"); // any shell will do for almost anything

            stdin = process.getOutputStream();
            stderr = process.getErrorStream();
            stdout = process.getInputStream();

            // "write" the parms into stdin inside the shell
            line = command + "\n";
            stdin.write(line.getBytes() );
            stdin.flush();

            // then close the shell by writing "exit" to stdin
            line = "exit" + "\n";
            stdin.write(line.getBytes() );
            stdin.flush();
            stdin.close();

            // grab any output in stdout and cleanup
            BufferedReader brCleanUp = new BufferedReader (new InputStreamReader(stdout));
            while ((line = brCleanUp.readLine ()) != null) {
                strResult += line + "\n";
            }
            brCleanUp.close();

            // clean up if any output in stderr
            brCleanUp =  new BufferedReader (new InputStreamReader (stderr));
            while ((line = brCleanUp.readLine ()) != null) {
                strResult += line + "\n";

            }
            brCleanUp.close();

        } catch (IOException e) {
        }
        return strResult;
    }

    static public String Kill(String command)
    {
        String strResult = "";
        int pid = Integer.parseInt(command);
        android.os.Process.killProcess(pid);
        return strResult;
    }


    public static String GetAppConfigFileName()
    {
        //
        // Get the Configuration file name, either from the overriding GUI value or the default
        //
        String strFileName = "";
        if (GAppGlobal.tabConfig != null) {
            TextView tv = (TextView) GAppGlobal.tabConfig.findViewById(a777.root.GApp.R.id.editConfigFileName);
            strFileName = tv.getText().toString();
        }
        if (strFileName.isEmpty()) {
            strFileName += Environment.getExternalStorageDirectory().getPath();
            strFileName += "/GAppConfig.txt";
            if (GAppGlobal.tabConfig != null) {
                TextView tv = (TextView) GAppGlobal.tabConfig.findViewById(a777.root.GApp.R.id.editConfigFileName);
                tv.setText(strFileName);
            }
        }
        return strFileName;
    }


    public static String GetAppConfig()
    {
        String strFileName = GetAppConfigFileName();

        //
        // Get the Configuration file DATA, either from the overriding GUI value or the default
        //
        String strFileContents = "";
        strFileContents = GAppGlobal.loadFile(strFileName);

        if (strFileContents.isEmpty()) {
            strFileContents += "[System]\r\n";
            strFileContents += "Pool=5\r\n";
            strFileContents += "ProxyPool=5\r\n";
            strFileContents += "LogFile=";
            strFileContents += Environment.getExternalStorageDirectory().getPath();
            strFileContents += "/GAppLog.txt\r\n";
            strFileContents += "LogCache=1\r\n";

            strFileContents += "\r\n[HTTP]\r\n";
            strFileContents += "Enable=yes\r\n";
            strFileContents += "Port=8888\r\n";
            strFileContents += "Home=";
            strFileContents += Environment.getExternalStorageDirectory().getPath();
            strFileContents += "/\r\n";
            strFileContents += "Index=index.html\r\n";
            strFileContents += "UseKeepAlives=1\r\n";
            strFileContents += "ContentCache=0\r\n";

            strFileContents += "\r\n[HTTPProxy]\r\n";
            strFileContents += "Enable=yes\r\n";
            strFileContents += "Port=8080\r\n";
            strFileContents += "RequireAuth=0\r\n";
            strFileContents += "LogFileName=\r\n";
            strFileContents += "LogEnabled=0\r\n";
            strFileContents += "RestrictIPs=1\r\n";
            strFileContents += "RestrictBeginMatch=10.0.\r\n";
            strFileContents += "Only443or80=1\r\n";
            strFileContents += "NegotiateBasic=0\r\n";
            strFileContents += "BlockHTTPProxyAds=1\r\n";
            strFileContents += "EnableOutboundConnectCache=0\r\n";
            strFileContents += "NegotiateNTLMv2=1\r\n";
            strFileContents += "LogHTTPTxnFiles=0\r\n";
            strFileContents += "BlockGoogle=1\r\n";

            strFileContents += "\r\n[Trace]\r\n";
            strFileContents += "Enable=1\r\n";
            strFileContents += "HTTPFilesTrace=1\r\n";
            strFileContents += "TransferSizes=0\r\n";
            strFileContents += "HTTPHeaderTrace=0\r\n";
            strFileContents += "HTTPHeaderTrace=0\r\n";
            strFileContents += "ThreadTrace=0\r\n";
            strFileContents += "ConnectTrace=1\r\n";
            strFileContents += "HTTPProxyTrace=0\r\n";
            strFileContents += "HTTPProxyTraceBinary=0\r\n";
            strFileContents += "HTTPProxyTraceExtra=0\r\n";
            strFileContents += "HTTPProxyReturnTrace=0\r\n";

            strFileContents += "\r\n[Email]\r\n";
            strFileContents += "SMTPServer=smtp.gmail.com\r\n";
            strFileContents += "SMTPPort=587\r\n";
            strFileContents += "Protocol=TLS\r\n";
            strFileContents += "LoginUser=YourGmailGoesHere@gmail.com\r\n"; // 1. put your GMAIL account here
            strFileContents += "LoginPass=YourPasswordGoesHere\r\n";             // 2. put your GMAIL password here
            strFileContents += "SenderAlias=XMLFoundation\r\n";
            strFileContents += "SenderEmail=DTrump@whitehouse.gov\r\n"; // i think google replaces this value
            strFileContents += "EmailSubject=Default Subject\r\n";
            strFileContents += "EmailRecipient=YourEmailGoesHere@yahoo.com\r\n";  //   3. put any of your email addres(s) here delimited by a ;
            strFileContents += "EmailMessage=The Mainframe is crashing.  Can you come save the world?\r\n";

        }
        return strFileContents;
    }




    public static native String serverInteract( int nOperation, String strArg1, String strArg2 );
    public static native int serverStart(String strStartMessage);
    public static native void serverStop();
    public static native int    configSetINIData(String strINIFileData);
    public static native int    configStore(  String strINIFileName );
    public static native String configLoad(  String strINIFileName );
    public static native String configGet(  String strSection, String strEntry );
    public static native int    configSet(  String strSection, String strEntry, String strValue );
    public static native int    emailSend(  String jsServer, String jsServerPort, String jsProtocol, String jsUser, String jsPass, String SenderAlias, String SenderEmail, String EmailSubject, String EmailRecipient, String EmailMessage);
    public static native String getWANIP();
    public static native String getThisIP();
    public static native String getThisHost();
    public static native String loadFile(String strFile);
    public static native int saveFile(String strFile, String strFileData);
    public static native int lineCount(String strFileData);
    static {
        System.loadLibrary("GApp");
    }



};


