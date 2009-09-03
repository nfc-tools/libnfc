/*
 
Public platform independent Near Field Communication (NFC) library
Copyright (C) 2009, Roel Verdult
 
This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>

*/
#ifndef _LIBNFC_H_
#define _LIBNFC_H_

/**
 * @file libnfc.h
 * @brief libnfc interface
 *
 * Provide all usefull functions to handle NFC devices.
 */

#include <stdint.h>
#include <stdbool.h>

#include "defines.h"
#include "types.h"
#include "bitutils.h"

/**
 * @fn dev_info* nfc_connect()
 * @brief Connect to a NFC device
 * @return Returns pointer to a dev_info struct if successfull; otherwise returns INVALID_DEVICE_INFO value.
 *
 * The first available NFC device is claimed by libnfc.
 * It will automatically search the system using all available drivers to determine a device is free.
 * When it has successfully claimed a NFC device, memory is allocated to save the device information. It will return a pointer to a dev_info struct.
 * This pointer should be supplied by every next function of libnfc that should perform an action with this device.
 */
dev_info* nfc_connect(void);

/**
 * @fn void nfc_disconnect(dev_info* pdi)
 * @brief Disconnect from a NFC device
 * @param pdi dev_info struct pointer that represent currently used device
 *
 * Initiator is disconnected and the device, including allocated dev_info struct, is released.
 */
void nfc_disconnect(dev_info* pdi);

/**
 * @fn nfc_configure(dev_info* pdi, const dev_config_option dco, const bool bEnable)
 * @brief Configure advanced NFC device settings
 * @return Returns true if action was successfully performed; otherwise returns false.
 * @param pdi dev_info struct pointer that represent currently used device
 * @param dco dev_config_option struct that contains options to set to device
 * @param bEnable boolean
 *
 * Configures parameters and registers that control for example timing, modulation, frame and error handling.
 * There are different categories for configuring the PN53X chip features (handle, activate, infinite and accept).
 * These are defined to organize future settings that will become available when they are needed.
 */
bool nfc_configure(dev_info* pdi, const dev_config_option dco, const bool bEnable);

/**
 * @fn nfc_initiator_init(const dev_info* pdi)
 * @brief Initialize NFC device as initiator (reader)
 * @return Returns true if action was successfully performed; otherwise returns false.
 * @param pdi dev_info struct pointer that represent currently used device
 *
 * The NFC device is configured to function as RFID reader. After initialization it can be used to communicate to passive RFID tags and active NFC devices. The reader will act as initiator to communicate peer 2 peer (NFCIP) to other active NFC devices.
 */
bool nfc_initiator_init(const dev_info* pdi);

/**
 * @fn nfc_initiator_select_tag(const dev_info* pdi, const init_modulation im, const byte_t* pbtInitData, const uint32_t uiInitDataLen, tag_info* pti)
 * @brief Select a passive or emulated tag
 * @return Returns true if action was successfully performed; otherwise returns false.
 * @param pdi dev_info struct pointer that represent currently used device
 * @param im Desired modulation
 *
 * The NFC device will try to find the available passive tags. Some NFC devices are capable to emulate passive tags. The standards (ISO18092 and ECMA-340) describe the modulation that can be used for reader to passive communications. The chip needs to know with what kind of tag it is dealing with, therefore the initial modulation and speed (106, 212 or 424 kbps) should be supplied.
 * @note For every initial modulation type there is a different collection of information returned (in tag_info pointer pti) They all fit in the data-type which is called tag_info. This is a union which contains the tag information that belongs to the according initial modulation type. 
 */
bool nfc_initiator_select_tag(const dev_info* pdi, const init_modulation im, const byte_t* pbtInitData, const uint32_t uiInitDataLen, tag_info* pti);

/**
 * @fn nfc_initiator_select_dep_target(const dev_info *pdi, const init_modulation im, const byte_t *pbtPidData, const uint32_t uiPidDataLen, const byte_t *pbtNFCID3i, const uint32_t uiNFCID3iDataLen, const byte_t *pbtGbData, const uint32_t uiGbDataLen, tag_info * pti);
 * @brief Select a target and request active or passive mode for DEP (Data Exchange Protocol)
 * @return Returns true if action was successfully performed; otherwise returns false.
 * @param pdi dev_info struct pointer that represent currently used device
 * @param im Desired modulation (IM_ACTIVE_DEP or IM_PASSIVE_DEP for active, respectively passive mode)
 * @param pbtPidData passive initiator data, 4 or 5 bytes long, (optional, only for IM_PASSIVE_DEP, can be NULL)
 * @param pbtNFCID3i the NFCID3, 10 bytes long, of the initiator (optional, can be NULL)
 * @param pbtGbData generic data of the initiator, max 48 bytes long, (optional, can be NULL)
 *
 * The NFC device will try to find the available target. The standards (ISO18092 and ECMA-340) describe the modulation that can be used for reader to passive communications.
 * @note tag_info_dep will be returned when the target was acquired successfully.
 */
