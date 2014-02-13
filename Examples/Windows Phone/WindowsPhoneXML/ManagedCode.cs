using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using InterfaceMyType;
using System.Windows;


namespace WindowsPhoneXML
{
    class MyManagedSide : IMyType
    {
        public String Name 
        { 
            get 
            {
                return "This String came from C#";
            }
        }

        public void MyMethod()
        {
            MessageBox.Show("This messageBox came from C#");
        }
    }
}
