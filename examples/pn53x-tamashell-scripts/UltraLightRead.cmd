02;                               // Get firmware version

// Reads content of a Mifare UltraLight
// PLEASE PUT ULTRALIGHT TAG NOW
4A  01  00;                       // 1 target requested

// Read memory content from address 4
40 01 30 00;                      Read 16 bytes from address 0x00
40 01 30 04;                      Read 16 bytes from address 0x04
40 01 30 08;                      Read 16 bytes from address 0x08
40 01 30 0C;                      Read 16 bytes from address 0x0C