bool nfc_initiator_select_dep_target(const dev_info* pdi, const init_modulation im, const byte_t* pbtPidData, const uint32_t uiPidDataLen, const byte_t* pbtNFCID3i, const uint32_t uiNFCID3iDataLen, const byte_t *pbtGbData, const uint32_t uiGbDataLen, tag_info* pti);
/**
 * @fn nfc_initiator_deselect_tag(const dev_info* pdi);
 * @brief Deselect a selected passive or emulated tag
 * @return Returns true if action was successfully performed; otherwise returns false.
 * @param pdi dev_info struct pointer that represent currently used device
 *
 * After selecting and communicating with a passive tag, this function could be used to deactivate and release the tag. This is very useful when there are multiple tags available in the field. It is possible to use the nfc_initiator_select_tag() function to select the first available tag, test it for the available features and support, deselect it and skip to the next tag until the correct tag is found. 
 */
bool nfc_initiator_deselect_tag(const dev_info* pdi);

/**
 * @fn nfc_initiator_transceive_bits(const dev_info* pdi, const byte_t* pbtTx, const uint32_t uiTxBits, const byte_t* pbtTxPar, byte_t* pbtRx, uint32_t* puiRxBits, byte_t* pbtRxPar)
 * @brief Transceive raw bit-frames
 * @return Returns true if action was successfully performed; otherwise returns false.
 * @param pbtTx contains a byte array of the frame that needs to be transmitted.
 * @param uiTxBits contains the length in bits.
 * @note For example the REQA (0×26) command (first anti-collision command of ISO14443-A) must be precise 7 bits long. This is not possible by using nfc_initiator_transceive_bytes(). With that function you can only communicate frames that consist of full bytes. When you send a full byte (8 bits + 1 parity) with the value of REQA (0×26), a tag will simply not respond. More information about this can be found in the anti-colision example. 
 * @param pbtTxPar parameter contains a byte array of the corresponding parity bits needed to send per byte.
 * @note For example if you send the SELECT_ALL (0×93, 0×20) = [ 10010011, 00100000 ] command, you have to supply the following parity bytes (0×01, 0×00) to define the correct odd parity bits. This is only an example to explain how it works, if you just are sending two bytes with ISO14443-A compliant parity bits you better can use the nfc_initiator_transceive_bytes() function.
 * @returns The received response from the tag will be stored in the parameters (pbtRx, puiRxBits and pbtRxPar). They work the same way as the corresponding parameters for transmission.
 *
 * The NFC reader will transmit low-level messages where only the modulation is handled by the PN53X chip. Construction of the frame (data, CRC and parity) is completely done by libnfc. This can be very useful for testing purposes. Some protocols (e.g. MIFARE Classic) require to violate the ISO14443-A standard by sending incorrect parity and CRC bytes. Using this feature you are able to simulate these frames.
 */
bool nfc_initiator_transceive_bits(const dev_info* pdi, const byte_t* pbtTx, const uint32_t uiTxBits, const byte_t* pbtTxPar, byte_t* pbtRx, uint32_t* puiRxBits, byte_t* pbtRxPar);

/**
 * @fn nfc_initiator_transceive_bytes(const dev_info* pdi, const byte_t* pbtTx, const uint32_t uiTxLen, byte_t* pbtRx, uint32_t* puiRxLen)
 * @brief Transceive byte and APDU frames
 * @return Returns true if action was successfully performed; otherwise returns false.
 *
 * The reader will transmit the supplied bytes in pbtTx to the target (tag). It waits for the response and stores the received bytes in the pbtRx byte array. The parity bits are handled by the PN53X chip. The CRC can be generated automatically or handled manually. Using this function, frames can be communicated very fast via the NFC reader to the tag. Tests show that on average this way of communicating is much faster than using the regular driver/middle-ware (often supplied by manufacturers).
 * @warning The configuration option DCO_HANDLE_PARITY must be set to true (the default value).
 */
bool nfc_initiator_transceive_bytes(const dev_info* pdi, const byte_t* pbtTx, const uint32_t uiTxLen, byte_t* pbtRx, uint32_t* puiRxLen);

/**
 * @fn nfc_initiator_transceive_dep_bytes(const dev_info* pdi, const byte_t* pbtTx, const uint32_t uiTxLen, byte_t* pbtRx, uint32_t* puiRxLen)
 * @brief Transceive data
 * @return Returns true if action was successfully performed; otherwise returns false.
 *
 * The reader will transmit the supplied (data) bytes in pbtTx to the target (tag). It waits for the response and stores the received bytes in the pbtRx byte array. The difference between this function and nfc_initiator_transceive_bytes is that here pbtTx and pbtRx contain *only* the data sent and received and not any additional commands, that is all handled internally by the PN53X.
 */
bool nfc_initiator_transceive_dep_bytes(const dev_info* pdi, const byte_t* pbtTx, const uint32_t uiTxLen, byte_t* pbtRx, uint32_t* puiRxLen);

