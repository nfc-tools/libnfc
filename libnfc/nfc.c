/*-
 * Public platform independent Near Field Communication (NFC) library
 * 
 * Copyright (C) 2009, 2010, Roel Verdult, Romuald Conty
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
 * @file nfc.c
 * @brief NFC library implementation
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif // HAVE_CONFIG_H

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#include <nfc/nfc.h>

#ifdef _WIN32
#  include <windows.h>

#  define snprintf sprintf_s
#endif

#include "chips.h"
#include "drivers.h"

#include <nfc/nfc-messages.h>

nfc_device_desc_t *nfc_pick_device (void);

/**
 * @brief Probe for the first discoverable supported devices (ie. only available for some drivers)
 * @return \a nfc_device_desc_t struct pointer
 */
nfc_device_desc_t *
nfc_pick_device (void)
{
  uint32_t uiDriver;
  nfc_device_desc_t *nddRes;

  for (uiDriver = 0; uiDriver < sizeof (drivers_callbacks_list) / sizeof (drivers_callbacks_list[0]); uiDriver++) {
    if (drivers_callbacks_list[uiDriver].pick_device != NULL) {
      nddRes = drivers_callbacks_list[uiDriver].pick_device ();
      if (nddRes != NULL)
        return nddRes;
    }
  }

  return NULL;
}

/**
 * @brief Probe for discoverable supported devices (ie. only available for some drivers)
 * @param pnddDevices Array of nfc_device_desc_t previously allocated by the caller.
 * @param szDevices size of the pnddDevices array.
 * @param pszDeviceFound number of devices found.
 */
void
nfc_list_devices (nfc_device_desc_t pnddDevices[], size_t szDevices, size_t * pszDeviceFound)
{
  uint32_t uiDriver;
  size_t  szN;

  *pszDeviceFound = 0;

  for (uiDriver = 0; uiDriver < sizeof (drivers_callbacks_list) / sizeof (drivers_callbacks_list[0]); uiDriver++) {
    if (drivers_callbacks_list[uiDriver].list_devices != NULL) {
      szN = 0;
      if (drivers_callbacks_list[uiDriver].
          list_devices (pnddDevices + (*pszDeviceFound), szDevices - (*pszDeviceFound), &szN)) {
        *pszDeviceFound += szN;
        DBG ("%ld device(s) found using %s driver", (unsigned long) szN, drivers_callbacks_list[uiDriver].acDriver);
      }
    } else {
      DBG ("No listing function avaible for %s driver", drivers_callbacks_list[uiDriver].acDriver);
    }
  }
}

/**
 * @brief Connect to a NFC device
 * @param pndd Device description if specific device is wanted, NULL otherwise
 * @return Returns pointer to a nfc_device_t struct if successfull; otherwise returns NULL value.
 *
 * If \a pndd is NULL, the first available NFC device is claimed by libnfc.
 * It will automatically search the system using all available drivers to determine a device is free.
 *
 * If \a pndd is passed then libnfc will try to claim the right device using information provided by this struct.
 *
 * When it has successfully claimed a NFC device, memory is allocated to save the device information. It will return a pointer to a nfc_device_t struct.
 * This pointer should be supplied by every next function of libnfc that should perform an action with this device.
 */
