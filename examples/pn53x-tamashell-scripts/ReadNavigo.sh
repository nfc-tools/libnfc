#!/bin/bash

ID=$(cat << EOF | \
    pn53x-tamashell |\
    grep -A1 "^Tx: 42  01  0b  3f  80" |\
    grep -o -P "(?<=Rx: 00  ..  ..  )..  ..  ..  .."|sed 's/  //g'
# Timeouts
3205000002
# ListTarget ModeB
4a010300
# TypeB' APGEN
42010b3f80
EOF
)

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
            print n toupper($0)}'

# Timeouts
3205000002

# ListTarget ModeB
4a010300

# TypeB'
42010b3f80

# timings...
3202010b0c

# TypeB' ATTRIB
42 01 0f $ID

# Select ICC file
42 01 04 0a 00a4 0800 04 3f00 0002
#ICC:
42 01 06 06 00b2 0104 1d

# Select EnvHol file
42 01 08 0a 00a4 0800 04 2000 2001
#EnvHol1:
42 01 0a 06 00b2 0104 1d

# Select EvLog file
42 01 0c 0a 00a4 0800 04 2000 2010
#EvLog1:
42 01 0e 06 00b2 0104 1d
#EvLog2:
42 01 00 06 00b2 0204 1d
#EvLog3:
42 01 02 06 00b2 0304 1d

# Select ConList file
42 01 04 0a 00a4 0800 04 2000 2050
#ConList:
42 01 06 06 00b2 0104 1d

# Select Contra file
42 01 08 0a 00a4 0800 04 2000 2020
#Contra1:
42 01 0a 06 00b2 0104 1d
#Contra2:
42 01 0c 06 00b2 0204 1d
#Contra3:
42 01 0e 06 00b2 0304 1d
#Contra4:
42 01 00 06 00b2 0404 1d

# Select Counter file
42 01 02 0a 00a4 0800 04 2000 2069
#Counter:
42 01 04 06 00b2 0104 1d

# Select SpecEv file
42 01 06 0a 00a4 08 0004 2000 2040
#SpecEv1:
42 01 08 06 00b2 0104 1d

# TypeB' Disconnect
42 01 03

EOF
