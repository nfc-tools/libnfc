#!/bin/bash

cat << EOF | \
    pn53x-tamashell |\
    awk '\
        /^> #.*:/{
            sub(/^> #/,"")
            n=$0
            for (i=0;i<8-length();i++) {
                n= n " "
            }
            getline
            getline
            getline
            sub(/Rx: 00/,"")
            gsub(/ +/," ")
            sub(/ 90 00 $/,"")
            print n toupper($0)}' |\
    grep -v ": 6A 83"

# Select one typeB target
4A010300

# Select ICC file
4001 80a4 0800 04 3f00 0002
#ICC:
4001 80b2 0104 1d

# Select Holder file
4001 80a4 0800 04 3f00 3f1c
#Holder1:
4001 80b2 0104 1d
#Holder2:
4001 80b2 0204 1d

# Select EnvHol file
4001 00a4 0800 04 2000 2001
#EnvHol1:
4001 00b2 0104 1d
#EnvHol2:
4001 00b2 0204 1d

# Select EvLog file
4001 00a4 0800 04 2000 2010
#EvLog1:
4001 00b2 0104 1d
#EvLog2:
4001 00b2 0204 1d
#EvLog3:
4001 00b2 0304 1d

# Select ConList file
4001 00a4 0800 04 2000 2050
#ConList:
4001 00b2 0104 1d

# Select Contra file
4001 00a4 0800 04 2000 2020
#Contra1:
4001 00b2 0104 1d
#Contra2:
4001 00b2 0204 1d
#Contra3:
4001 00b2 0304 1d
#Contra4:
4001 00b2 0404 1d
#Contra5:
4001 00b2 0504 1d
#Contra6:
4001 00b2 0604 1d
#Contra7:
4001 00b2 0704 1d
#Contra8:
4001 00b2 0804 1d
#Contra9:
4001 00b2 0904 1d
#ContraA:
4001 00b2 0a04 1d
#ContraB:
4001 00b2 0b04 1d
#ContraC:
4001 00b2 0c04 1d

# Select Counter file
4001 00a4 0800 04 2000 2069
#Counter:
4001 00b2 0104 1d

# Select LoadLog file
4001 00a4 0800 04 1000 1014
#LoadLog:
4001 00b2 0104 1d

# Select Purcha file
4001 00a4 08 0004 1000 1015
#Purcha1:
4001 00b2 0104 1d
#Purcha2:
4001 00b2 0204 1d
#Purcha3:
4001 00b2 0304 1d

# Select SpecEv file
4001 00a4 08 0004 2000 2040
#SpecEv1:
4001 00b2 0104 1d
#SpecEv2:
4001 00b2 0204 1d
#SpecEv3:
4001 00b2 0304 1d
#SpecEv4:
4001 00b2 0404 1d
EOF