nfc_device_t *
nfc_connect (nfc_device_desc_t * pndd)
{
  nfc_device_t *pnd = NULL;
  uint32_t uiDriver;

  // Search through the device list for an available device
  for (uiDriver = 0; uiDriver < sizeof (drivers_callbacks_list) / sizeof (drivers_callbacks_list[0]); uiDriver++) {
    if (pndd == NULL) {
      // No device description specified: try to automatically claim a device
      if (drivers_callbacks_list[uiDriver].pick_device != NULL) {
        DBG ("Autodetecting available devices using %s driver.", drivers_callbacks_list[uiDriver].acDriver);
        pndd = drivers_callbacks_list[uiDriver].pick_device ();

        if (pndd != NULL) {
          DBG ("Auto-connecting to %s using %s driver", pndd->acDevice, drivers_callbacks_list[uiDriver].acDriver);
          pnd = drivers_callbacks_list[uiDriver].connect (pndd);
          if (pnd == NULL) {
            DBG ("No device available using %s driver", drivers_callbacks_list[uiDriver].acDriver);
            pndd = NULL;
          }

          free (pndd);
        }
      }
    } else {
      // Specific device is requested: using device description pndd
      if (0 != strcmp (drivers_callbacks_list[uiDriver].acDriver, pndd->pcDriver)) {
        continue;
      } else {
        pnd = drivers_callbacks_list[uiDriver].connect (pndd);
      }
    }

    // Test if the connection was successful
    if (pnd != NULL) {
      DBG ("[%s] has been claimed.", pnd->acName);
      // Great we have claimed a device
      pnd->pdc = &(drivers_callbacks_list[uiDriver]);

      if (!pn53x_get_firmware_version (pnd))
        return NULL;

      // Reset the ending transmission bits register, it is unknown what the last tranmission used there
      if (!pn53x_set_reg (pnd, REG_CIU_BIT_FRAMING, SYMBOL_TX_LAST_BITS, 0x00))
        return NULL;

      // Set default configuration options
      // Make sure we reset the CRC and parity to chip handling.
      if (!nfc_configure (pnd, NDO_HANDLE_CRC, true))
        return NULL;
      if (!nfc_configure (pnd, NDO_HANDLE_PARITY, true))
        return NULL;

      // Deactivate the CRYPTO1 chiper, it may could cause problems when still active
      if (!nfc_configure (pnd, NDO_ACTIVATE_CRYPTO1, false))
        return NULL;

      // Activate "easy framing" feature by default
      if (!nfc_configure (pnd, NDO_EASY_FRAMING, true))
        return NULL;

      // Activate auto ISO14443-4 switching by default
      if (!nfc_configure (pnd, NDO_AUTO_ISO14443_4, true))
        return NULL;

      // Disallow invalid frame
      if (!nfc_configure (pnd, NDO_ACCEPT_INVALID_FRAMES, false))
        return NULL;

      // Disallow multiple frames
      if (!nfc_configure (pnd, NDO_ACCEPT_MULTIPLE_FRAMES, false))
        return NULL;

      return pnd;
    } else {
      DBG ("No device found using driver: %s", drivers_callbacks_list[uiDriver].acDriver);
    }
  }
  // Too bad, no reader is ready to be claimed
  return NULL;
}

/**
 * @brief Disconnect from a NFC device
 * @param pnd nfc_device_t struct pointer that represent currently used device
 *
 * Initiator is disconnected and the device, including allocated \a nfc_device_t struct, is released.
 */
void
nfc_disconnect (nfc_device_t * pnd)
{
  if (pnd) {
    // Release and deselect all active communications
    nfc_initiator_deselect_target (pnd);
    // Disable RF field to avoid heating
    nfc_configure (pnd, NDO_ACTIVATE_FIELD, false);
    // Disconnect, clean up and release the device 
    pnd->pdc->disconnect (pnd);
  }
}

/**
 * @brief Configure advanced NFC device settings
 * @return Returns true if action was successfully performed; otherwise returns false.
 * @param pnd nfc_device_t struct pointer that represent currently used device
 * @param ndo nfc_device_option_t struct that contains options to set to device
 * @param bEnable boolean
 *
 * Configures parameters and registers that control for example timing, modulation, frame and error handling.
 * There are different categories for configuring the PN53X chip features (handle, activate, infinite and accept).
 * These are defined to organize future settings that will become available when they are needed.
 */
bool
nfc_configure (nfc_device_t * pnd, const nfc_device_option_t ndo, const bool bEnable)
{
  pnd->iLastError = 0;

  return pn53x_configure (pnd, ndo, bEnable);
}


/**
 * @brief Initialize NFC device as initiator (reader)
 * @return Returns true if action was successfully performed; otherwise returns false.
 * @param pnd nfc_device_t struct pointer that represent currently used device
 *
 * The NFC device is configured to function as RFID reader.
 * After initialization it can be used to communicate to passive RFID tags and active NFC devices.
 * The reader will act as initiator to communicate peer 2 peer (NFCIP) to other active NFC devices.
 */
