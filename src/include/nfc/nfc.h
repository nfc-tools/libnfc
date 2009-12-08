/**
 * Public platform independent Near Field Communication (NFC) library
 * 
 * Copyright (C) 2009, Roel Verdult
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
 * 
 * 
 * @file libnfc.h
 * @brief libnfc interface
 *
 * Provide all usefull functions (API) to handle NFC devices.
 */

#ifndef _LIBNFC_H_
#define _LIBNFC_H_

#include <stdint.h>
#include <stdbool.h>


#include <nfc/nfc-types.h>

#ifdef __cplusplus 
    extern "C" {
#endif // __cplusplus

/**
 * @fn void nfc_list_devices(nfc_device_desc_t *pnddDevices[], size_t szDevices, size_t *pszDeviceFound)
 * @brief Probe for discoverable supported devices (ie. only available for some drivers)
 * @param pnddDevices Array of nfc_device_desc_t previously allocated by the caller.
 * @param szDevices size of the pnddDevices array.
 * @param pszDeviceFound number of devices found.
 */
void nfc_list_devices(nfc_device_desc_t pnddDevices[], size_t szDevices, size_t *pszDeviceFound);

/**
 * @fn nfc_device_t* nfc_connect(nfc_device_desc_t* pndd)
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
nfc_device_t* nfc_connect(nfc_device_desc_t* pndd);

/**
 * @fn void nfc_disconnect(nfc_device_t* pnd)
 * @brief Disconnect from a NFC device
 * @param pnd nfc_device_t struct pointer that represent currently used device
 *
 * Initiator is disconnected and the device, including allocated nfc_device_t struct, is released.
 */
void nfc_disconnect(nfc_device_t* pnd);

/**
 * @fn nfc_configure(nfc_device_t* pnd, const nfc_device_option_t ndo, const bool bEnable)
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
bool nfc_configure(nfc_device_t* pnd, const nfc_device_option_t ndo, const bool bEnable);

/**
 * @fn nfc_initiator_init(const nfc_device_t* pnd)
 * @brief Initialize NFC device as initiator (reader)
 * @return Returns true if action was successfully performed; otherwise returns false.
 * @param pnd nfc_device_t struct pointer that represent currently used device
 *
 * The NFC device is configured to function as RFID reader. After initialization it can be used to communicate to passive RFID tags and active NFC devices. The reader will act as initiator to communicate peer 2 peer (NFCIP) to other active NFC devices.
 */
bool nfc_initiator_init(const nfc_device_t* pnd);

/**
 * @fn nfc_initiator_select_tag(const nfc_device_t* pnd, const nfc_modulation_t nmInitModulation, const byte_t* pbtInitData, const size_t szInitDataLen, nfc_target_info_t* pti)
 * @brief Select a passive or emulated tag
 * @return Returns true if action was successfully performed; otherwise returns false.
 * @param pnd nfc_device_t struct pointer that represent currently used device
 * @param im Desired modulation
 * @param pbtInitData Optional initiator data used for Felica, ISO14443B, Topaz Polling or for ISO14443A selecting a specific UID.
 * @param szInitDataLen Length of initiator data \a pbtInitData.
 *
 * The NFC device will try to find the available passive tags. Some NFC devices are capable to emulate passive tags. The standards (ISO18092 and ECMA-340) describe the modulation that can be used for reader to passive communications. The chip needs to know with what kind of tag it is dealing with, therefore the initial modulation and speed (106, 212 or 424 kbps) should be supplied.
 * @note For every initial modulation type there is a different collection of information returned (in nfc_target_info_t pointer pti) They all fit in the data-type which is called nfc_target_info_t. This is a union which contains the tag information that belongs to the according initial modulation type. 
 */
bool nfc_initiator_select_tag(const nfc_device_t* pnd, const nfc_modulation_t nmInitModulation, const byte_t* pbtInitData, const size_t szInitDataLen, nfc_target_info_t* pti);

/**
 * @fn nfc_initiator_select_dep_target(const nfc_device_t *pnd, const nfc_modulation_t nmInitModulation, const byte_t *pbtPidData, const size_t szPidDataLen, const byte_t *pbtNFCID3i, const size_t szNFCID3iDataLen, const byte_t *pbtGbData, const size_t szGbDataLen, nfc_target_info_t * pti);
 * @brief Select a target and request active or passive mode for DEP (Data Exchange Protocol)
 * @return Returns true if action was successfully performed; otherwise returns false.
 * @param pnd nfc_device_t struct pointer that represent currently used device
 * @param im Desired modulation (NM_ACTIVE_DEP or NM_PASSIVE_DEP for active, respectively passive mode)
 * @param pbtPidData passive initiator data, 4 or 5 bytes long, (optional, only for NM_PASSIVE_DEP, can be NULL)
 * @param pbtNFCID3i the NFCID3, 10 bytes long, of the initiator (optional, can be NULL)
 * @param pbtGbData generic data of the initiator, max 48 bytes long, (optional, can be NULL)
 *
 * The NFC device will try to find the available target. The standards (ISO18092 and ECMA-340) describe the modulation that can be used for reader to passive communications.
 * @note nfc_target_info_t_dep will be returned when the target was acquired successfully.
 */
bool nfc_initiator_select_dep_target(const nfc_device_t* pnd, const nfc_modulation_t nmInitModulation, const byte_t* pbtPidData, const size_t szPidDataLen, const byte_t* pbtNFCID3i, const size_t szNFCID3iDataLen, const byte_t *pbtGbData, const size_t szGbDataLen, nfc_target_info_t* pti);
/**
 * @fn nfc_initiator_deselect_tag(const nfc_device_t* pnd);
 * @brief Deselect a selected passive or emulated tag
 * @return Returns true if action was successfully performed; otherwise returns false.
 * @param pnd nfc_device_t struct pointer that represent currently used device
 *
 * After selecting and communicating with a passive tag, this function could be used to deactivate and release the tag. This is very useful when there are multiple tags available in the field. It is possible to use the nfc_initiator_select_tag() function to select the first available tag, test it for the available features and support, deselect it and skip to the next tag until the correct tag is found. 
 */
bool nfc_initiator_deselect_tag(const nfc_device_t* pnd);

/**
 * @fn nfc_initiator_transceive_bits(const nfc_device_t* pnd, const byte_t* pbtTx, const size_t szTxBits, const byte_t* pbtTxPar, byte_t* pbtRx, size_t* pszRxBits, byte_t* pbtRxPar)
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
bool nfc_initiator_transceive_bits(const nfc_device_t* pnd, const byte_t* pbtTx, const size_t szTxBits, const byte_t* pbtTxPar, byte_t* pbtRx, size_t* pszRxBits, byte_t* pbtRxPar);

/**
 * @fn nfc_initiator_transceive_bytes(const nfc_device_t* pnd, const byte_t* pbtTx, const size_t szTxLen, byte_t* pbtRx, size_t* pszRxLen)
 * @brief Transceive byte and APDU frames
 * @return Returns true if action was successfully performed; otherwise returns false.
 *
 * The reader will transmit the supplied bytes in pbtTx to the target (tag). It waits for the response and stores the received bytes in the pbtRx byte array. The parity bits are handled by the PN53X chip. The CRC can be generated automatically or handled manually. Using this function, frames can be communicated very fast via the NFC reader to the tag. Tests show that on average this way of communicating is much faster than using the regular driver/middle-ware (often supplied by manufacturers).
 * @warning The configuration option NDO_HANDLE_PARITY must be set to true (the default value).
 */
bool nfc_initiator_transceive_bytes(const nfc_device_t* pnd, const byte_t* pbtTx, const size_t szTxLen, byte_t* pbtRx, size_t* pszRxLen);

/**
 * @fn nfc_initiator_transceive_dep_bytes(const nfc_device_t* pnd, const byte_t* pbtTx, const size_t szTxLen, byte_t* pbtRx, size_t* pszRxLen)
 * @brief Transceive data
 * @return Returns true if action was successfully performed; otherwise returns false.
 *
 * The reader will transmit the supplied (data) bytes in pbtTx to the target (tag). It waits for the response and stores the received bytes in the pbtRx byte array. The difference between this function and nfc_initiator_transceive_bytes is that here pbtTx and pbtRx contain *only* the data sent and received and not any additional commands, that is all handled internally by the PN53X.
 */
bool nfc_initiator_transceive_dep_bytes(const nfc_device_t* pnd, const byte_t* pbtTx, const size_t szTxLen, byte_t* pbtRx, size_t* pszRxLen);

/**
 * @fn nfc_initiator_mifare_cmd(const nfc_device_t* pnd, const mifare_cmd mc, const uint8_t ui8Block, mifare_param* pmp)
 * @brief Execute a MIFARE Classic Command
 * @return Returns true if action was successfully performed; otherwise returns false.
 * @param pmp Some commands need additional information. This information should be supplied in the mifare_param union.
 *
 * The specified MIFARE command will be executed on the tag. There are different commands possible, they all require the destination block number.
 * @note There are three different types of information (Authenticate, Data and Value).
 *
 * First an authentication must take place using Key A or B. It requires a 48 bit Key (6 bytes) and the UID. They are both used to initialize the internal cipher-state of the PN53X chip (http://libnfc.org/hardware/pn53x-chip). After a successful authentication it will be possible to execute other commands (e.g. Read/Write). The MIFARE Classic Specification (http://www.nxp.com/acrobat/other/identification/M001053_MF1ICS50_rev5_3.pdf) explains more about this process. 
 */
bool nfc_initiator_mifare_cmd(const nfc_device_t* pnd, const mifare_cmd mc, const uint8_t ui8Block, mifare_param* pmp);

/**
 * @fn nfc_target_init(const nfc_device_t* pnd, byte_t* pbtRx, size_t* pszRxBits)
 * @brief Initialize NFC device as an emulated tag
 * @return Returns true if action was successfully performed; otherwise returns false.
 *
 * This functionality allows the NFC device to act as an emulated tag. There seems to be quite some options available for this feature. Not all of the PN53X modulations are tested and documented at the moment. At the moment it could best be seen as a preliminary functionality.
 *
 * @warning Be aware that this function will wait (hang) until a command is received that is not part of the anti-collision. The RATS command for example would wake up the emulator. After this is received, the send and receive functions can be used. 
 */
bool nfc_target_init(const nfc_device_t* pnd, byte_t* pbtRx, size_t* pszRxBits);

/**
 * @fn nfc_target_receive_bits(const nfc_device_t* pnd, byte_t* pbtRx, size_t* pszRxBits, byte_t* pbtRxPar)
 * @brief Receive bit-frames
 * @return Returns true if action was successfully performed; otherwise returns false.
 *
 * This function makes it possible to receive (raw) bit-frames. It returns all the messages that are stored in the FIFO buffer of the PN53X chip. It does not require to send any frame and thereby could be used to snoop frames that are transmitted by a nearby reader. Check out the NDO_ACCEPT_MULTIPLE_FRAMES configuration option to avoid losing transmitted frames. 
 */
bool nfc_target_receive_bits(const nfc_device_t* pnd, byte_t* pbtRx, size_t* pszRxBits, byte_t* pbtRxPar);

/**
 * @fn nfc_target_receive_bytes(const nfc_device_t* pnd, byte_t* pbtRx, size_t* pszRxLen)
 * @brief Receive bytes and APDU frames
 * @return Returns true if action was successfully performed; otherwise returns false.
 *
 * The main receive function that returns the received frames from a nearby reader.
 */
bool nfc_target_receive_bytes(const nfc_device_t* pnd, byte_t* pbtRx, size_t* pszRxLen);

/**
 * @fn nfc_target_receive_dep_bytes(const nfc_device_t* pnd, byte_t* pbtRx, size_t* pszRxLen)
 * @brief Receive data
 * @return Returns true if action was successfully performed; otherwise returns false.
 *
 * The main receive function that returns the received data from a nearby reader. The difference between this function and nfc_target_receive_bytes is that here pbtRx contains *only* the data received and not any additional commands, that is all handled internally by the PN53X.
 */
bool nfc_target_receive_dep_bytes(const nfc_device_t* pnd, byte_t* pbtRx, size_t* pszRxLen);

/**
 * @fn nfc_target_send_bits(const nfc_device_t* pnd, const byte_t* pbtTx, const size_t szTxBits, const byte_t* pbtTxPar)
 * @brief Send raw bit-frames
 * @return Returns true if action was successfully performed; otherwise returns false.
 *
 * This function can be used to transmit (raw) bit-frames to the reader.
 */
bool nfc_target_send_bits(const nfc_device_t* pnd, const byte_t* pbtTx, const size_t szTxBits, const byte_t* pbtTxPar);

/**
 * @fn nfc_target_send_bytes(const nfc_device_t* pnd, const byte_t* pbtTx, const size_t szTxLen)
 * @brief Send bytes and APDU frames
 * @return Returns true if action was successfully performed; otherwise returns false.
 *
 * To communicate byte frames and APDU responses to the reader, this function could be used.
 */
bool nfc_target_send_bytes(const nfc_device_t* pnd, const byte_t* pbtTx, const size_t szTxLen);

/**
 * @fn nfc_target_send_dep_bytes(const nfc_device_t* pnd, const byte_t* pbtTx, const size_t szTxLen)
 * @brief Send data
 * @return Returns true if action was successfully performed; otherwise returns false.
 *
 * To communicate data to the reader, this function could be used. The difference between this function and nfc_target_send_bytes is that here pbtTx contains *only* the data sent and not any additional commands, that is all handled internally by the PN53X.
 */
bool nfc_target_send_dep_bytes(const nfc_device_t* pnd, const byte_t* pbtTx, const size_t szTxLen);

/**
 * @fn const char* nfc_version(void)
 * @brief Returns the library version
 * @return Returns a string with the library version
 */
const char* nfc_version(void);

#ifdef __cplusplus 
}
#endif // __cplusplus


#endif // _LIBNFC_H_

