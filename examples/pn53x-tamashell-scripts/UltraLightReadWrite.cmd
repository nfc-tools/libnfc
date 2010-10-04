02;                               Get firmware version

// Create NFC-Forum tag type2 with URL
// WARNING It burns the OTP bits of sector 3!!
// PLEASE PUT ULTRALIGHT TAG NOW
4A  01  00;                       1 target requested

// Clear memory from address 0x04
40 01 A2 04 00 00 00 00;          Write 4 bytes from address 0x04
40 01 A2 05 00 00 00 00;          Write 4 bytes from address 0x05
40 01 A2 06 00 00 00 00;          Write 4 bytes from address 0x06
40 01 A2 07 00 00 00 00;          Write 4 bytes from address 0x07
40 01 A2 08 00 00 00 00;          Write 4 bytes from address 0x08
40 01 A2 09 00 00 00 00;          Write 4 bytes from address 0x09
40 01 A2 0A 00 00 00 00;          Write 4 bytes from address 0x0A
40 01 A2 0B 00 00 00 00;          Write 4 bytes from address 0x0B
40 01 A2 0C 00 00 00 00;          Write 4 bytes from address 0x0C

// Read memory content from address 4
40 01 30 04;                      Read 16 bytes from address 0x04
40 01 30 08;                      Read 16 bytes from address 0x08
40 01 30 0C;                      Read 16 bytes from address 0x0C

// cf NFC-Forum Type 1 Tag Operation Specification TS
// Write @ address 0x03 (OTP):    NDEF, v1.0, 48 bytes, RW
40 01 A2 03 E1 10 06 00;
// Write @ address 0x04:          NDEF TLV, 15 bytes,...
// cf NFC-Forum NFC Data Exchange Format (NDEF) TS
//                                ...,MB,ME,SR,TNF=1 (wkt), typeL=1 byte
40 01 A2 04 03 0F D1 01;
// Write @ address 0x05:          payloadL=11 bytes,...
// cf NFC-Forum NFC Record Type Definition (RTD) TS
//                                ...,type=urn:nfc:wkt:U = URI
// cf NFC-Forum URI Record Type Definition TS
//                                ...,01=>URI id code=http://www.
//                                ...,"l"
40 01 A2 05 0B 55 01 6C;
// Write @ address 0x06:          "ibnf"
40 01 A2 06 69 62 6E 66;
// Write @ address 0x07:          "c.or"
40 01 A2 07 63 2E 6F 72;
// Write @ address 0x08:          "g",TLV:FE
40 01 A2 08 67 FE 00 00;

// Read memory content from address 4
40 01 30 04;                      Read 16 bytes from address 0x04
40 01 30 08;                      Read 16 bytes from address 0x08
40 01 30 0C;                      Read 16 bytes from address 0x0C