bool
nfc_initiator_init (nfc_device_t * pnd)
{
  pnd->iLastError = 0;

  // Make sure we are dealing with a active device
  if (!pnd->bActive)
    return false;

  // Set the PN53X to force 100% ASK Modified miller decoding (default for 14443A cards)
  if (!pn53x_set_reg (pnd, REG_CIU_TX_AUTO, SYMBOL_FORCE_100_ASK, 0x40))
    return false;

  // Configure the PN53X to be an Initiator or Reader/Writer
  if (!pn53x_set_reg (pnd, REG_CIU_CONTROL, SYMBOL_INITIATOR, 0x10))
    return false;

  return true;
}

/**
 * @brief Select a target and request active or passive mode for DEP (Data Exchange Protocol)
 * @return Returns true if action was successfully performed; otherwise returns false.
 * @param pnd nfc_device_t struct pointer that represent currently used device
 * @param nmInitModulation Desired modulation (NM_ACTIVE_DEP or NM_PASSIVE_DEP for active, respectively passive mode)
 * @param pbtPidData passive initiator data, 4 or 5 bytes long, (optional, only for NM_PASSIVE_DEP, can be NULL)
 * @param szPidDataLen size of \a pbtPidData
 * @param pbtNFCID3i the NFCID3, 10 bytes long, of the initiator (optional, can be NULL)
 * @param szNFCID3iDataLen size of \a pbtNFCID3i
 * @param pbtGbData generic data of the initiator, max 48 bytes long, (optional, can be NULL)
 * @param szGbDataLen size of \a pbtGbData
 * @param pnti is a \a nfc_target_info_t struct pointer where target information will be put.
 *
 * The NFC device will try to find the available target. The standards (ISO18092 and ECMA-340) describe the modulation that can be used for reader to passive communications.
 * @note nfc_dep_info_t will be returned when the target was acquired successfully.
 */
bool
nfc_initiator_select_dep_target (nfc_device_t * pnd, const nfc_modulation_t nmInitModulation, const byte_t * pbtPidData,
                                 const size_t szPidDataLen, const byte_t * pbtNFCID3i, const size_t szNFCID3iDataLen,
                                 const byte_t * pbtGbData, const size_t szGbDataLen, nfc_target_info_t * pnti)
{
  pnd->iLastError = 0;

  return pn53x_initiator_select_dep_target (pnd, nmInitModulation, pbtPidData, szPidDataLen, pbtNFCID3i,
                                            szNFCID3iDataLen, pbtGbData, szGbDataLen, pnti);
}

/**
 * @fn nfc_initiator_select_passive_target(const nfc_device_t* pnd, const nfc_modulation_t nmInitModulation, const byte_t* pbtInitData, const size_t szInitDataLen, nfc_target_info_t* pti)
 * @brief Select a passive or emulated tag
 * @return Returns true if action was successfully performed; otherwise returns false.
 *
 * @param pnd nfc_device_t struct pointer that represent currently used device
 * @param nmInitModulation Desired modulation
 * @param pbtInitData Optional initiator data used for Felica, ISO14443B, Topaz Polling or for ISO14443A selecting a specific UID.
 * @param szInitDataLen Length of initiator data \a pbtInitData.
 * @param pnti nfc_target_info_t struct pointer which will filled if available
 *
 * The NFC device will try to find the available passive tags. Some NFC devices are capable to emulate passive tags. The standards (ISO18092 and ECMA-340) describe the modulation that can be used for reader to passive communications. The chip needs to know with what kind of tag it is dealing with, therefore the initial modulation and speed (106, 212 or 424 kbps) should be supplied.
 * @note For every initial modulation type there is a different collection of information returned (in nfc_target_info_t pointer pti) They all fit in the data-type which is called nfc_target_info_t. This is a union which contains the tag information that belongs to the according initial modulation type.
 */