/**
 * @fn nfc_initiator_mifare_cmd(const dev_info* pdi, const mifare_cmd mc, const uint8_t ui8Block, mifare_param* pmp)
 * @brief Execute a MIFARE Classic Command
 * @return Returns true if action was successfully performed; otherwise returns false.
 * @param pmp Some commands need additional information. This information should be supplied in the mifare_param union.
 *
 * The specified MIFARE command will be executed on the tag. There are different commands possible, they all require the destination block number.
 * @note There are three different types of information (Authenticate, Data and Value).
 *
 * First an authentication must take place using Key A or B. It requires a 48 bit Key (6 bytes) and the UID. They are both used to initialize the internal cipher-state of the PN53X chip (http://libnfc.org/hardware/pn53x-chip). After a successful authentication it will be possible to execute other commands (e.g. Read/Write). The MIFARE Classic Specification (http://www.nxp.com/acrobat/other/identification/M001053_MF1ICS50_rev5_3.pdf) explains more about this process. 
 */
bool nfc_initiator_mifare_cmd(const dev_info* pdi, const mifare_cmd mc, const uint8_t ui8Block, mifare_param* pmp);

/**
 * @fn nfc_target_init(const dev_info* pdi, byte_t* pbtRx, uint32_t* puiRxBits)
 * @brief Initialize NFC device as an emulated tag
 * @return Returns true if action was successfully performed; otherwise returns false.
 *
 * This functionality allows the NFC device to act as an emulated tag. There seems to be quite some options available for this feature. Not all of the PN53X modulations are tested and documented at the moment. At the moment it could best be seen as a preliminary functionality.
 *
 * @warning Be aware that this function will wait (hang) until a command is received that is not part of the anti-collision. The RATS command for example would wake up the emulator. After this is received, the send and receive functions can be used. 
 */
bool nfc_target_init(const dev_info* pdi, byte_t* pbtRx, uint32_t* puiRxBits);

/**
 * @fn nfc_target_receive_bits(const dev_info* pdi, byte_t* pbtRx, uint32_t* puiRxBits, byte_t* pbtRxPar)
 * @brief Receive bit-frames
 * @return Returns true if action was successfully performed; otherwise returns false.
 *
 * This function makes it possible to receive (raw) bit-frames. It returns all the messages that are stored in the FIFO buffer of the PN53X chip. It does not require to send any frame and thereby could be used to snoop frames that are transmitted by a nearby reader. Check out the DCO_ACCEPT_MULTIPLE_FRAMES configuration option to avoid losing transmitted frames. 
 */
bool nfc_target_receive_bits(const dev_info* pdi, byte_t* pbtRx, uint32_t* puiRxBits, byte_t* pbtRxPar);

/**
 * @fn nfc_target_receive_bytes(const dev_info* pdi, byte_t* pbtRx, uint32_t* puiRxLen)
 * @brief Receive bytes and APDU frames
 * @return Returns true if action was successfully performed; otherwise returns false.
 *
 * The main receive function that returns the received frames from a nearby reader.
 */
bool nfc_target_receive_bytes(const dev_info* pdi, byte_t* pbtRx, uint32_t* puiRxLen);

/**
 * @fn nfc_target_receive_dep_bytes(const dev_info* pdi, byte_t* pbtRx, uint32_t* puiRxLen)
 * @brief Receive data
 * @return Returns true if action was successfully performed; otherwise returns false.
 *
 * The main receive function that returns the received data from a nearby reader. The difference between this function and nfc_target_receive_bytes is that here pbtRx contains *only* the data received and not any additional commands, that is all handled internally by the PN53X.
 */
bool nfc_target_receive_dep_bytes(const dev_info* pdi, byte_t* pbtRx, uint32_t* puiRxLen);

/**
 * @fn nfc_target_send_bits(const dev_info* pdi, const byte_t* pbtTx, const uint32_t uiTxBits, const byte_t* pbtTxPar)
 * @brief Send raw bit-frames
 * @return Returns true if action was successfully performed; otherwise returns false.
 *
 * This function can be used to transmit (raw) bit-frames to the reader.
 */
bool nfc_target_send_bits(const dev_info* pdi, const byte_t* pbtTx, const uint32_t uiTxBits, const byte_t* pbtTxPar);

/**
 * @fn nfc_target_send_bytes(const dev_info* pdi, const byte_t* pbtTx, const uint32_t uiTxLen)
 * @brief Send bytes and APDU frames
 * @return Returns true if action was successfully performed; otherwise returns false.
 *
 * To communicate byte frames and APDU responses to the reader, this function could be used.
 */
bool nfc_target_send_bytes(const dev_info* pdi, const byte_t* pbtTx, const uint32_t uiTxLen);

/**
 * @fn nfc_target_send_dep_bytes(const dev_info* pdi, const byte_t* pbtTx, const uint32_t uiTxLen)
 * @brief Send data
 * @return Returns true if action was successfully performed; otherwise returns false.
 *
 * To communicate data to the reader, this function could be used. The difference between this function and nfc_target_send_bytes is that here pbtTx contains *only* the data sent and not any additional commands, that is all handled internally by the PN53X.
 */
bool nfc_target_send_dep_bytes(const dev_info* pdi, const byte_t* pbtTx, const uint32_t uiTxLen);

#endif // _LIBNFC_H_

