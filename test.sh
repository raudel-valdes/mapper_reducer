#!/bin/bash
# This is a comment

./clean_os.sh
rm *~
make clean
make
echo
echo Three tests will be performed and testing will take around 12 seconds...

# DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
# sed 's,replace,'"$DIR"',' commandFile1.dat > commandFile1.txt
# sed 's,replace,'"$DIR"',' commandFile2.dat > commandFile2.txt
# sed 's,replace,'"$DIR"',' commandFile3.dat > commandFile3.txt

./mapper commandFile3.txt 10&
sleep 1
./reducer out.txt
sleep 3
