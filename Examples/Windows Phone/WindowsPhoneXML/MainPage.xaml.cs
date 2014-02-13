using System;
using System.Collections.Generic;
using System.Linq;
using System.Net;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Navigation;
using Microsoft.Phone.Controls;
using Microsoft.Phone.Shell;
using InterfaceMyType;
using NativeSide;

// to write a file for the HTTP server
using System.IO;
using System.Threading.Tasks;
using Windows.Storage;
using System.Text;


namespace WindowsPhoneXML
{
    public partial class MainPage : PhoneApplicationPage
    {
        // Constructor
        public MainPage()
        {
            InitializeComponent();
        }
        private void DisplayResults(String strResults)
        {
            // count the number of lines(\n) in the results
            int nLines = 0;
            for (int i = 0; i < strResults.Length; i++)
            {
                if (strResults[i] == '\n')
                    nLines++;
            }
            xmlOutput.Height += (nLines * 20);  // grow the control so it scrolls properly
            xmlOutput.Text += strResults;       // log the results to the screen
        }
        private async void StartServer(object sender, RoutedEventArgs e)
        {
            IMyType myManagedObject = null;      // this is the base class interface to a managed object
            myManagedObject = new MyManagedSide();  // a managed c# class passed into the unmanaged side
            SelfManaged goodManagement = new SelfManaged(myManagedObject);

            // we will write this string to an HTML file, and it will be the home page when the HTTP server starts on the phone (on port 8080)
            // after you start the server, (by pressing "Start" on the phone) it will display the IP address of the phone in my case its 192.168.30.129
            // so this web page can be found here:   http://192.168.30.129:8080/
            String strFileData = "<html><head><title>XMLFoundation</title></head><body><p>XMLFoundation says - Hello World!!! on Windows Phone</p></body></html>";
            
            Encoding utf8 = Encoding.UTF8;
            byte[] fileBytes = utf8.GetBytes(strFileData);

            // Get the local folder.
            StorageFolder local = Windows.Storage.ApplicationData.Current.LocalFolder;
            String strHTTPHome = local.Path + "\\HTTPHome";

            // Create a new folder name DataFolder.
            var dataFolder = await local.CreateFolderAsync("HTTPHome",  CreationCollisionOption.OpenIfExists);

            // Create a new file named DataFile.txt.
            var file = await dataFolder.CreateFileAsync("Index.html",    CreationCollisionOption.ReplaceExisting);

            // Write the data from the textbox.
            using (var s = await file.OpenStreamForWriteAsync())
            {
                s.Write(fileBytes, 0, fileBytes.Length);
            }


            // all of the work is done in the native code
            String strResults = goodManagement.StartServerCore("ServerLog.txt&&Yes&&Index.html&&" + strHTTPHome + "&&Yes");
            DisplayResults(strResults);

            // do this on a timer
            String strUpdatedResults = goodManagement.GetServerLogData();
        }
        private void Test(object sender, RoutedEventArgs e)
        {
            IMyType myManagedObject = null;      // this is the base class interface to a managed object
            myManagedObject = new MyManagedSide();  // a managed c# class

            SelfManaged goodManagement = new SelfManaged(myManagedObject);

            // all of the work is done in the native code
            String strResults = goodManagement.Test();
            DisplayResults(strResults);
        }
        private void StartHere0(object sender, RoutedEventArgs e)
        {
            IMyType myManagedObject = null;      // this is the base class interface to a managed object
            myManagedObject = new MyManagedSide();  // a managed c# class

            SelfManaged goodManagement = new SelfManaged(myManagedObject);

            // all of the work is done in the native code
            String strResults = goodManagement.NativeStartHere0();
            DisplayResults(strResults);
                        
        }
    }
}