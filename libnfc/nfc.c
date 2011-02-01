/*-
 * Public platform independent Near Field Communication (NFC) library
 * 
 * Copyright (C) 2009, Roel Verdult, Romuald Conty
 * Copyright (C) 2010, Roel Verdult, Romuald Conty, Romain Tartière
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
#  include "../contrib/windows.h"
#endif

#include "chips.h"
#include "drivers.h"

#include <nfc/nfc-messages.h>

nfc_device_desc_t *nfc_pick_device (void);

/**
 * @brief Connect to a NFC device
 * @param pndd device description if specific device is wanted, \c NULL otherwise
 * @return Returns pointer to a \a nfc_device_t struct if successfull; otherwise returns \c NULL value.
 *
 * If \e pndd is \c NULL, the first available NFC device is claimed.
 * It will automatically search the system using all available drivers to determine a device is NFC-enabled.
 *
 * If \e pndd is passed then this function will try to claim the right device using information provided by the \a nfc_device_desc_t struct.
 *
 * When it has successfully claimed a NFC device, memory is allocated to save the device information. It will return a pointer to a \a nfc_device_t struct.
 * This pointer should be supplied by every next functions of libnfc that should perform an action with this device.
 *
 * @note During this function, the device will be configured with default options:
 * - Crc is handled by the device (NDO_HANDLE_CRC = true)
 * - Parity is handled the device (NDO_HANDLE_PARITY = true)
 * - Cryto1 cipher is disabled (NDO_ACTIVATE_CRYPTO1 = false)
 * - Easy framing is enabled (NDO_EASY_FRAMING = true)
 * - Auto-switching in ISO14443-4 mode is enabled (NDO_AUTO_ISO14443_4 = true)
 * - Invalid frames are not accepted (NDO_ACCEPT_INVALID_FRAMES = false)
 * - Multiple frames are not accepted (NDO_ACCEPT_MULTIPLE_FRAMES = false)
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

      // TODO: Put this pn53x related in driver_init()
      if (!pn53x_init (pnd))
        return NULL;

      if (pnd->pdc->init) {
        pnd->pdc->init (pnd);
      }

      // Set default configuration options
      // Make sure we reset the CRC and parity to chip handling.
      if (!nfc_configure (pnd, NDO_HANDLE_CRC, true))
        return NULL;
      if (!nfc_configure (pnd, NDO_HANDLE_PARITY, true))
        return NULL;

      // Deactivate the CRYPTO1 cipher, it may could cause problems when still active
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
 * @param pnd \a nfc_device_t struct pointer that represent currently used device
 *
 * Initiator's selected tag is disconnected and the device, including allocated \a nfc_device_t struct, is released.
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
 * @param[out] pnddDevices array of \a nfc_device_desc_t previously allocated by the caller.
 * @param szDevices size of the \a pnddDevices array.
 * @param[out] pszDeviceFound number of devices found.
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
      if (drivers_callbacks_list[uiDriver].list_devices
          (pnddDevices + (*pszDeviceFound), szDevices - (*pszDeviceFound), &szN)) {
        *pszDeviceFound += szN;
        DBG ("%ld device(s) found using %s driver", (unsigned long) szN, drivers_callbacks_list[uiDriver].acDriver);
      }
    } else {
      DBG ("No listing function avaible for %s driver", drivers_callbacks_list[uiDriver].acDriver);
    }
  }
}

/**
 * @brief Configure advanced NFC device settings
 * @return Returns \c true if action was successfully performed; otherwise returns \c false.
 * @param pnd \a nfc_device_t struct pointer that represent currently used device
 * @param ndo \a nfc_device_option_t struct that contains option to set to device
 * @param bEnable boolean to activate/disactivate the option
 *
 * Configures parameters and registers that control for example timing,
 * modulation, frame and error handling.  There are different categories for
 * configuring the \e PN53X chip features (handle, activate, infinite and
 * accept).
 */
bool
nfc_configure (nfc_device_t * pnd, const nfc_device_option_t ndo, const bool bEnable)
{
  pnd->iLastError = 0;

  return pn53x_configure (pnd, ndo, bEnable);
}