bool
nfc_initiator_select_passive_target (nfc_device_t * pnd,
                                     const nfc_modulation_t nmInitModulation,
                                     const byte_t * pbtInitData, const size_t szInitDataLen, nfc_target_info_t * pnti)
{
  byte_t  abtInit[MAX_FRAME_LEN];
  size_t  szInitLen;

  size_t  szTargetsData;
  byte_t  abtTargetsData[MAX_FRAME_LEN];

  pnd->iLastError = 0;

  // Make sure we are dealing with a active device
  if (!pnd->bActive)
    return false;
  // TODO Put this in a function
  switch (nmInitModulation) {
  case NM_ISO14443A_106:
    switch (szInitDataLen) {
    case 7:
      abtInit[0] = 0x88;
      memcpy (abtInit + 1, pbtInitData, 7);
      szInitLen = 8;
      break;

    case 10:
      abtInit[0] = 0x88;
      memcpy (abtInit + 1, pbtInitData, 3);
      abtInit[4] = 0x88;
      memcpy (abtInit + 5, pbtInitData + 3, 7);
      szInitLen = 12;
      break;

    case 4:
    default:
      memcpy (abtInit, pbtInitData, szInitDataLen);
      szInitLen = szInitDataLen;
      break;
    }
    break;

  default:
    memcpy (abtInit, pbtInitData, szInitDataLen);
    szInitLen = szInitDataLen;
    break;
  }

  if (!pn53x_InListPassiveTarget (pnd, nmInitModulation, 1, abtInit, szInitLen, abtTargetsData, &szTargetsData))
    return false;

  // Make sure one tag has been found, the PN53X returns 0x00 if none was available
  if (abtTargetsData[0] == 0)
    return false;

  // Is a tag info struct available
  if (pnti) {
    // Fill the tag info struct with the values corresponding to this init modulation
    switch (nmInitModulation) {
    case NM_ISO14443A_106:
      if (!pn53x_decode_target_data (abtTargetsData + 1, szTargetsData - 1, pnd->nc, NTT_GENERIC_PASSIVE_106, pnti)) {
        return false;
      }
      break;

    case NM_FELICA_212:
      if (!pn53x_decode_target_data (abtTargetsData + 1, szTargetsData - 1, pnd->nc, NTT_FELICA_212, pnti)) {
        return false;
      }
      break;
    case NM_FELICA_424:
      if (!pn53x_decode_target_data (abtTargetsData + 1, szTargetsData - 1, pnd->nc, NTT_FELICA_424, pnti)) {
        return false;
      }
      break;

    case NM_ISO14443B_106:
      if (!pn53x_decode_target_data (abtTargetsData + 1, szTargetsData - 1, pnd->nc, NTT_ISO14443B_106, pnti)) {
        return false;
      }
      break;

    case NM_JEWEL_106:
      if (!pn53x_decode_target_data (abtTargetsData + 1, szTargetsData - 1, pnd->nc, NTT_JEWEL_106, pnti)) {
        return false;
      }
      break;

    default:
      // Should not be possible, so whatever...
      break;
    }
  }
  return true;
}

bool
nfc_initiator_list_passive_targets (nfc_device_t * pnd, const nfc_modulation_t nmInitModulation,
                                    nfc_target_info_t anti[], const size_t szTargets, size_t * pszTargetFound)
{
  nfc_target_info_t nti;
  size_t  szTargetFound = 0;
  byte_t *pbtInitData = NULL;
  size_t  szInitDataLen = 0;

  pnd->iLastError = 0;

  // Let the reader only try once to find a target
  nfc_configure (pnd, NDO_INFINITE_SELECT, false);

  if (nmInitModulation == NM_ISO14443B_106) {
    // Application Family Identifier (AFI) must equals 0x00 in order to wakeup all ISO14443-B PICCs (see ISO/IEC 14443-3)
    pbtInitData = (byte_t *) "\x00";
    szInitDataLen = 1;
  }
  while (nfc_initiator_select_passive_target (pnd, nmInitModulation, pbtInitData, szInitDataLen, &nti)) {
    nfc_initiator_deselect_target (pnd);

    if (szTargets > szTargetFound) {
      memcpy (&(anti[szTargetFound]), &nti, sizeof (nfc_target_info_t));
    }
    szTargetFound++;
  }
  *pszTargetFound = szTargetFound;

  return true;
}

/**
 * @brief Deselect a selected passive or emulated tag
 * @return Returns true if action was successfully performed; otherwise returns false.
 * @param pnd nfc_device_t struct pointer that represent currently used device
 *
 * After selecting and communicating with a passive tag, this function could be used to deactivate and release the tag. This is very useful when there are multiple tags available in the field. It is possible to use the nfc_initiator_select_passive_target() function to select the first available tag, test it for the available features and support, deselect it and skip to the next tag until the correct tag is found.
 */
bool
nfc_initiator_deselect_target (nfc_device_t * pnd)
{
  pnd->iLastError = 0;

  return (pn53x_InDeselect (pnd, 0));   // 0 mean deselect all selected targets
}

