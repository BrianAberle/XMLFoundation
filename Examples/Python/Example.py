##############################################################
# Format: Class::Method[ArgName&&DataType]&&&&
# where [ArgName&&DataType] repeats for each argument
##############################################################
def ExposedMethods(): return \
"Global::SayHello!" \
"Global::FnWithParams&Name&String&Value&Integer!" \
"MyClass::MyPublic1&x&Integer&y&Integer!" \
"MyClass::MyPublic2&x&Integer&y&Integer!" \
"MyClass::MyPublic3&x&Integer&y&Integer!" \
"AClass::MyPublic1&x&Integer&y&Integer!" \
"AClass::MyPublic2&x&Integer&y&Integer!" \
"AClass::MyPublic3&x&Integer&y&Integer!" 


def SayHello():
return "Hello " \
"from Python"

def FnWithParams(Name, Value):
return "Name[%s] Value[%i]" % (Name, Value)


class MyClass:
def MyPrivate(self, x, y): return "%i-MyPrivate-%i" % (x, y)
def MyPublic1(self, x, y): return "%i-MyPublic1-%i" % (x, y)
def MyPublic2(self, x, y): return "%i-MyPublic2-%i" % (x, y)
def MyPublic3(self, x, y): return "%i-MyPublic3-%i" % (x, y)

class AClass:
def MyPrivate(self, x, y): return "%i-APrivate-%i" % (x, y)
def MyPublic1(self, x, y): return "%i-APublic1-%i" % (x, y)
def MyPublic2(self, x, y): return "%i-APublic2-%i" % (x, y)
def MyPublic3(self, x, y): return "%i-APublic3-%i" % (x, y)