/**
 * @brief Initialize NFC device as initiator (reader)
 * @return Returns \c true if action was successfully performed; otherwise returns \c false.
 * @param pnd \a nfc_device_t struct pointer that represent currently used device
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
 * @brief Select a passive or emulated tag
 * @return Returns \c true if action was successfully performed; otherwise returns \c false.
 *
 * @param pnd \a nfc_device_t struct pointer that represent currently used device
 * @param nm desired modulation
 * @param pbtInitData optional initiator data used for Felica, ISO14443B, Topaz polling or to select a specific UID in ISO14443A.
 * @param szInitData length of initiator data \a pbtInitData.
 * @note pbtInitData is used with different kind of data depending on modulation type:
 * - for an ISO/IEC 14443 type A modulation, pbbInitData contains the UID you want to select;
 * - for an ISO/IEC 14443 type B modulation, pbbInitData contains Application Family Identifier (AFI) (see ISO/IEC 14443-3);
 * - for a FeliCa modulation, pbbInitData contains polling payload (see ISO/IEC 18092 11.2.2.5).
 *
 * @param[out] pnt \a nfc_target_t struct pointer which will filled if available
 *
 * The NFC device will try to find one available passive tag or emulated tag. 
 *
 * The chip needs to know with what kind of tag it is dealing with, therefore
 * the initial modulation and speed (106, 212 or 424 kbps) should be supplied.
 */
bool
nfc_initiator_select_passive_target (nfc_device_t * pnd,
                                     const nfc_modulation_t nm,
                                     const byte_t * pbtInitData, const size_t szInitData,
                                     nfc_target_t * pnt)
{
  byte_t  abtInit[MAX_FRAME_LEN];
  size_t  szInit;

  pnd->iLastError = 0;

  // Make sure we are dealing with a active device
  if (!pnd->bActive)
    return false;
  // TODO Put this in a function: this part is defined by ISO14443-3 (UID and Cascade levels)
  switch (nm.nmt) {
  case NMT_ISO14443A:
    switch (szInitData) {
    case 7:
      abtInit[0] = 0x88;
      memcpy (abtInit + 1, pbtInitData, 7);
      szInit = 8;
      break;

    case 10:
      abtInit[0] = 0x88;
      memcpy (abtInit + 1, pbtInitData, 3);
      abtInit[4] = 0x88;
      memcpy (abtInit + 5, pbtInitData + 3, 7);
      szInit = 12;
      break;

    case 4:
    default:
      memcpy (abtInit, pbtInitData, szInitData);
      szInit = szInitData;
      break;
    }
    break;

  default:
    memcpy (abtInit, pbtInitData, szInitData);
    szInit = szInitData;
    break;
  }

  return pn53x_initiator_select_passive_target (pnd, nm, abtInit, szInit, pnt);
}

/**
 * @brief List passive or emulated tags
 * @return Returns \c true if action was successfully performed; otherwise returns \c false.
 *
 * @param pnd \a nfc_device_t struct pointer that represent currently used device
 * @param nm desired modulation
 * @param[out] ant array of \a nfc_target_t that will be filled with targets info
 * @param szTargets size of \a ant (will be the max targets listed)
 * @param[out] pszTargetFound pointer where target found counter will be stored
 *
 * The NFC device will try to find the available passive tags. Some NFC devices
 * are capable to emulate passive tags. The standards (ISO18092 and ECMA-340)
 * describe the modulation that can be used for reader to passive
 * communications. The chip needs to know with what kind of tag it is dealing
 * with, therefore the initial modulation and speed (106, 212 or 424 kbps)
 * should be supplied.
 */
