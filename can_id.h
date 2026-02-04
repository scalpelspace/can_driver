/*******************************************************************************
 * @file can_id.h
 * @brief Custom CAN ID standard for ScalpelSpace specific node devices.
 *******************************************************************************
 * Custom 11-bit CAN ID packing for:
 *   [ 2 bits priority ][ 4 bits message_type ][ 5 bits node_id ]
 *
 * Bit layout (MSB -> LSB):
 *    bits 10..9  : priority      (0..3)    (2 bits)
 *    bits  8..5  : message_type  (0..15)   (4 bits)
 *    bits  4..0  : node_id       (0..31)   (5 bits)
 *
 * Result is always an 11-bit standard CAN identifier (0..0x7FF).
 *******************************************************************************
 */

#ifndef __CAN_ID_H
#define __CAN_ID_H

/** CPP guard open. ***********************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/** Includes. *****************************************************************/

#include <stdbool.h>
#include <stdint.h>

/** Public functions. *********************************************************/

/**
 * @brief Packs the custom ID into a standard 11-bit CAN ID.
 *
 * @param priority
 * @param message_type
 * @param node_id
 * @param out_can_id
 *
 * @return True on success, False if inputs out of range.
 */
bool can_id_pack(uint8_t priority, uint8_t message_type, uint8_t node_id,
                 uint16_t *out_can_id);

/**
 * @brief Unpacks a standard 11-bit CAN ID into custom fields.
 *
 * @param can_id ScalpelSpace node CAN ID to check.
 * @param out_priority
 * @param out_message_type
 * @param out_node_id
 *
 * @return True on success, False if ID is out of 11-bit range.
 */
bool can_id_unpack(uint16_t can_id, uint8_t *out_priority,
                   uint8_t *out_message_type, uint8_t *out_node_id);

/**
 * @brief Check if CAN ID is set to unassigned.
 *
 * @param can_id ScalpelSpace node CAN ID to check.
 *
 * @return True if CAN ID is unassigned, else False.
 */
bool can_id_is_unassigned(uint16_t can_id);

/**
 * @brief Check if CAN ID is set to broadcast.
 *
 * @param can_id ScalpelSpace node CAN ID to check.
 *
 * @return True if CAN ID is broadcast, else False.
 */
bool can_id_is_broadcast(uint16_t can_id);

/** CPP guard close. **********************************************************/

#ifdef __cplusplus
}
#endif

#endif
