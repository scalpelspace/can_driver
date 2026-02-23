/*******************************************************************************
 * @file can_id.h
 * @brief Custom CAN ID standard for ScalpelSpace specific node devices.
 *******************************************************************************
 * Custom 11-bit CAN ID packing for:
 *   [ 6 bits message_id ][ 5 bits node_id ]
 *
 * Bit layout (MSB -> LSB):
 *    bits 10..5  : message_id  (0..63)   (6 bits)
 *    bits  4..0  : node_id     (0..31)   (5 bits)
 *
 * Result is always an 11-bit standard CAN identifier (0..0x7FF).
 *******************************************************************************
 */

#ifndef CAN_DRIVER__CAN_ID_H
#define CAN_DRIVER__CAN_ID_H

/** CPP guard open. ***********************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/** Includes. *****************************************************************/

#include <stdbool.h>
#include <stdint.h>

/** Definitions. **************************************************************/

// Reserved Node IDs.
#define CAN_ID_NODE_ID_UNASSIGNED (0u)
#define CAN_ID_NODE_ID_BROADCAST (31u)

/** Public types. *************************************************************/

typedef uint8_t can_message_id_t; // Range: 0..63.
typedef uint8_t can_node_id_t;    // Range: 0..31.
typedef uint16_t can_id_t;        // Range: 0..0x7FF.

/** Public functions. *********************************************************/

/**
 * @brief Packs the custom ID into a standard 11-bit CAN ID.
 *
 * @param message_id
 * @param node_id
 * @param out_can_id
 *
 * @return True on success, False if inputs out of range.
 */
bool can_id_pack(can_message_id_t message_id, can_node_id_t node_id,
                 can_id_t *out_can_id);

/**
 * @brief Unpacks a standard 11-bit CAN ID into custom fields.
 *
 * @param can_id
 * @param out_message_id
 * @param out_node_id
 *
 * @return True on success, False if ID is out of 11-bit range.
 */
bool can_id_unpack(can_id_t can_id, can_message_id_t *out_message_id,
                   can_node_id_t *out_node_id);

/**
 * @brief Check if CAN ID is set to unassigned.
 *
 * @param can_id ScalpelSpace node CAN ID to check.
 *
 * @return True if CAN ID is unassigned, else False.
 */
bool can_id_is_unassigned(can_id_t can_id);

/**
 * @brief Check if CAN ID is set to broadcast.
 *
 * @param can_id ScalpelSpace node CAN ID to check.
 *
 * @return True if CAN ID is broadcast, else False.
 */
bool can_id_is_broadcast(can_id_t can_id);

/** CPP guard close. **********************************************************/

#ifdef __cplusplus
}
#endif

#endif
