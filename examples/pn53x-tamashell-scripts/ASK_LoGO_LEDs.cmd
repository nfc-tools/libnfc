# To be used only on ASK LoGO readers!!!
# As we don't know how GPIO can be wired, it may hurt your hardware!!!

# P32=0 LED1
# P34=0 progressive field off
# SFR_P3:     0x..101011
08 ff b0 2b

p 100

# P32=0 LED1
# P31=0 LED2
# SFR_P3:     0x..101001
08 ff b0 29

p 100

# P32=0 LED1
# P31=0 LED2
# P30=0 P33=0 LED3
# SFR_P3:     0x..100000
08 ff b0 20

p 100

# P32=0 LED1
# P31=0 LED2
# P30=0 P33=0 LED3
# P35=0 LED4
# SFR_P3:     0x..000000
08 ff b0 00

p 100

# P32=0 LED1
# P31=0 LED2
# P30=0 P33=0 LED3
# SFR_P3:     0x..100000
08 ff b0 20

p 100

# P32=0 LED1
# P31=0 LED2
# SFR_P3:     0x..101001
08 ff b0 29

p 100

# P32=0 LED1
# SFR_P3:     0x..101011
08 ff b0 2b

p 100

# P32=0 LED1
# SFR_P3:     0x..101011
08 ff b0 2b