bool
nfc_initiator_list_passive_targets (nfc_device_t * pnd,
                                    const nfc_modulation_t nm,
                                    nfc_target_t ant[], const size_t szTargets, size_t * pszTargetFound)
{
  nfc_target_t nt;
  size_t  szTargetFound = 0;
  byte_t *pbtInitData = NULL;
  size_t  szInitDataLen = 0;

  pnd->iLastError = 0;

  // Drop the field for a while
  if (!nfc_configure (pnd, NDO_ACTIVATE_FIELD, false)) {
    return false;
  }
  // Let the reader only try once to find a tag
  if (!nfc_configure (pnd, NDO_INFINITE_SELECT, false)) {
    return false;
  }
  // Enable field so more power consuming cards can power themselves up
  if (!nfc_configure (pnd, NDO_ACTIVATE_FIELD, true)) {
    return false;
  }

  switch (nm.nmt) {
    case NMT_ISO14443B: {
      // Application Family Identifier (AFI) must equals 0x00 in order to wakeup all ISO14443-B PICCs (see ISO/IEC 14443-3)
      pbtInitData = (byte_t *) "\x00";
      szInitDataLen = 1;
    }
    break;
    case NMT_FELICA: {
      // polling payload must be present (see ISO/IEC 18092 11.2.2.5)
      pbtInitData = (byte_t *) "\x00\xff\xff\x01\x00";
      szInitDataLen = 5;
    }
    break;
    default:
      // nothing to do
    break;
  }

  while (nfc_initiator_select_passive_target (pnd, nm, pbtInitData, szInitDataLen, &nt)) {
    nfc_initiator_deselect_target (pnd);

    if (szTargets > szTargetFound) {
      memcpy (&(ant[szTargetFound]), &nt, sizeof (nfc_target_t));
    } else {
      break;
    }
    szTargetFound++;
    // deselect has no effect on FeliCa and Jewel cards so we'll stop after one...
    if ((nm.nmt == NMT_FELICA) || (nm.nmt == NMT_JEWEL)) {
        break;
    }
  }
  *pszTargetFound = szTargetFound;

  return true;
}

/**
 * @brief Polling for NFC targets
 * @return Returns \c true if action was successfully performed; otherwise returns \c false.
 *
 * @param pnd \a nfc_device_t struct pointer that represent currently used device
 * @param ppttTargetTypes array of desired target types
 * @param szTargetTypes \e ppttTargetTypes count
 * @param btPollNr specifies the number of polling
 * @note one polling is a polling for each desired target type
 * @param btPeriod indicates the polling period in units of 150 ms
 * @param[out] pntTargets pointer on array of 2 \a nfc_target_t (over)writables struct
 * @param[out] pszTargetFound found targets count
 */
bool
nfc_initiator_poll_targets (nfc_device_t * pnd,
                            const nfc_modulation_t * pnmModulations, const size_t szModulations,
                            const byte_t btPollNr, const byte_t btPeriod,
                            nfc_target_t * pntTargets, size_t * pszTargetFound)
{
  pnd->iLastError = 0;

  return pn53x_initiator_poll_targets (pnd, pnmModulations, szModulations, btPollNr, btPeriod, pntTargets, pszTargetFound);
}


/**
 * @brief Select a target and request active or passive mode for D.E.P. (Data Exchange Protocol)
 * @return Returns \c true if action was successfully performed; otherwise returns \c false.
 *
 * @param pnd \a nfc_device_t struct pointer that represent currently used device
 * @param ndm desired D.E.P. mode (\a NDM_ACTIVE or \a NDM_PASSIVE for active, respectively passive mode)
 * @param ndiInitiator pointer \a nfc_dep_info_t struct that contains \e NFCID3 and \e General \e Bytes to set to the initiator device (optionnal, can be \e NULL)
 * @param[out] pnt is a \a nfc_target_t struct pointer where target information will be put.
 *
 * The NFC device will try to find an available D.E.P. target. The standards
 * (ISO18092 and ECMA-340) describe the modulation that can be used for reader
 * to passive communications.
 * 
 * @note \a nfc_dep_info_t will be returned when the target was acquired successfully.
 */
bool
nfc_initiator_select_dep_target (nfc_device_t * pnd, 
                                 const nfc_dep_mode_t ndm, const nfc_baud_rate_t nbr,
                                 const nfc_dep_info_t * pndiInitiator, nfc_target_t * pnt)
{
  pnd->iLastError = 0;

  return pn53x_initiator_select_dep_target (pnd, ndm, nbr, pndiInitiator, pnt);
}

