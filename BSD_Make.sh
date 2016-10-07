#! /bin/csh

# pseudo makefile for BSD

set EXECUTABLE="firewall_holepunch"
set CC="cc"
set CPP="c++"


$CPP -c -O2 *.cpp -std=c++0x
$CC -c -O2 *.c
$CPP -o "$EXECUTABLE" *.o
