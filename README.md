# n-api-tools
A toolbox containing C++ classes to be used with node-addon-api. 
Also contains a ``var`` data type, which is, as the name suggests variable:
```c++
var s = 5; // s is now 5 (double / integer)
s + "5"; // s is now "55" (string)

s = "some string"; // s is now "some string" (string)

s = s + "a" + 54; // s is now "some stringa54" (string)

s = 5; // s is now 5 (double / integer)
s = s + "a " + true; // s is now "5a true"
```

I still don't know why anybody would want this, but here it is. I mean if you like endless weird
compilation errors, you're welcome, stranger. May create a stand-alone version of this.