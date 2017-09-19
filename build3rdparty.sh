@echo off
echo Building Thirdparty
mkdir thirdparty
cd thirdparty
echo Getting LLRPLTK....
git clone https://github.com/mbuckaway/LLRPToolkit LLRPToolkit
cd LLRPToolkit
mkdir build
cd build
cmake -G "Unix Makefiles"



