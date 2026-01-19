@echo off
echo Building distributed-kv...

mkdir build 2>nul
cd build

echo Compiling...
g++ -c ../src/protocol.cpp -I../include -o protocol.o
g++ -c ../src/store.cpp -I../include -o store.o
g++ -c ../src/threadpool.cpp -I../include -o threadpool.o
g++ -c ../src/server.cpp -I../include -o server.o
g++ -c ../src/client.cpp -I../include -o client.o

echo Linking...
g++ protocol.o store.o threadpool.o server.o -o server.exe -lws2_32 -static -O2
g++ protocol.o client.o -o client.exe -lws2_32 -static -O2

echo Done!
cd ..
