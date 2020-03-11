#!/bin/bash
# This is a comment

./clean_os.sh
rm *.txt
rm *~
make clean
make
echo
echo Three tests will be performed and testing will take around 12 seconds...

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
sed 's,replace,'"$DIR"',' commandFile1.dat > commandFile1.txt
sed 's,replace,'"$DIR"',' commandFile2.dat > commandFile2.txt
sed 's,replace,'"$DIR"',' commandFile3.dat > commandFile3.txt

./mapper commandFile1.txt 10&
sleep 1
./reducer student_out1.txt
sleep 3

if diff -w student_out1.txt correct_output/correct_out1.txt; then
    echo Test 1: Success-----------------------------------------------Success
else
    echo Test 1: Fail--------------------------------------------------Fail
fi

./mapper commandFile2.txt 10&
sleep 1
./reducer student_out2.txt
sleep 3
if diff -w student_out2.txt correct_output/correct_out2.txt; then
    echo Test 2: Success-----------------------------------------------Success
else
    echo Test 2: Fail--------------------------------------------------Fail
fi

./mapper commandFile3.txt 10&
sleep 1
./reducer student_out3.txt
sleep 3
if diff -w student_out3.txt correct_output/correct_out3.txt; then
    echo Test 3: Success-----------------------------------------------Success
else
    echo Test 3: Fail--------------------------------------------------Fail
fi
