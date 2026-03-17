/*******************************************************************************
 * @file can_id_allocator.c
 * @brief CAN ID standard (allocator) for ScalpelSpace specific nodes.
 *******************************************************************************
 */

/** Includes. *****************************************************************/

#include "can_id_allocator.h"
#include "can_id.h"
#include "can_id_allocation_dbc.h"
#include <stdint.h>
#include <string.h>

/** Private types. ************************************************************/

typedef enum {
  ALLOCATOR_IDLE = 0,     // Allocator not started via can_id_allocator_start().
  ALLOCATOR_DISCOVER = 1, // Transmitting discovery command to allocatee(s).
  ALLOCATOR_AWAIT_ADVERTISE = 2, // Awaiting allocatee advertisements.
  ALLOCATOR_ASSIGN = 3,          // Assigning discovered allocatee(s).
  ALLOCATOR_AWAIT_ACK = 4,       // Awaiting Node ID acknowledgements.
} allocator_state_t;

/** Private variables. ********************************************************/

// CAN ID allocator configuration.
static allocator_config_t config = {NULL};

// State machine variable for allocator.
static allocator_state_t allocator_state = ALLOCATOR_IDLE;
static uint8_t session_id = 0; // Expected session ID.

// Counters for node management.
static uint8_t discovered_nodes = 0;       // Nodes found in discovery.
static uint8_t soft_assigned_nodes = 0;    // Nodes attempted assignment.
static uint8_t assignment_acked_nodes = 0; // Nodes that ACKed assignment.

// Arrays indexed by received order of UIDs (discovered_nodes):
// Discovered UID hash48s (0..15).
static uint16_t discovered_uids_0[CAN_ID_MAX_NODES] = {0};
// Discovered UID hash48s (16..31).
static uint16_t discovered_uids_1[CAN_ID_MAX_NODES] = {0};
// Discovered UID hash48s (32..47).
static uint16_t discovered_uids_2[CAN_ID_MAX_NODES] = {0};
// Assigning Node ID value (awaiting ACK).
static can_node_id_t assigning_node_ids[CAN_ID_MAX_NODES] = {0};

// Assigned (ACKed) Node IDs, indexed by Node ID.
static can_node_id_t assigned_node_ids[CAN_ID_MAX_NODES] = {0};

/** Private functions. ********************************************************/

/**
 * @brief Search for matching UIDs (allocator ACK verification).
 *
 * @param uid_0 UID hash48 (0..15) to check for match.
 * @param uid_1 UID hash48 (16..31) to check for match.
 * @param uid_2 UID hash48 (32..47) to check for match.
 */
int16_t search_received_uids(const uint16_t uid_0, const uint16_t uid_1,
                             const uint16_t uid_2) {
  for (uint8_t i = 0; i < CAN_ID_MAX_NODES; i++) {
    if ((discovered_uids_0[i] == uid_0) && (discovered_uids_1[i] == uid_1) &&
        (discovered_uids_2[i] == uid_2)) {
      return i;
    }
  }
  return -1;
}

// TODO: Implement Node ID assignment strategy, maybe set in allocator_config_t?
//  Current temporary solution, assign Node IDs as FIFO (first in, first
//  assigned).
void node_id_strategy(void) {
  for (uint8_t i = 0; i < discovered_nodes; i++) {

    // Assign Node ID based on strategy.
    const can_node_id_t node_id = i + 1;

    // Maintain log of attempted Node ID assignments.
    assigning_node_ids[i] = node_id;
  }
}

/** Public functions. *********************************************************/

bool can_id_allocator_start(const allocator_config_t allocator) {
  if (allocator_state == ALLOCATOR_IDLE) {
    config = allocator; // Update configuration.
    allocator_state = ALLOCATOR_DISCOVER;
    discovered_nodes = 0;
    soft_assigned_nodes = 0;
    assignment_acked_nodes = 0;
    return true;
  }
  return false;
}

bool can_id_allocator_end_discovery(void) {
  if (allocator_state == ALLOCATOR_AWAIT_ADVERTISE) {
    allocator_state = ALLOCATOR_ASSIGN;
    return true;
  }
  return false;
}

void can_rx_can_id_allocatee_advertise(const can_header_t *header,
                                       const uint8_t *data) {
  const can_message_t msg =
      allocation_dbc[CAN_ID_ALLOCATION_DBC_IDX_NODE_ID_ADVERTISE];

  if (!header || !data)
    return;
  if (allocator_state != ALLOCATOR_AWAIT_ADVERTISE)
    return;

  // Ensure maximum Node count is not reached.
  if (discovered_nodes >= CAN_ID_MAX_NODES)
    return;

  // Ensure CAN header consistency.
  if (header->standard_id != msg.message_id || header->dlc != msg.dlc)
    return;

  // Decode fields.
  const uint16_t rx_uid_0 = (uint16_t)decode_signal(&msg.signals[0], data);
  const uint16_t rx_uid_1 = (uint16_t)decode_signal(&msg.signals[1], data);
  const uint16_t rx_uid_2 = (uint16_t)decode_signal(&msg.signals[2], data);
  const uint8_t rx_session_id = (uint8_t)decode_signal(&msg.signals[3], data);

  // Session must match.
  if (session_id != rx_session_id)
    return;

  // Store UID.
  discovered_uids_0[discovered_nodes] = rx_uid_0;
  discovered_uids_1[discovered_nodes] = rx_uid_1;
  discovered_uids_2[discovered_nodes] = rx_uid_2;

  // Increment index.
  discovered_nodes += 1;
}

