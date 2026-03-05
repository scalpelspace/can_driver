/*******************************************************************************
 * @file can_id_allocatee.h
 * @brief CAN ID standard (allocatee) for ScalpelSpace specific nodes.
 *******************************************************************************
 */

#ifndef CAN_DRIVER__CAN_ID_ALLOCATEE_H
#define CAN_DRIVER__CAN_ID_ALLOCATEE_H

/** Includes. *****************************************************************/

#include "can_driver.h"
#include <stdbool.h>
#include <stdint.h>

/** Public types. *************************************************************/

/**
 * @brief Define function pointer type for sending CAN bus messages.
 */
typedef bool (*can_tx_func_t)(const can_message_t *msg, const uint8_t data[8]);

/**
 * @brief Determine 3x 16-bit (48-bit split) UID hash.
 *
 * @param uid0 UID hash48, 16-bit segment 1 of 3 (bits 0..15 of 48-bit hash).
 * @param uid1 UID hash48, 16-bit segment 2 of 3 (bits 16..31 of 48-bit hash).
 * @param uid2 UID hash48, 16-bit segment 3 of 3 (bits 32..47 of 48-bit hash).
 */
typedef void (*get_uid_hash48_func_t)(uint16_t *uid0, uint16_t *uid1,
                                      uint16_t *uid2);

typedef struct allocatee_config {
  can_tx_func_t can_tx_func; // CAN message transmit function pointer.
  get_uid_hash48_func_t get_uid_hash48_func; // Get UID hash48 function pointer.
} allocatee_config_t;

/** Public functions. *********************************************************/

/**
 * @breif CAN RX callback function for allocatee discovery message processing.
 *
 * @param header
 * @param data
 *
 * @return Success status.
 * @retval true -> Discovery message processed successfully.
 * @retval false -> Error.
 */
void can_rx_can_id_allocatee_discovery(const can_header_t *header,
                                       const uint8_t *data);

/**
 * @breif CAN RX callback function for allocatee assignment message processing.
 *
 * @param header
 * @param data
 *
 * @return Success status.
 * @retval true -> Allocatee message processed successfully.
 * @retval false -> Error.
 */
void can_rx_can_id_allocatee_assignment(const can_header_t *header,
                                        const uint8_t *data);

/**
 * @breif Begin CAN ID allocate state machine.
 *
 * @param allocatee
 *
 * @return Success status.
 * @retval true -> Allocatee state machine started successfully.
 * @retval false -> Error.
 */
bool can_id_allocatee_start(allocatee_config_t allocatee);

/**
 * @breif Run CAN ID allocatee state machine.
 */
void can_id_allocatee_state_machine(void);

#endif
