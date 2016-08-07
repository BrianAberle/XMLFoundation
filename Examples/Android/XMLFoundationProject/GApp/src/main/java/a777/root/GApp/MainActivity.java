package a777.root.GApp  ;

import android.app.ActionBar;
import android.app.FragmentTransaction;
import android.os.Bundle;
import android.os.Environment;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;
import android.widget.ScrollView;

import java.io.File;
import java.util.Timer;
import java.util.TimerTask;
import android.os.Handler;
import android.support.v4.app.Fragment;             // in android-support-v4.jar
import android.support.v4.app.FragmentActivity;     // in android-support-v4.jar
import android.support.v4.app.FragmentManager;      // in android-support-v4.jar
import android.support.v4.app.FragmentPagerAdapter; // in android-support-v4.jar
import android.support.v4.view.ViewPager;           // in android-support-v4.jar
import android.widget.ArrayAdapter;
import android.widget.Spinner;

public class MainActivity
        extends FragmentActivity
        implements ActionBar.TabListener {



    // for each of the primary sections of the app. We use a .v4.app.FragmentPagerAdapter
    //  derivative, which will keep every loaded fragment in memory.
    private AppSectionsPagerAdapter mAppSectionsPagerAdapter;
    private ViewPager mViewPager;

    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(a777.root.GApp.R.layout.activity_main);

        // Create the adapter that will return a fragment for each of the three primary sections
        // of the app.
        mAppSectionsPagerAdapter = new AppSectionsPagerAdapter(getSupportFragmentManager());

        // Set up the action bar.
        final ActionBar actionBar = getActionBar();

        // Specify that the Home/Up button should not be enabled, since there is no hierarchical parent.
        actionBar.setHomeButtonEnabled(false);

        // Specify that we will be displaying tabs in the action bar.
        actionBar.setNavigationMode(ActionBar.NAVIGATION_MODE_TABS);

        // Set up the ViewPager, attaching the adapter and setting up a listener for when the
        // user swipes between sections.
        mViewPager = (ViewPager) findViewById(a777.root.GApp.R.id.pager);
        mViewPager.setAdapter(mAppSectionsPagerAdapter);
        mViewPager.setOnPageChangeListener(new ViewPager.SimpleOnPageChangeListener() {
            @Override
            public void onPageSelected(int position) {
                // When swiping between different app sections, select the corresponding tab.
                // We can also use ActionBar.Tab#select() to do this if we have a reference to the
                // Tab.
                actionBar.setSelectedNavigationItem(position);
            }
        });

        // For each of the sections in the app, add a tab to the action bar.
        for (int i = 0; i < mAppSectionsPagerAdapter.getCount(); i++) {
            // Create a tab with text corresponding to the page title defined by the adapter.
            // Also specify this Activity object, which implements the TabListener interface, as the
            // listener for when this tab is selected.
            actionBar.addTab(
                    actionBar.newTab()
                            .setText(mAppSectionsPagerAdapter.getPageTitle(i))
                            .setTabListener(this));
        }
    }

    @Override
    public void onTabUnselected(ActionBar.Tab tab, FragmentTransaction fragmentTransaction) {
    }

    @Override
    public void onTabSelected(ActionBar.Tab tab, FragmentTransaction fragmentTransaction) {
        // When the given tab is selected, switch to the corresponding page in the ViewPager.
        mViewPager.setCurrentItem(tab.getPosition());
    }

    @Override
    public void onTabReselected(ActionBar.Tab tab, FragmentTransaction fragmentTransaction) {
    }

    // This is where each "Fragment" of the application is manageed.
    public static class AppSectionsPagerAdapter extends FragmentPagerAdapter {

        public AppSectionsPagerAdapter(FragmentManager fm) {
            super(fm);
        }

        @Override
        public Fragment getItem(int i) {
            switch (i) {
                case 0:
                    return new ServerSectionFragment();
                case 1:
                    return new SystemSectionFragment();
                case 2:
                    return new XMLSectionFragment();
                case 3:
                    return new EmailSectionFragment();
                case 4:
                    return new ConfigSectionFragment();
                case 5:
                    return new HTTPSectionFragment();

                default:
                    // The other sections of the app are placeholders for future functionality
                    Fragment fragment = new NextSectionFragment();
                    Bundle args = new Bundle();
                    args.putInt(NextSectionFragment.ARG_SECTION_NUMBER, i + 1);
                    fragment.setArguments(args);
                    return fragment;
            }
        }

        @Override
        public int getCount() {
            return 6;
        }

        @Override
        public CharSequence getPageTitle(int position)
        {
            if (position == 0) return " Server ";
            if (position == 1) return " System ";
            if (position == 2) return " XML ";
            if (position == 3) return " Email ";
            if (position == 4) return " Config ";
            if (position == 5) return " Debug ";
            return " Tab " + (position + 1);
        }
    }





    public static class ServerSectionFragment extends Fragment {
      private ScrollView mScrollView;
      private TextView mText;
      private Handler myHandler = new Handler();
      public static boolean bDumpArray = false;


      final Runnable r = new Runnable()
        {
            public void run()
            {
                if (GAppGlobal.tabServer == null) {
                    return;
                }
                final ScrollView mScrollView = ((ScrollView) GAppGlobal.tabServer.findViewById(a777.root.GApp.R.id.scrollView));
                final TextView mText = (TextView) GAppGlobal.tabServer.findViewById(a777.root.GApp.R.id.editText);

                // update the log text
                String strCurrentText = mText.getText().toString();

                if (strCurrentText == GAppGlobal.mTextData)
                    return;

                if (GAppGlobal.mTextData != null && GAppGlobal.bUpdateTextData) {
                    GAppGlobal.bUpdateTextData = false;
                    mText.setText(GAppGlobal.mTextData);
                    mText.setLines(GAppGlobal.mLineCount);
                    mText.invalidate();

                    mScrollView.post(new Runnable() {
                        @Override
                        public void run() {   mScrollView.fullScroll(ScrollView.FOCUS_DOWN); }
                    });
                }

            }
        };




        @Override
        public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {

            final View rootView = inflater.inflate(a777.root.GApp.R.layout.fragment_section_server, container, false);

            GAppGlobal.tabServer = rootView;

            mScrollView = ((ScrollView) rootView.findViewById(a777.root.GApp.R.id.scrollView));
            mText = (TextView) rootView.findViewById(a777.root.GApp.R.id.editText);

            // Declare the timer to print messages from the arrayLines to the TextView.
            Timer t = new Timer();
            //Set the schedule function and rate
            t.scheduleAtFixedRate(new TimerTask() {
                                      @Override
                                      public void run() {   myHandler.postDelayed(r, 500); } },
                    //Set how long before to start calling the TimerTask (in milliseconds)
                    1000,
                    //Set the amount of time between each execution (in milliseconds)
                    1000);



            //
            // On Clicked Start
            //
            rootView.findViewById(a777.root.GApp.R.id.start).setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View view) {

                    String s = "WAN IP:" + GAppGlobal.getWANIP();
                    GAppGlobal.messageMe(s);

                    s = "IP:" + GAppGlobal.getThisIP();
                    GAppGlobal.messageMe(s);

                    s = "Host:" + GAppGlobal.getThisHost();
                    GAppGlobal.messageMe(s);

                    GAppGlobal.serverStart(GAppGlobal.GetAppConfig());
                }
            });



            //
            // On Clicked Stop
            //
            rootView.findViewById(a777.root.GApp.R.id.stop).setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View view) {

                    GAppGlobal.serverStop();

                }
            });

            //
            // On Clicked Clear
            //
            rootView.findViewById(a777.root.GApp.R.id.clear).setOnClickListener(new View.OnClickListener() {
                        @Override
                        public void onClick(View view) {

                            TextView tv = (TextView) rootView.findViewById(a777.root.GApp.R.id.editText);
                            tv.setText( "" );
                            GAppGlobal.mLineCount = 0;
                        }
                    });

            return rootView;
        }

        }
    // End of Server app fragment









    public static class ConfigSectionFragment extends Fragment {

        static String mServerLogFile = Environment.getExternalStorageDirectory().getPath();
        private ScrollView mScrollView;
        private TextView mText;
        private Handler myHandler = new Handler();


        @Override
        public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {

            final View rootView = inflater.inflate(a777.root.GApp.R.layout.fragment_section_config, container, false);
            GAppGlobal.tabConfig = rootView;

            mScrollView = ((ScrollView) rootView.findViewById(a777.root.GApp.R.id.scrollView));
            mText = (TextView) rootView.findViewById(a777.root.GApp.R.id.editText);


            String strFileContents = GAppGlobal.GetAppConfig();
            mText.setText(strFileContents);

            //
            // On Clicked Save
            //
            rootView.findViewById(a777.root.GApp.R.id.save).setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View view) {

                    GAppGlobal.messageMe("Saved Config file:");

                    TextView tv = (TextView) rootView.findViewById(a777.root.GApp.R.id.editConfigFileName);
                    String strFile = tv.getText().toString();
                    GAppGlobal.messageMe(strFile);

                    String strFileData = mText.getText().toString();
                    GAppGlobal.saveFile(strFile,strFileData);
                }
            });




            //
            // On Clicked Load
            //
            rootView.findViewById(a777.root.GApp.R.id.load).setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View view) {

                    GAppGlobal.messageMe("Loading...");
                    //
                    TextView tv = (TextView)rootView.findViewById(a777.root.GApp.R.id.editConfigFileName);
                    String sFileName = tv.getText().toString();
                    GAppGlobal.messageMe( sFileName );

                    if (!sFileName.isEmpty()) {
                        String strFileContents = GAppGlobal.loadFile(sFileName);
                        if (!strFileContents.isEmpty()) {
                            mText.setText(strFileContents);
                        }
                    }

                }
            });


            return rootView;
        }
    }





    public static class SystemSectionFragment extends Fragment {

        private ScrollView mScrollView;
        private TextView mText;
        private Handler myHandler = new Handler();
        static String mTextDataDefault = "Pids will list the Process ID's \nPorts will list the TCP ports in use.\n\n\nEnter a numeric pid of a process to kill then press kill.\n\nOR\n\nEnter any ADB shell command  then press execute. try:\nnetstat -nlp | grep :80\nls /system/bin\n ";
        static String mTextData = "";
        public static int mLineCount = 0;

        final Runnable r = new Runnable()
        {
            public void run()
            {
                final ScrollView mScrollView = ((ScrollView) GAppGlobal.tabSystem.findViewById(a777.root.GApp.R.id.scrollView));
                final TextView mText = (TextView) GAppGlobal.tabSystem.findViewById(a777.root.GApp.R.id.editText);

                mText.setText( mTextData );
                mText.setLines(mLineCount);

                mText.invalidate();
                mScrollView.post(new Runnable() {
                    @Override
                    public void run() {
//                        mScrollView.fullScroll(ScrollView.FOCUS_DOWN);
                    }
                });
            }
        };

        public static void showResults(String text)
        {
            // clean off the usage instructions with the first bit of output
            if ( text == "null" || text == "" )
                return;

            int n = GAppGlobal.tabSystem.getMeasuredWidth();

            text += "\n";
            mLineCount += GAppGlobal.lineCount(text);
            mTextData += text;
        }



        @Override
        public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {

            final View rootView = inflater.inflate(a777.root.GApp.R.layout.fragment_section_system, container, false);
            GAppGlobal.tabSystem = rootView;

            mScrollView = ((ScrollView) rootView.findViewById(a777.root.GApp.R.id.scrollView));
            mText = (TextView) rootView.findViewById(a777.root.GApp.R.id.editText);

            // Declare the timer to print messages from the arrayLines to the TextView.
            Timer t = new Timer();
            //Set the schedule function and rate.  call after 1000millisecond delay, and at 1000ms intervals
            t.scheduleAtFixedRate(new TimerTask() {
                @Override
                public void run() {
                    myHandler.postDelayed(r, 500);
                }
            }, 1000, 1000);

            showResults(mTextDataDefault);


            //
            // On Clicked Kill
            //
            rootView.findViewById(a777.root.GApp.R.id.one).setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View view) {

                    TextView tv = (TextView) rootView.findViewById(a777.root.GApp.R.id.editCommand);
                    String strPID = tv.getText().toString();
//                    GAppGlobal.messageMe("Kill");
//                    GAppGlobal.messageMe(strFile);
                    showResults(GAppGlobal.Kill(strPID));

                }
            });



            //
            // On Clicked Execute
            //
            rootView.findViewById(a777.root.GApp.R.id.buttonExec).setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View view) {

                    TextView tv = (TextView) rootView.findViewById(a777.root.GApp.R.id.editCommand);
                    String strFile = tv.getText().toString();
//                    GAppGlobal.messageMe("Exec");
//                    GAppGlobal.messageMe(strFile);
                    showResults(GAppGlobal.ShellExec(strFile));

                }
            });

            //
            // On Clicked Pids
            //
            rootView.findViewById(a777.root.GApp.R.id.pids).setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View view) {

                    // display list of running processes
                    showResults(GAppGlobal.serverInteract(1, "", ""));
                }
            });


            //
            // On Clicked Ports
            //
            rootView.findViewById(a777.root.GApp.R.id.ports).setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View view) {

                    // display a netstat
                    showResults(GAppGlobal.serverInteract(2, "", ""));
                }
            });



            //
            // On Clicked Clear
            //
            rootView.findViewById(a777.root.GApp.R.id.clear).setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View view) {
                    mTextData = "";
                    mLineCount = 0;
                }
            });


            return rootView;
        }
    }









    public static class XMLSectionFragment extends Fragment {

        private ScrollView mScrollView;
        private TextView mText;
        private Handler myHandler = new Handler();
        static String mTextDataDefault = "Go = XMLFoundation Serialization\n\nLogfile = Displays the System Log\n\nDelete = Deletes the Logfile\n\n+line = view 1 more line\n\nClear = Clear this area";
        static String mTextData = "";
        public static int mLineCount = 0;
        // ************************************************************************
        // this will be the initial state of the objects.
        // ************************************************************************
        public static String getXML1()
        {
            return "<Customer>" +
                    "	<long>4555444333</long>" +
                    "	<short>2</short>" +
                    "	<double>3.333</double>" +
                    "	<byte>65</byte>" +
                    "	<bool>false</bool>" +
                    "	<FirstName>Autodidacticism</FirstName>" +
                    "	<CustomerID>777</CustomerID>" +
                    "	<StringList>" +
                    "		<Level2Wrapper>" +
                    "			<StringItem>Hello</StringItem>" +
                    "			<StringItem>There</StringItem>" +
                    "			<StringItem>World</StringItem>" +
                    "		</Level2Wrapper>" +
                    "	</StringList>" +
                    "	<Order TransactionTime='today' >"+
                    "		<SalesPerson>Omega</SalesPerson>" +
                    "		<OrderNumber>911</OrderNumber>" +
                    "		<LineItemContainer>" +
                    "			<LineItem>" +
                    "				<Description>Peace beyond understanding</Description>" +
                    "				<Quantity>OceansWavesCount</Quantity>" +
                    "				<SKU>123</SKU>" +
                    "			</LineItem>" +
                    "			<LineItem>" +
                    "				<Description>Freedom</Description>" +
                    "				<Quantity>Ultimate</Quantity>" +
                    "				<SKU>456</SKU>" +
                    "			</LineItem>" +
                    "		</LineItemContainer>" +
                    "	</Order>"+
                    "</Customer>";
        }

        // ************************************************************************
        // Then this state will be 'applied' to the object instances.
        // It causes the customer Name to change to 'You' and updates
        // an existing LineItem(456) and insert a new LineItem(789)
        // ************************************************************************
        public static String getXML2()
        {
            return "<Customer>" +
                    "	<FirstName>Gnosis</FirstName>" +
                    "	<Order>"+
                    "		<LineItemContainer>" +
                    "		<LineItem>" +
                    "			<Quantity>Infinate</Quantity>" +
                    "			<SKU>456</SKU>" +
                    "		</LineItem>" +
                    "		<LineItem>" +
                    "			<Description>Life</Description>" +
                    "			<Quantity>Eternal</Quantity>" +
                    "			<SKU>789</SKU>" +
                    "		</LineItem>" +
                    "		</LineItemContainer>" +
                    "	</Order>"+
                    "</Customer>";
        }



        final Runnable r = new Runnable()
        {
            public void run()
            {

                final ScrollView mScrollView = ((ScrollView) GAppGlobal.tabXML.findViewById(a777.root.GApp.R.id.scrollView));
                final TextView mText = (TextView) GAppGlobal.tabXML.findViewById(a777.root.GApp.R.id.editText);

                mText.setText(mTextData);
                mText.append("\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
                mText.setLines(mLineCount+ 14);

                mText.invalidate();
                mScrollView.post(new Runnable() {
                    @Override
                    public void run() {
//                        mScrollView.fullScroll(ScrollView.FOCUS_DOWN);
                    }
                });
            }
        };

        public static void showResults(String text)
        {
            if ( text == "null" || text == "" )
                return;

            text += "\n";
            mLineCount += GAppGlobal.lineCount(text);
            mTextData += text;
        }

        @Override
        public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {

            final View rootView = inflater.inflate(a777.root.GApp.R.layout.fragment_section_xml, container, false);
            GAppGlobal.tabXML = rootView;

            mScrollView = ((ScrollView) rootView.findViewById(a777.root.GApp.R.id.scrollView));
            mText = (TextView) rootView.findViewById(a777.root.GApp.R.id.editText);

            // Declare the timer to print messages from the arrayLines to the TextView.
            Timer t = new Timer();
            //Set the schedule function and rate.  call after 1000millisecond delay, and at 1000ms intervals
            t.scheduleAtFixedRate(new TimerTask() {
                @Override
                public void run() {myHandler.postDelayed(r, 500);}}, 1000, 1000);

            showResults(mTextDataDefault);





            //
            // On Clicked XML Example
            //
            rootView.findViewById(a777.root.GApp.R.id.one).setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View view) {

                    //********************************************************************
                    // Example 1. direct inheritance
                    //********************************************************************
                    // create a customer, assign data through XML, then print the object

                    Customer1 c1 = new Customer1();
                    // Assign all the member variables from XML (and create instances of pure java Orders objects as defined in the XML
                    c1.fromXML( getXML1() );

                    // seeing is believing, so lets just peek at the data in our instance of a Customer.  Look how much
                    // java code was written in Customer1::ObjectDump() to dump this object...
                    // There is an easier way - just write it out in XML
                    c1.ObjDump();


                    // now do it the easy way - no code to write
                    showResults( c1.toXML() );
                    showResults("************************\n");

                    // Now change the object members.  1st with nothing but XML(and no code)
                    c1.fromXML( getXML2() );
                    // that was easy - we just changed the customers name changed from "Autodidacticism" to "Gnosis" and thanks to MapObjectId()
                    // the XML made made KEYED changes to the line items in the Order Object.  LineItem 456 was UPDATED to have
                    // a quantity of "Infinate" and LineItem 789 was INSERTED.  Adding and inserting objects normally requires lots of
                    // hand coded application logic - using XMLObject all that is inherited logic.  Seeing is believing:
                    showResults(c1.toXML());	    // view through XML - lets do things the easy way

                    // we can manipulate the object model via XML as we see - but we can still manipulate the 1 and only object model
                    // directly with hand coded Java logic.  Here we add line Item 777
                    c1.getOrder().addLineItem(777, "The Spirit", "Always");

                    // Look Gnosis has an Order with 4 lineItems:
                    // SKU          Description                         Quantity            note
                    // -----------------------------------------------------------------------------------------------------------
                    // 123          Peace beyond understanding          OceansWaveCount
                    // 456          Freedom                             Infinate            ('Ultimate' was updated to 'Infinate')
                    // 777          The Spirit                          Always              (inserted by Java)
                    // 789          Life                                Eternal             (inserted by XML)
                    showResults("************************\n");

                    // The screen is small on my phone - so I can see the XML better - I email to myself
                    GAppGlobal.emailSend(GAppGlobal.configGet("Email", "SMTPServer"),
                            GAppGlobal.configGet("Email", "SMTPPort"),
                            GAppGlobal.configGet("Email", "Protocol"),
                            GAppGlobal.configGet("Email", "LoginUser"),
                            GAppGlobal.configGet("Email", "LoginPass"),
                            GAppGlobal.configGet("Email", "SenderAlias"),
                            GAppGlobal.configGet("Email", "SenderEmail"),
                            "Example 1 Direct inheritance",
                            GAppGlobal.configGet("Email", "EmailRecipient"),
                            c1.toXML());

                    // this is what I got in my inbox  (I added a few tabs )
/*
                    <Customer>
                    <FirstName>Gnosis</FirstName>
                    <CustomerID>777</CustomerID>
                    <long>4555444333</long>
                    <short>2</short>
                    <double>3.333</double>
                    <byte>54</byte>
                    <bool>False</bool>
                        <Order TransactionTime="today">
                        <SalesPerson>Omega</SalesPerson>
                        <OrderNumber>911</OrderNumber>
                        <LineItemContainer>
                            <LineItem>
                                <Quantity>OceansWavesCount</Quantity>
                                <Description>Peace beyond understanding</Description>
                                <SKU>123</SKU>
                            </LineItem>
                            <LineItem>
                                <Quantity>Infinate</Quantity>
                                <Description>Freedom</Description>
                                <SKU>456</SKU>
                            </LineItem>
                            <LineItem>
                                <Quantity>Eternal</Quantity>
                                <Description>Life</Description>
                                <SKU>789</SKU>
                            </LineItem>
                            <LineItem>
                                <Quantity>Always</Quantity>
                                <Description>The Spirit</Description>
                                <SKU>777</SKU>
                            </LineItem>
                        </LineItemContainer>
                        </Order>
                    <StringList>
                    <Level2Wrapper>
                    <StringItem>Hello</StringItem>
                    <StringItem>There</StringItem>
                    <StringItem>World</StringItem>
                    </Level2Wrapper>
                    </StringList>
                    </Customer>

*/








                    //********************************************************************
                    // Example 2. indirect inheritance-containment doing the same as above
                    //********************************************************************
                    // Notice that this time we are using a Customer2 - defined differently in the Object model.
                    // class Customer1 extends XMLObject
                    // class Customer2 contains "private XMLObject	ContainedXMLObj" as member data.
                    // so that you know JavaXMLFoundation does not impose that direct inheritance design constraint
                    // here you see the same functionality from a Java object that derives from nothing.
                    Customer2 c2 = new Customer2( getXML1() );

                    c2.ObjDump();
                    c2.ApplyXML(getXML2());
                    c2.getOrder().addLineItem(777, "The Spirit", "Indirectly Inherited... Always");
                    showResults(c2.XMLDump());


                    // the XML is identical to above except for 1 value
                    //  <Quantity>Indirectly Inherited... Always</Quantity>
                    GAppGlobal.emailSend(GAppGlobal.configGet("Email", "SMTPServer"),
                            GAppGlobal.configGet("Email", "SMTPPort"),
                            GAppGlobal.configGet("Email", "Protocol"),
                            GAppGlobal.configGet("Email", "LoginUser"),
                            GAppGlobal.configGet("Email", "LoginPass"),
                            GAppGlobal.configGet("Email", "SenderAlias"),
                            GAppGlobal.configGet("Email", "SenderEmail"),
                            "Example 2 Indirectly Inherited",
                            GAppGlobal.configGet("Email", "EmailRecipient"),
                            c2.XMLDump() );


                }
            });

            //
            // On Clicked Two
            //
            rootView.findViewById(a777.root.GApp.R.id.two).setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View view) {

                    // load the log file
                    String sLog = GAppGlobal.loadFile( GAppGlobal.configGet("System", "LogFile") );
                    showResults( sLog );

                }
            });


            //
            // On Clicked delete
            //
            rootView.findViewById(a777.root.GApp.R.id.three).setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View view) {

                    // delete the log file
                    File f = new File( GAppGlobal.configGet("System", "LogFile"));
                    f.delete();
                    showResults( "delete" );

                }
            });


            //
            // On Clicked line+
            //
            rootView.findViewById(a777.root.GApp.R.id.four).setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View view) {

                    showResults( "++line++" );

                }
            });



            //
            // On Clicked Clear
            //
            rootView.findViewById(a777.root.GApp.R.id.clear).setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View view) {
                    mTextData = "";
                    mLineCount = 0;
                }
            });


            return rootView;
        }
    }











    public static class EmailSectionFragment extends Fragment {

        static Spinner spinner = null;

        private ScrollView mScrollView;
        private TextView mText;
        private Handler myHandler = new Handler();
        static String mTextDataDefault = "OpenSSL TLS SMTP Example";
        static String mTextData = "";
        public static int mLineCount = 0;
        static String stLast = "";


        final Runnable r = new Runnable()
        {
            public void run()
            {

                final ScrollView mScrollView = ((ScrollView) GAppGlobal.tabEmail.findViewById(a777.root.GApp.R.id.scrollView));
                final TextView mText = (TextView) GAppGlobal.tabEmail.findViewById(a777.root.GApp.R.id.editText);

                String st = (String)spinner.getSelectedItem();
                if (st != stLast){
                    stLast = st;
                    EmailSectionFragment.showResults(st);
                    final TextView mValueText = (TextView) GAppGlobal.tabEmail.findViewById(a777.root.GApp.R.id.editTextValue);
                    mValueText.setText( GAppGlobal.configGet("Email", stLast) );
                    mValueText.invalidate();
                    EmailSectionFragment.showResults(GAppGlobal.configGet("Email", stLast));
                }

                mText.setText(mTextData);
                mText.setLines(mLineCount);

                mText.invalidate();
                mScrollView.post(new Runnable() {
                    @Override
                    public void run() {
//                        mScrollView.fullScroll(ScrollView.FOCUS_DOWN);
                    }
                });
            }
        };

        public static void showResults(String text)
        {
            if ( text == "null" || text == "" )
                return;

            text += "\n";
            mLineCount += GAppGlobal.lineCount(text);
            mTextData += text;
        }


        @Override
        public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {

            final View rootView = inflater.inflate(a777.root.GApp.R.layout.fragment_section_email, container, false);
            GAppGlobal.tabEmail = rootView;

            mScrollView = ((ScrollView) rootView.findViewById(a777.root.GApp.R.id.scrollView));
            mText = (TextView) rootView.findViewById(a777.root.GApp.R.id.editText);


            spinner = (Spinner)rootView.findViewById(R.id.spinner);

            ArrayAdapter<String> spinnerArrayAdapter = new ArrayAdapter<String>(getContext(),
                    android.R.layout.simple_spinner_item, new String[] { "SMTPServer","SMTPPort","Protocol","LoginUser","LoginPass","SenderAlias","SenderEmail","EmailSubject","EmailRecipient","EmailMessage"  });

            spinnerArrayAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);

            spinner.setAdapter(spinnerArrayAdapter);  // Tell the spinner about our adapter

            GAppGlobal.configLoad(GAppGlobal.GetAppConfigFileName());




            // Declare the timer to print messages from the arrayLines to the TextView.
            Timer t = new Timer();
            //Set the schedule function and rate.  call after 1000millisecond delay, and at 1000ms intervals
            t.scheduleAtFixedRate(new TimerTask() {
                @Override
                public void run() {myHandler.postDelayed(r, 500);}}, 1000, 1000);





            //
            // On Clicked SetValue
            //
            rootView.findViewById(a777.root.GApp.R.id.setvalue).setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View view) {

                    TextView tv = (TextView) rootView.findViewById(a777.root.GApp.R.id.editTextValue);
                    String strValue = tv.getText().toString();

                    String strEntry = (String)spinner.getSelectedItem();
                    GAppGlobal.configSet("Email", strEntry, strValue);
                    GAppGlobal.configStore(GAppGlobal.GetAppConfigFileName());

                }
            });



            //
            // On Clicked ShowAll
            //
            rootView.findViewById(a777.root.GApp.R.id.showall).setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View view) {

                    showResults(GAppGlobal.configGet("Email", "SMTPServer"));
                    showResults(GAppGlobal.configGet("Email", "SMTPPort"));
                    showResults(GAppGlobal.configGet("Email", "Protocol"));
                    showResults(GAppGlobal.configGet("Email", "LoginUser"));
                    showResults(GAppGlobal.configGet("Email", "LoginPass"));
                    showResults(GAppGlobal.configGet("Email", "SenderAlias"));
                    showResults(GAppGlobal.configGet("Email", "SenderEmail"));
                    showResults(GAppGlobal.configGet("Email", "EmailSubject"));
                    showResults(GAppGlobal.configGet("Email", "EmailRecipient"));
                    showResults(GAppGlobal.configGet("Email", "EmailMessage"));

                }
            });


            //
            // On Clicked Send
            //
            rootView.findViewById(a777.root.GApp.R.id.send).setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View view) {

                    GAppGlobal.emailSend(GAppGlobal.configGet("Email", "SMTPServer"),
                            GAppGlobal.configGet("Email", "SMTPPort"),
                            GAppGlobal.configGet("Email", "Protocol"),
                            GAppGlobal.configGet("Email", "LoginUser"),
                            GAppGlobal.configGet("Email", "LoginPass"),
                            GAppGlobal.configGet("Email", "SenderAlias"),
                            GAppGlobal.configGet("Email", "SenderEmail"),
                            GAppGlobal.configGet("Email", "EmailSubject"),
                            GAppGlobal.configGet("Email", "EmailRecipient"),
                            GAppGlobal.configGet("Email", "EmailMessage"));

                }
            });



            //
            // On Clicked Clear
            //
            rootView.findViewById(a777.root.GApp.R.id.clear).setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View view) {
                    mTextData = "";
                    mLineCount = 0;
                }
            });


            return rootView;
        }



    }





    public static class HTTPSectionFragment extends Fragment {

        @Override
        public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {

            final View rootView = inflater.inflate(a777.root.GApp.R.layout.fragment_section_http, container, false);
            GAppGlobal.tabHTTP = rootView;

            //
            // On Clicked button1
            //
            rootView.findViewById(a777.root.GApp.R.id.button1).setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View view) {

                    // testing audio capture
                    GAppGlobal.serverInteract(4, "", "");
                    GAppGlobal.messageMe("Button1");


                }
            });


            //
            // On Clicked button2
            //
            rootView.findViewById(a777.root.GApp.R.id.button2).setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View view) {

                    GAppGlobal.messageMe("Button2");

                }
            });

            return rootView;
        }
    }











    // A fragment in this GUI framework representing the NEXT section of this app.
    public static class NextSectionFragment extends Fragment {

        public static final String ARG_SECTION_NUMBER = "section_number";

        @Override
        public View onCreateView(LayoutInflater inflater, ViewGroup container,
                Bundle savedInstanceState) {
            View rootView = inflater.inflate(a777.root.GApp.R.layout.fragment_section_next, container, false);

            GAppGlobal.tabNext = rootView;

            Bundle args = getArguments();
            ((TextView) rootView.findViewById(android.R.id.text1)).setText(
                    getString(a777.root.GApp.R.string.next_section_text, args.getInt(ARG_SECTION_NUMBER)));
            return rootView;
        }
    }




}