/**
 * @brief Deselect a selected passive or emulated tag
 * @return Returns \c true if action was successfully performed; otherwise returns \c false.
 * @param pnd \a nfc_device_t struct pointer that represents currently used device
 *
 * After selecting and communicating with a passive tag, this function could be
 * used to deactivate and release the tag. This is very useful when there are
 * multiple tags available in the field. It is possible to use the \fn
 * nfc_initiator_select_passive_target() function to select the first available
 * tag, test it for the available features and support, deselect it and skip to
 * the next tag until the correct tag is found.
 */
bool
nfc_initiator_deselect_target (nfc_device_t * pnd)
{
  pnd->iLastError = 0;

  return (pn53x_InDeselect (pnd, 0));   // 0 mean deselect all selected targets
}

/**
 * @brief Send data to target then retrieve data from target
 * @return Returns \c true if action was successfully performed; otherwise returns \c false.
 *
 * The NFC device (configured as initiator) will transmit the supplied bytes (\a pbtTx) to the target.
 * It waits for the response and stores the received bytes in the \a pbtRx byte array.
 *
 * If \a NDO_EASY_FRAMING option is disabled the frames will sent and received in raw mode: \e PN53x will not handle input neither output data.
 *
 * The parity bits are handled by the \e PN53x chip. The CRC can be generated automatically or handled manually.
 * Using this function, frames can be communicated very fast via the NFC initiator to the tag.
 *
 * Tests show that on average this way of communicating is much faster than using the regular driver/middle-ware (often supplied by manufacturers).
 *
 * @warning The configuration option \a NDO_HANDLE_PARITY must be set to \c true (the default value).
 */
bool
nfc_initiator_transceive_bytes (nfc_device_t * pnd, const byte_t * pbtTx, const size_t szTx, byte_t * pbtRx,
                                size_t * pszRx)
{
  pnd->iLastError = 0;

  return pn53x_initiator_transceive_bytes (pnd, pbtTx, szTx, pbtRx, pszRx);
}

/**
 * @brief Transceive raw bit-frames to a target
 * @return Returns \c true if action was successfully performed; otherwise returns \c false.
 *
 * @param pbtTx contains a byte array of the frame that needs to be transmitted.
 * @param szTxBits contains the length in bits.
 *
 * @note For example the REQA (0x26) command (first anti-collision command of
 * ISO14443-A) must be precise 7 bits long. This is not possible by using
 * nfc_initiator_transceive_bytes(). With that function you can only
 * communicate frames that consist of full bytes. When you send a full byte (8
 * bits + 1 parity) with the value of REQA (0x26), a tag will simply not
 * respond. More information about this can be found in the anti-collision
 * example (\e nfc-anticol).
 *
 * @param pbtTxPar parameter contains a byte array of the corresponding parity bits needed to send per byte.
 *
 * @note For example if you send the SELECT_ALL (0x93, 0x20) = [ 10010011,
 * 00100000 ] command, you have to supply the following parity bytes (0x01,
 * 0x00) to define the correct odd parity bits. This is only an example to
 * explain how it works, if you just are sending two bytes with ISO14443-A
 * compliant parity bits you better can use the
 * nfc_initiator_transceive_bytes() function.
 * 
 * @param[out] pbtRx response from the tag
 * @param[out] pszRxBits \a pbtRx length in bits
 * @param[out] pbtRxPar parameter contains a byte array of the corresponding parity bits
 *
 * The NFC device (configured as \e initiator) will transmit low-level messages
 * where only the modulation is handled by the \e PN53x chip. Construction of
 * the frame (data, CRC and parity) is completely done by libnfc.  This can be
 * very useful for testing purposes. Some protocols (e.g. MIFARE Classic)
 * require to violate the ISO14443-A standard by sending incorrect parity and
 * CRC bytes. Using this feature you are able to simulate these frames.
 */
bool
nfc_initiator_transceive_bits (nfc_device_t * pnd, const byte_t * pbtTx, const size_t szTxBits, const byte_t * pbtTxPar,
                               byte_t * pbtRx, size_t * pszRxBits, byte_t * pbtRxPar)
{
  pnd->iLastError = 0;

  return pn53x_initiator_transceive_bits (pnd, pbtTx, szTxBits, pbtTxPar, pbtRx, pszRxBits, pbtRxPar);
}