/**
 * @brief Polling for NFC targets
 * @param pnd nfc_device_t struct pointer that represent currently used device
 * @param pnttTargetTypes array of desired target types
 * @param szTargetTypes pnttTargetTypes count
 * @param btPollNr specifies the number of polling
 * @note one polling is a polling for each desired target type
 * @param btPeriod indicates the polling period in units of 150 ms
 * @param pntTargets pointer on array of 2 nfc_target_t (over)writables struct
 * @param pszTargetFound found targets count
 */
bool
nfc_initiator_poll_targets (nfc_device_t * pnd,
                            const nfc_target_type_t * pnttTargetTypes, const size_t szTargetTypes,
                            const byte_t btPollNr, const byte_t btPeriod,
                            nfc_target_t * pntTargets, size_t * pszTargetFound)
{
  pnd->iLastError = 0;

  return pn53x_InAutoPoll (pnd, pnttTargetTypes, szTargetTypes, btPollNr, btPeriod, pntTargets, pszTargetFound);
}

/**
 * @brief Transceive raw bit-frames
 * @return Returns true if action was successfully performed; otherwise returns false.
 * @param pbtTx contains a byte array of the frame that needs to be transmitted.
 * @param szTxBits contains the length in bits.
 * @note For example the REQA (0x26) command (first anti-collision command of ISO14443-A) must be precise 7 bits long. This is not possible by using nfc_initiator_transceive_bytes(). With that function you can only communicate frames that consist of full bytes. When you send a full byte (8 bits + 1 parity) with the value of REQA (0x26), a tag will simply not respond. More information about this can be found in the anti-colision example.
 * @param pbtTxPar parameter contains a byte array of the corresponding parity bits needed to send per byte.
 * @note For example if you send the SELECT_ALL (0x93, 0x20) = [ 10010011, 00100000 ] command, you have to supply the following parity bytes (0x01, 0x00) to define the correct odd parity bits. This is only an example to explain how it works, if you just are sending two bytes with ISO14443-A compliant parity bits you better can use the nfc_initiator_transceive_bytes() function.
 * @returns The received response from the tag will be stored in the parameters (pbtRx, pszRxBits and pbtRxPar). They work the same way as the corresponding parameters for transmission.
 *
 * The NFC reader will transmit low-level messages where only the modulation is handled by the PN53X chip. Construction of the frame (data, CRC and parity) is completely done by libnfc. This can be very useful for testing purposes. Some protocols (e.g. MIFARE Classic) require to violate the ISO14443-A standard by sending incorrect parity and CRC bytes. Using this feature you are able to simulate these frames.
 */
bool
nfc_initiator_transceive_bits (nfc_device_t * pnd, const byte_t * pbtTx, const size_t szTxBits, const byte_t * pbtTxPar,
                               byte_t * pbtRx, size_t * pszRxBits, byte_t * pbtRxPar)
{
  pnd->iLastError = 0;

  return pn53x_initiator_transceive_bits (pnd, pbtTx, szTxBits, pbtTxPar, pbtRx, pszRxBits, pbtRxPar);
}

/**
 * @brief Send raw data to target then retrieve raw data from target
 * @return Returns true if action was successfully performed; otherwise returns false.
 *
 * The reader will transmit the supplied bytes (\a pbtTx) to the target in raw mode: PN53x will not handle input neither output data.
 * It waits for the response and stores the received bytes in the pbtRx byte array.
 * The parity bits are handled by the PN53X chip. The CRC can be generated automatically or handled manually.
 * Using this function, frames can be communicated very fast via the NFC reader to the tag.
 *
 * Tests show that on average this way of communicating is much faster than using the regular driver/middle-ware (often supplied by manufacturers).
 *
 * @warning The configuration option NDO_HANDLE_PARITY must be set to true (the default value).
 */
bool
nfc_initiator_transceive_bytes (nfc_device_t * pnd, const byte_t * pbtTx, const size_t szTxLen, byte_t * pbtRx,
                                size_t * pszRxLen)
{
  pnd->iLastError = 0;

  return pn53x_initiator_transceive_bytes (pnd, pbtTx, szTxLen, pbtRx, pszRxLen);
}

