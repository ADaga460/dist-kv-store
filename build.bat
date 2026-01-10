@echo off
echo Building distributed-kv...

mkdir build 2>nul
cd build

echo Compiling source files...
g++ -c -static  ../src/protocol.cpp -I../include -o protocol.o
g++ -c -static  ../src/store.cpp -I../include -o store.o
g++ -c -static  ../src/server.cpp -I../include -o server.o
g++ -c -static  ../src/client.cpp -I../include -o client.o

echo Linking server...
g++ protocol.o store.o server.o -o server.exe -lws2_32 -static 

echo Linking client...
g++ protocol.o client.o -o client.exe -lws2_32 -static 

echo Build complete!
echo.
echo Run server: build\server.exe
echo Run client: build\client.exe set key1 value1
cd ..