/**
 * @brief Initialize NFC device as an emulated tag
 * @return Returns \c true if action was successfully performed; otherwise returns \c false.
 *
 * @param pnd \a nfc_device_t struct pointer that represent currently used device
 * @param ntm target mode restriction that you want to emulate (eg. NTM_PASSIVE_ONLY)
 * @param pnt pointer to \a nfc_target_t struct that represents the wanted emulated target
 *
 * @note \a pnt can be updated by this function: if you set NBR_UNDEFINED
 * and/or NDM_UNDEFINED (ie. for DEP mode), these fields will be updated.
 *
 * @param[out] pbtRx Rx buffer pointer
 * @param[out] pszRx received bytes count
 *
 * This function initialize NFC device in \e target mode in order to emulate a
 * tag using the specified \a nfc_target_mode_t.
 *
 * @warning Be aware that this function will wait (hang) until a command is
 * received that is not part of the anti-collision. The RATS command for
 * example would wake up the emulator. After this is received, the send and
 * receive functions can be used.
 */
bool
nfc_target_init (nfc_device_t * pnd, nfc_target_t * pnt, byte_t * pbtRx, size_t * pszRx)
{
  pnd->iLastError = 0;

  return pn53x_target_init (pnd, pnt, pbtRx, pszRx);
}

/**
 * @brief Send bytes and APDU frames
 * @return Returns \c true if action was successfully performed; otherwise returns \c false.
 *
 * @param pnd \a nfc_device_t struct pointer that represent currently used device
 * @param pbtTx pointer to Tx buffer
 * @param szTx size of Tx buffer
 *
 * This function make the NFC device (configured as \e target) send byte frames
 * (e.g. APDU responses) to the \e initiator.
 */
bool
nfc_target_send_bytes (nfc_device_t * pnd, const byte_t * pbtTx, const size_t szTx)
{
  pnd->iLastError = 0;

  return pn53x_target_send_bytes (pnd, pbtTx, szTx);
}

/**
 * @brief Receive bytes and APDU frames
 * @return Returns \c true if action was successfully performed; otherwise returns \c false.
 * @param pnd \a nfc_device_t struct pointer that represent currently used device
 * @param[out] pbtRx pointer to Rx buffer
 * @param[out] pszRx received byte count
 *
 * This function retrieves bytes frames (e.g. ADPU) sent by the \e initiator to the NFC device (configured as \e target).
 */
bool
nfc_target_receive_bytes (nfc_device_t * pnd, byte_t * pbtRx, size_t * pszRx)
{
  pnd->iLastError = 0;

  return pn53x_target_receive_bytes (pnd, pbtRx, pszRx);
}

/**
 * @brief Send raw bit-frames
 * @return Returns \c true if action was successfully performed; otherwise returns \c false.
 *
 * This function can be used to transmit (raw) bit-frames to the \e initiator
 * using the specified NFC device (configured as \e target).
 */
bool
nfc_target_send_bits (nfc_device_t * pnd, const byte_t * pbtTx, const size_t szTxBits, const byte_t * pbtTxPar)
{
  pnd->iLastError = 0;

  return pn53x_target_send_bits (pnd, pbtTx, szTxBits, pbtTxPar);
}

/**
 * @brief Receive bit-frames
 * @return Returns \c true if action was successfully performed; otherwise returns \c false.
 *
 * This function makes it possible to receive (raw) bit-frames.  It returns all
 * the messages that are stored in the FIFO buffer of the \e PN53x chip.  It
 * does not require to send any frame and thereby could be used to snoop frames
 * that are transmitted by a nearby \e initiator.  @note Check out the
 * NDO_ACCEPT_MULTIPLE_FRAMES configuration option to avoid losing transmitted
 * frames.
 */
bool
nfc_target_receive_bits (nfc_device_t * pnd, byte_t * pbtRx, size_t * pszRxBits, byte_t * pbtRxPar)
{
  pnd->iLastError = 0;

  return pn53x_target_receive_bits (pnd, pbtRx, pszRxBits, pbtRxPar);
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
