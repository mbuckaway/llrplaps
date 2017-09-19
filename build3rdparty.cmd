@echo off
echo Building Thirdparty
mkdir thirdparty
cd thirdparty
echo Getting LIBXML2
git clone git://git.gnome.org/libxml2 libxml2
cd libxml2
cd win32
cscript configure.js iconv=no prefix=..\.. debug=yes
nmake install
cd ..

echo Getting LIBXSLT
git clone git://git.gnome.org/libxslt libxslt 
cd libxslt
cd win32
cscript configure.js iconv=no prefix=..\.. include=..\..\include\libxml2 lib=..\..\lib debug=yes
nmake install
cd ..

echo Getting LLRPLTK....
git clone https://github.com/mbuckaway/LLRPToolkit LLRPToolkit
cd LLRPToolkit
mkdir build
cd build
cmake -G "NMake Makefiles"



