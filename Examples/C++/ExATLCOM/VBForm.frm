VERSION 5.00
Begin VB.Form Form1 
   Caption         =   "Form1"
   ClientHeight    =   8175
   ClientLeft      =   60
   ClientTop       =   345
   ClientWidth     =   8220
   LinkTopic       =   "Form1"
   ScaleHeight     =   8175
   ScaleWidth      =   8220
   StartUpPosition =   3  'Windows Default
   Begin VB.CommandButton cmdRemoveSubs 
      Caption         =   "Remove All Sub Objects/Interfaces"
      Height          =   375
      Left            =   4560
      TabIndex        =   17
      Top             =   7680
      Width           =   3015
   End
   Begin VB.CommandButton cmdDump 
      Caption         =   "Dump Root"
      Height          =   375
      Left            =   2160
      TabIndex        =   15
      Top             =   7680
      Width           =   2055
   End
   Begin VB.TextBox txtCustID 
      Height          =   285
      Left            =   1200
      TabIndex        =   14
      Text            =   "123"
      Top             =   6480
      Width           =   615
   End
   Begin VB.CommandButton cmdEight 
      Caption         =   "8. 777 Sub-Interface:"
      Height          =   375
      Left            =   0
      TabIndex        =   12
      Top             =   6000
      Width           =   2055
   End
   Begin VB.CommandButton Command3 
      Caption         =   "6. Clear TextBox"
      Height          =   375
      Left            =   0
      TabIndex        =   11
      Top             =   4800
      Width           =   2055
   End
   Begin VB.TextBox txtString 
      Height          =   375
      Left            =   3480
      TabIndex        =   10
      Text            =   "Text1"
      Top             =   840
      Width           =   2775
   End
   Begin VB.CommandButton cmdSeven 
      Caption         =   "7. View Object State"
      Height          =   375
      Left            =   0
      TabIndex        =   9
      Top             =   5400
      Width           =   2055
   End
   Begin VB.CommandButton cndFive 
      Caption         =   "5. Push XML into Object"
      Height          =   375
      Left            =   0
      TabIndex        =   8
      Top             =   4200
      Width           =   2055
   End
   Begin VB.CommandButton cndFour 
      Caption         =   "4. Set VB App TextBox"
      Height          =   375
      Left            =   0
      TabIndex        =   7
      Top             =   3600
      Width           =   2055
   End
   Begin VB.CommandButton cmdThree 
      Caption         =   "3. View Object state"
      Height          =   375
      Left            =   0
      TabIndex        =   6
      Top             =   3000
      Width           =   2055
   End
   Begin VB.CommandButton cmdTwo 
      Caption         =   "2. Modify COM  Object"
      Height          =   375
      Left            =   0
      TabIndex        =   5
      Top             =   2400
      Width           =   2055
   End
   Begin VB.CommandButton cndOne 
      Caption         =   "1. View Object State"
      Height          =   375
      Left            =   0
      TabIndex        =   4
      Top             =   1800
      Width           =   2055
   End
   Begin VB.TextBox Text2 
      Height          =   5775
      Left            =   2160
      MultiLine       =   -1  'True
      TabIndex        =   1
      Text            =   "VBForm.frx":0000
      Top             =   1800
      Width           =   5775
   End
   Begin VB.TextBox txtCount 
      Height          =   375
      Left            =   2160
      TabIndex        =   0
      Text            =   "Text1"
      Top             =   840
      Width           =   1215
   End
   Begin VB.Label Label5 
      Caption         =   "Read ExAtlCOM.html"
      BeginProperty Font 
         Name            =   "MS Sans Serif"
         Size            =   18
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   495
      Left            =   2280
      TabIndex        =   18
      Top             =   0
      Width           =   3615
   End
   Begin VB.Label Label4 
      Caption         =   "============================================================="
      Height          =   135
      Left            =   2160
      TabIndex        =   16
      Top             =   1320
      Width           =   5535
   End
   Begin VB.Label Label3 
      Caption         =   "CustomerID="
      Height          =   255
      Left            =   120
      TabIndex        =   13
      Top             =   6480
      Width           =   975
   End
   Begin VB.Label Label2 
      Caption         =   "Data through Com over XML Interface:"
      Height          =   255
      Left            =   2160
      TabIndex        =   3
      Top             =   1560
      Width           =   3015
   End
   Begin VB.Label Label1 
      Caption         =   "Data through Direct COM Interface:"
      Height          =   255
      Left            =   2160
      TabIndex        =   2
      Top             =   600
      Width           =   2895
   End
End
Attribute VB_Name = "Form1"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
    Dim MyO As IMyATLObj

Private Sub cmdDump_Click()
    MsgBox MyO.Dump
End Sub

Private Sub cmdEight_Click()
    Dim MyO2 As IMyATLObj
    Set MyO2 = MyO.GetMyInterface(txtCustID.Text)
    If (MyO2 Is Nothing) Then
        MsgBox ("Requested CustomerID Not found")
    Else
        ' invoke a method on the interface returned by GetMyInterface()
        MsgBox MyO2.XMLState
        Set MyO2 = Nothing
    End If
End Sub

Private Sub cmdRemoveSubs_Click()
    MyO.RemoveSubObjects
End Sub

Private Sub cmdSeven_Click()
    Call cndOne_Click
End Sub

Private Sub cmdThree_Click()
    Call cndOne_Click
End Sub

Private Sub cmdTwo_Click()
    MyO.TheCount = 777
    MyO.TheString = "Hello World"
End Sub

Private Sub cndFive_Click()
    MyO.XMLState = Text2
End Sub

Private Sub cndFour_Click()
    Text2 = "<Container>" + Chr$(13) + Chr$(10) + "     <CustomerName>Brian</CustomerName>" + Chr$(13) + Chr$(10) + "     <CustomerID>777</CustomerID>" + Chr$(13) + Chr$(10) + "     <Container>" + Chr$(13) + Chr$(10) + "          <CustomerName>Sean</CustomerName>" + Chr$(13) + Chr$(10) + "          <CustomerID>123</CustomerID>" + Chr$(13) + Chr$(10) + "     </Container>" + Chr$(13) + Chr$(10) + "     <Container>" + Chr$(13) + Chr$(10) + "          <CustomerName>Bob</CustomerName>" + Chr$(13) + Chr$(10) + "          <CustomerID>456</CustomerID>" + Chr$(13) + Chr$(10) + "     </Container>" + Chr$(13) + Chr$(10) + "</Container>"
    
End Sub

Private Sub cndOne_Click()
    Text2 = MyO.XMLState
    txtCount = MyO.TheCount
    txtString = MyO.TheString
End Sub

Private Sub Command1_Click()
    txtCount = MyO.TheCount
    Text2 = MyO.XMLState
End Sub

Private Sub Command2_Click()
    MyO.XMLState = Text2
End Sub

Private Sub Command3_Click()
    Text2 = ""
End Sub

Private Sub Form_Load()
 
    Set MyO = New MyATLObj
    txtCount = MyO.TheCount

End Sub

Private Sub Form_Unload(Cancel As Integer)
    Set MyO = Nothing
End Sub

