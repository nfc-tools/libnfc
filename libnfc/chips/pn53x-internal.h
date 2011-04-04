/*-
 * Public platform independent Near Field Communication (NFC) library
 * 
 * Copyright (C) 2011, Romuald Conty, Romain Tartière
 * 
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 */

/**
 * @file pn53x-internal.h
 * @brief PN531, PN532 and PN533 defines and compatibility
 */

#ifndef __PN53X_INTERNAL_H__
#define __PN53X_INTERNAL_H__

#include "pn53x.h"

// Miscellaneous
#define Diagnose 0x00
#define GetFirmwareVersion 0x02
#define GetGeneralStatus 0x04
#define ReadRegister 0x06
#define WriteRegister 0x08
#define ReadGPIO 0x0C
#define WriteGPIO 0x0E
#define SetSerialBaudRate 0x10
#define SetParameters 0x12
#define SAMConfiguration 0x14
#define PowerDown 0x16
#define AlparCommandForTDA 0x18

// RF communication
#define RFConfiguration 0x32
#define RFRegulationTest 0x58

// Initiator
#define InJumpForDEP 0x56
#define InJumpForPSL 0x46
#define InListPassiveTarget 0x4A
#define InATR 0x50
#define InPSL 0x4E
#define InDataExchange 0x40
#define InCommunicateThru 0x42
#define InQuartetByteExchange 0x38
#define InDeselect 0x44
#define InRelease 0x52
#define InSelect 0x54
#define InActivateDeactivatePaypass 0x48
#define InAutoPoll 0x60

// Target
#define TgInitAsTarget 0x8C
#define TgSetGeneralBytes 0x92
#define TgGetData 0x86
#define TgSetData 0x8E
#define TgSetDataSecure 0x96
#define TgSetMetaData 0x94
#define TgSetMetaDataSecure 0x98
#define TgGetInitiatorCommand 0x88
#define TgResponseToInitiator 0x90
#define TgGetTargetStatus 0x8A

/** @note PN53x's normal frame:
 * 
 *   .-- Start
 *   |   .-- Packet lenght
 *   |   |  .-- Lenght checksum
 *   |   |  |  .-- Direction (D4 Host to PN, D5 PN to Host)
 *   |   |  |  |  .-- Code
 *   |   |  |  |  |  .-- Packet checksum
 *   |   |  |  |  |  |  .-- Postamble 
 *   V   |  |  |  |  |  |
 * ----- V  V  V  V  V  V
 * 00 FF 02 FE D4 02 2A 00
 */

/** @note PN53x's extended frame:
 * 
 *   .-- Start
 *   |     .-- Fixed to FF to enable extended frame
 *   |     |     .-- Packet lenght
 *   |     |     |   .-- Lenght checksum
 *   |     |     |   |  .-- Direction (D4 Host to PN, D5 PN to Host)
 *   |     |     |   |  |  .-- Code
 *   |     |     |   |  |  |  .-- Packet checksum
 *   |     |     |   |  |  |  |  .-- Postamble 
 *   V     V     V   |  |  |  |  |
 * ----- ----- ----- V  V  V  V  V
 * 00 FF FF FF 00 02 FE D4 02 2A 00
 */

/** 
 * Start bytes, packet lenght, lenght checksum, direction, packet checksum and postamble are overhead
 */
// The TFI is considered part of the overhead
#  define PN53x_NORMAL_FRAME__DATA_MAX_LEN		254
#  define PN53x_NORMAL_FRAME__OVERHEAD			8
#  define PN53x_EXTENDED_FRAME__DATA_MAX_LEN		264
#  define PN53x_EXTENDED_FRAME__OVERHEAD		11

typedef struct {
  uint8_t ui8Code;
  uint8_t ui8CompatFlags;
#ifdef DEBUG
  const char * abtCommandText;
#endif
} pn53x_command;

/*
#define PN531 0x01
#define PN532 0x02
#define PN533 0X04
*/

#ifndef DEBUG
#  define PNCMD( X, Y ) { X , Y }
#  define PNCMD_DBG( X ) do { \
   } while(0)
#else
#  define PNCMD( X, Y ) { X , Y, #X }
#  define PNCMD_DBG( X ) do { \
     for (size_t i=0; i<(sizeof(pn53x_commands)/sizeof(pn53x_command)); i++) { \
       if ( X == pn53x_commands[i].ui8Code ) { \
         DBG( "%s", pn53x_commands[i].abtCommandText ); \
         break; \
       } \
     } \
   } while(0)
#endif

static const pn53x_command pn53x_commands[] = {
  // Miscellaneous
  PNCMD( Diagnose, PN531|PN532|PN533 ),
  PNCMD( GetFirmwareVersion, PN531|PN532|PN533 ),
  PNCMD( GetGeneralStatus, PN531|PN532|PN533 ),
  PNCMD( ReadRegister, PN531|PN532|PN533 ),
  PNCMD( WriteRegister, PN531|PN532|PN533 ),
  PNCMD( ReadGPIO, PN531|PN532|PN533 ),
  PNCMD( WriteGPIO, PN531|PN532|PN533 ),
  PNCMD( SetSerialBaudRate, PN531|PN532|PN533 ),
  PNCMD( SetParameters, PN531|PN532|PN533 ),
  PNCMD( SAMConfiguration, PN531|PN532 ),
  PNCMD( PowerDown, PN531|PN532 ),
  PNCMD( AlparCommandForTDA, PN533 ),
  
  // RF communication
  PNCMD( RFConfiguration, PN531|PN532|PN533 ),
  PNCMD( RFRegulationTest, PN531|PN532|PN533 ),

  // Initiator
  PNCMD( InJumpForDEP, PN531|PN532|PN533 ),
  PNCMD( InJumpForPSL, PN531|PN532|PN533 ),
  PNCMD( InListPassiveTarget, PN531|PN532|PN533 ),
  PNCMD( InATR, PN531|PN532|PN533 ),
  PNCMD( InPSL, PN531|PN532|PN533 ),
  PNCMD( InDataExchange, PN531|PN532|PN533 ),
  PNCMD( InCommunicateThru, PN531|PN532|PN533 ),
  PNCMD( InQuartetByteExchange, PN533 ),
  PNCMD( InDeselect, PN531|PN532|PN533 ),
  PNCMD( InRelease, PN531|PN532|PN533 ),
  PNCMD( InSelect, PN531|PN532|PN533 ),
  PNCMD( InAutoPoll, PN532 ),
  PNCMD( InActivateDeactivatePaypass, PN533 ),

  // Target
  PNCMD( TgInitAsTarget, PN531|PN532|PN533 ),
  PNCMD( TgSetGeneralBytes, PN531|PN532|PN533 ),
  PNCMD( TgGetData, PN531|PN532|PN533 ),
  PNCMD( TgSetData, PN531|PN532|PN533 ),
  PNCMD( TgSetDataSecure, PN533 ),
  PNCMD( TgSetMetaData, PN531|PN532|PN533 ),
  PNCMD( TgSetMetaDataSecure, PN533 ),
  PNCMD( TgGetInitiatorCommand, PN531|PN532|PN533 ),
  PNCMD( TgResponseToInitiator, PN531|PN532|PN533 ),
  PNCMD( TgGetTargetStatus, PN531|PN532|PN533 ),
};

#define _BV( X ) (1 << X)

#define P30 0
#define P31 1
#define P32 2
#define P33 3
#define P34 4
#define P35 5

#endif /* __PN53X_INTERNAL_H__ */
