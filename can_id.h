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

/** Public types. *************************************************************/

typedef enum {
  CAN_PRIO_EMERGENCY = 0, // Hard fault, estop, overcurrent, watchdog reset.
  CAN_PRIO_HIGH = 1,      // Real time critical control.
  CAN_PRIO_NORMAL = 2,    // Regular state updates, telemetry.
  CAN_PRIO_LOW = 3        // Debug, configuration, discovery, logging.
} can_priority_t;

typedef enum {
  // Synchronization.
  CAN_MSG_SYNC = 0x0, // Network sync/timestep alignment.

  // Control/real-time.
  CAN_MSG_CONTROL_CMD = 0x1,      // Control systems command.
  CAN_MSG_CONTROL_FEEDBACK = 0x2, // Control systems feedback.

  // General state.
  CAN_MSG_STATUS = 0x3, // State summary (mode, errors, flags).

  // Telemetry.
  CAN_MSG_SENSOR = 0x4, // Raw or processed sensor data.

  // Configuration/management.
  CAN_MSG_CONFIG_SET = 0x5,   // Write config parameter.
  CAN_MSG_CONFIG_GET = 0x6,   // Read config parameter.
  CAN_MSG_CONFIG_REPLY = 0x7, // Config read response/ACK.

  // System/infrastructure.
  CAN_MSG_HEARTBEAT = 0x8, // Periodic heartbeat ping.
  CAN_MSG_NODE_INFO = 0x9, // Firmware version, capabilities.
  CAN_MSG_DISCOVERY = 0xA, // Node enumeration/ID assignment.

  // Diagnostics/debug.
  CAN_MSG_ERROR = 0xB, // Error report/fault detail.
  CAN_MSG_DEBUG = 0xC, // Debug prints, trace, dev tools.

  // Reserved/future.
  CAN_MSG_RESERVED_13 = 0xD,
  CAN_MSG_RESERVED_14 = 0xE,
  CAN_MSG_RESERVED_15 = 0xF
} can_message_type_t;

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
bool can_id_pack(can_priority_t priority, can_message_type_t message_type,
                 uint8_t node_id, uint16_t *out_can_id);

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
bool can_id_unpack(uint16_t can_id, can_priority_t *out_priority,
                   can_message_type_t *out_message_type, uint8_t *out_node_id);

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