/**
 * @brief Initialize NFC device as an emulated tag
 * @return Returns true if action was successfully performed; otherwise returns false.
 *
 * This functionality allows the NFC device to act as an emulated tag. There seems to be quite some options available for this feature. Not all of the PN53X modulations are tested and documented at the moment. At the moment it could best be seen as a preliminary functionality.
 *
 * @warning Be aware that this function will wait (hang) until a command is received that is not part of the anti-collision. The RATS command for example would wake up the emulator. After this is received, the send and receive functions can be used.
 */
bool
nfc_target_init (nfc_device_t * pnd, byte_t * pbtRx, size_t * pszRxBits)
{
  pnd->iLastError = 0;

  return pn53x_target_init (pnd, pbtRx, pszRxBits);
}

/**
 * @brief Receive bit-frames
 * @return Returns true if action was successfully performed; otherwise returns false.
 *
 * This function makes it possible to receive (raw) bit-frames. It returns all the messages that are stored in the FIFO buffer of the PN53X chip. It does not require to send any frame and thereby could be used to snoop frames that are transmitted by a nearby reader. Check out the NDO_ACCEPT_MULTIPLE_FRAMES configuration option to avoid losing transmitted frames.
 */
bool
nfc_target_receive_bits (nfc_device_t * pnd, byte_t * pbtRx, size_t * pszRxBits, byte_t * pbtRxPar)
{
  pnd->iLastError = 0;

  return pn53x_target_receive_bits (pnd, pbtRx, pszRxBits, pbtRxPar);
}

/**
 * @brief Receive bytes and APDU frames
 * @return Returns true if action was successfully performed; otherwise returns false.
 *
 * The main receive function that returns the received frames from a nearby reader.
 */
bool
nfc_target_receive_bytes (nfc_device_t * pnd, byte_t * pbtRx, size_t * pszRxLen)
{
  pnd->iLastError = 0;

  return pn53x_target_receive_bytes (pnd, pbtRx, pszRxLen);
}

/**
 * @brief Send raw bit-frames
 * @return Returns true if action was successfully performed; otherwise returns false.
 *
 * This function can be used to transmit (raw) bit-frames to the reader.
 */
bool
nfc_target_send_bits (nfc_device_t * pnd, const byte_t * pbtTx, const size_t szTxBits, const byte_t * pbtTxPar)
{
  pnd->iLastError = 0;

  return pn53x_target_send_bits (pnd, pbtTx, szTxBits, pbtTxPar);
}


/**
 * @brief Send bytes and APDU frames
 * @return Returns true if action was successfully performed; otherwise returns false.
 *
 * To communicate byte frames and APDU responses to the reader, this function could be used.
 */
bool
nfc_target_send_bytes (nfc_device_t * pnd, const byte_t * pbtTx, const size_t szTxLen)
{
  pnd->iLastError = 0;

  return pn53x_target_send_bytes (pnd, pbtTx, szTxLen);
}

/**
 * @brief Return the PCD error string
 * @return Returns a string
 */
const char *
nfc_strerror (const nfc_device_t * pnd)
{
  return pnd->pdc->pcc->strerror (pnd);
}

/**
 * @brief Renders the PCD error in pcStrErrBuf for a maximum size of szBufLen chars
 * @return Returns 0 upon success
 */
int
nfc_strerror_r (const nfc_device_t * pnd, char *pcStrErrBuf, size_t szBufLen)
{
  return (snprintf (pcStrErrBuf, szBufLen, "%s", nfc_strerror (pnd)) < 0) ? -1 : 0;
}

/**
 * @brief Display the PCD error a-la perror
 */
void
nfc_perror (const nfc_device_t * pnd, const char *pcString)
{
  fprintf (stderr, "%s: %s\n", pcString, nfc_strerror (pnd));
}

/* Special data accessors */

/**
 * @brief Returns the device name
 * @return Returns a string with the device name
 */
const char *
nfc_device_name (nfc_device_t * pnd)
{
  return pnd->acName;
}

/* Misc. functions */

/**
 * @brief Returns the library version
 * @return Returns a string with the library version
 */
const char *
nfc_version (void)
{
#ifdef SVN_REVISION
  return PACKAGE_VERSION " (r" SVN_REVISION ")";
#else
  return PACKAGE_VERSION;
#endif // SVN_REVISION
}
