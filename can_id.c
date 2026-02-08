/*******************************************************************************
 * @file can_id.c
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

/** Includes. *****************************************************************/

#include "can_id.h"
#include "math.h"

/** Definitions. **************************************************************/

// Masks (post-shifted).
#define CAN_ID_NODE_MASK ((uint16_t)CAN_ID_NODE_MAX)
#define CAN_ID_MSGTYPE_MASK ((uint16_t)CAN_ID_MSGTYPE_MAX)
#define CAN_ID_PRIORITY_MASK ((uint16_t)CAN_ID_PRIORITY_MAX)

// Shifts.
#define CAN_ID_NODE_SHIFT (0u)
#define CAN_ID_MSGTYPE_SHIFT (CAN_ID_NODE_BITS)                        // 5
#define CAN_ID_PRIORITY_SHIFT (CAN_ID_NODE_BITS + CAN_ID_MSGTYPE_BITS) // 9

// Reserved Node IDs.
#define CAN_ID_NODE_UNASSIGNED (0u)
#define CAN_ID_NODE_BROADCAST (31u)

/** Private types. ************************************************************/

enum {
  CAN_ID_PRIORITY_BITS = 2u,
  CAN_ID_MSGTYPE_BITS = 4u,
  CAN_ID_NODE_BITS = 5u,

  CAN_ID_NODE_MAX = (1u << CAN_ID_NODE_BITS) - 1u,         // 31
  CAN_ID_MSGTYPE_MAX = (1u << CAN_ID_MSGTYPE_BITS) - 1u,   // 15
  CAN_ID_PRIORITY_MAX = (1u << CAN_ID_PRIORITY_BITS) - 1u, // 3

  CAN_ID_STD_MAX = 0x7FFu
};

/** Private functions. ********************************************************/

static bool can_id_fields_valid(const can_priority_t priority,
                                const can_message_type_t message_type,
                                const uint8_t node_id) {
  if (priority > CAN_ID_PRIORITY_MAX)
    return false;
  if (message_type > CAN_ID_MSGTYPE_MAX)
    return false;
  if (node_id > CAN_ID_NODE_MAX)
    return false;
  return true;
}

/** Public functions. *********************************************************/

bool can_id_pack(const can_priority_t priority,
                 const can_message_type_t message_type, const uint8_t node_id,
                 uint16_t *out_can_id) {
  if (out_can_id == NULL)
    return false;
  if (!can_id_fields_valid(priority, message_type, node_id))
    return false;

  uint16_t id = 0u;
  id |= ((uint16_t)(priority & CAN_ID_PRIORITY_MASK) << CAN_ID_PRIORITY_SHIFT);
  id |=
      ((uint16_t)(message_type & CAN_ID_MSGTYPE_MASK) << CAN_ID_MSGTYPE_SHIFT);
  id |= ((uint16_t)(node_id & CAN_ID_NODE_MASK) << CAN_ID_NODE_SHIFT);

  // Safety: ensure 11-bit
  id &= CAN_ID_STD_MAX;

  *out_can_id = id;
  return true;
}

bool can_id_unpack(const uint16_t can_id, can_priority_t *out_priority,
                   can_message_type_t *out_message_type, uint8_t *out_node_id) {
  if (can_id > CAN_ID_STD_MAX)
    return false;

  if (out_priority) {
    *out_priority =
        (uint8_t)((can_id >> CAN_ID_PRIORITY_SHIFT) & CAN_ID_PRIORITY_MASK);
  }
  if (out_message_type) {
    *out_message_type =
        (uint8_t)((can_id >> CAN_ID_MSGTYPE_SHIFT) & CAN_ID_MSGTYPE_MASK);
  }
  if (out_node_id) {
    *out_node_id = (uint8_t)((can_id >> CAN_ID_NODE_SHIFT) & CAN_ID_NODE_MASK);
  }
  return true;
}

bool can_id_is_unassigned(const uint16_t can_id) {
  uint8_t node = 0u;
  (void)can_id_unpack(can_id, NULL, NULL, &node);
  return (node == (uint8_t)CAN_ID_NODE_UNASSIGNED);
}

bool can_id_is_broadcast(const uint16_t can_id) {
  uint8_t node = 0u;
  (void)can_id_unpack(can_id, NULL, NULL, &node);
  return (node == (uint8_t)CAN_ID_NODE_BROADCAST);
}