void can_rx_can_id_allocator_ack(const can_header_t *header,
                                 const uint8_t *data) {
  const can_message_t msg = allocation_dbc
      [CAN_ID_ALLOCATION_DBC_IDX_NODE_ID_ACK_00]; // Reference only message.

  if (!header || !data)
    return;
  if (allocator_state != ALLOCATOR_AWAIT_ACK)
    return;

  // Validate message_id and node_id from CAN ID.
  can_message_id_t rx_msg_id = 0;
  can_node_id_t rx_node_id_in_can_id = 0;
  if (!can_id_unpack(header->standard_id, &rx_msg_id, &rx_node_id_in_can_id))
    return;
  if (rx_msg_id != (can_message_id_t)CAN_MSG_ENUM_ACK)
    return;
  if (rx_node_id_in_can_id == CAN_ID_NODE_ID_UNASSIGNED ||
      rx_node_id_in_can_id == CAN_ID_NODE_ID_BROADCAST)
    return;

  // DLC must match.
  if (header->dlc != msg.dlc)
    return;

  // Decode fields (using ACK message[0] signals as reference for decoding).
  const uint16_t rx_uid_0 = (uint16_t)decode_signal(&msg.signals[0], data);
  const uint16_t rx_uid_1 = (uint16_t)decode_signal(&msg.signals[1], data);
  const uint16_t rx_uid_2 = (uint16_t)decode_signal(&msg.signals[2], data);
  const uint8_t rx_session_id = (uint8_t)decode_signal(&msg.signals[3], data);
  const can_node_id_t rx_node_id_in_data =
      (can_node_id_t)decode_signal(&msg.signals[4], data);

  // Session must match.
  if (session_id != rx_session_id)
    return;

  // Node ID in data must match node ID in CAN ID.
  if (rx_node_id_in_data != rx_node_id_in_can_id)
    return;

  // Find UID and validate match.
  const int16_t uid_index = search_received_uids(rx_uid_0, rx_uid_1, rx_uid_2);
  if (uid_index < 0)
    return;

  // Successful Node ID assignment confirmed.
  assigned_node_ids[rx_node_id_in_data] = (uint8_t)uid_index;
  assignment_acked_nodes += 1;
}

void can_id_allocator_state_machine(void) {
  uint8_t tx_data[8] = {0};
  can_message_t msg = {0};

  switch (allocator_state) {
  case ALLOCATOR_IDLE:
    // State transition via can_id_allocator_start().
    break;

  case ALLOCATOR_DISCOVER:
    // Increment session ID for new allocation session (discovery) initiated.
    session_id += 1;

    // Pack signals and send.
    msg = allocation_dbc[CAN_ID_ALLOCATION_DBC_IDX_NODE_ID_DISCOVER];
    pack_signal_raw32(&msg.signals[0], tx_data, session_id);
    config.can_tx_func(&msg, tx_data);

    allocator_state = ALLOCATOR_AWAIT_ADVERTISE; // State transition.
    break;

  case ALLOCATOR_AWAIT_ADVERTISE:
    // TODO: Add a software timed limit/watchdog of some kind?

    // Transition state if max nodes is found.
    if (discovered_nodes >= CAN_ID_MAX_NODES) {
      allocator_state = ALLOCATOR_ASSIGN;
    }

    // Timed state transition via can_id_allocator_end_discovery().
    break;

  case ALLOCATOR_ASSIGN:
    // Run Node ID assignment strategy, assignments set in assigning_node_ids.
    node_id_strategy();

    // Transmit the assignment with each related UID.
    for (uint8_t i = 0; i < discovered_nodes; i++) {
      // Pack signals and send.
      msg = allocation_dbc[CAN_ID_ALLOCATION_DBC_IDX_NODE_ID_ASSIGN];
      pack_signal_raw32(&msg.signals[0], tx_data, discovered_uids_0[i]);
      pack_signal_raw32(&msg.signals[1], tx_data, discovered_uids_1[i]);
      pack_signal_raw32(&msg.signals[2], tx_data, discovered_uids_2[i]);
      pack_signal_raw32(&msg.signals[3], tx_data, session_id);
      pack_signal_raw32(&msg.signals[4], tx_data, assigning_node_ids[i]);
      pack_signal_raw32(&msg.signals[5], tx_data, 0u);
      config.can_tx_func(&msg, tx_data);

      memset(tx_data, 0, sizeof(tx_data)); // Clear TX data buffer.

      soft_assigned_nodes += 1; // Increment assigned, but not yet ACKed count.
    }

    allocator_state = ALLOCATOR_AWAIT_ACK; // State transition.
    break;

  case ALLOCATOR_AWAIT_ACK:
    // TODO: Add a software timed limit/watchdog of some kind?

    // Transition state if once all previously assigned nodes ACK.
    if (soft_assigned_nodes == assignment_acked_nodes) {

      // Call assignment complete callback.
      config.allocator_assigned_func(discovered_uids_0, discovered_uids_1,
                                     discovered_uids_2, assigned_node_ids,
                                     assignment_acked_nodes);

      // State transition.
      allocator_state = ALLOCATOR_IDLE;
    }
    break;

  default:
    break;
  }
}
