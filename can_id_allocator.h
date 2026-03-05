/*******************************************************************************
 * @file can_id_allocator.h
 * @brief CAN ID standard (allocator) for ScalpelSpace specific nodes.
 *******************************************************************************
 */

#ifndef CAN_DRIVER__CAN_ID_ALLOCATOR_H
#define CAN_DRIVER__CAN_ID_ALLOCATOR_H

/** Includes. *****************************************************************/

#include "can_driver.h"
#include "can_id_allocatee.h" // Included for can_tx_func_t type definition.
#include <stdbool.h>
#include <stdint.h>

/** Public types. *************************************************************/

/**
 * @brief Define function pointer type for post allocatee node ID assignment.
 *
 * @param uids_0
 * @param uids_1
 * @param uids_2
 * @param node_ids
 * @param node_count
 */
typedef void (*allocator_assigned_func_t)(
    uint16_t uids_0[CAN_ID_MAX_NODES], uint16_t uids_1[CAN_ID_MAX_NODES],
    uint16_t uids_2[CAN_ID_MAX_NODES], can_node_id_t node_ids[CAN_ID_MAX_NODES],
    can_node_id_t node_count);

typedef struct allocator_config {
  can_tx_func_t can_tx_func; // CAN message transmit function pointer.
  allocator_assigned_func_t
      allocator_assigned_func; // Allocation success callback.
} allocator_config_t;

/** Public functions. *********************************************************/

/**
 * @breif CAN RX callback function for allocator advertise message processing.
 *
 * @param header
 * @param data
 */
void can_rx_can_id_allocatee_advertise(const can_header_t *header,
                                       const uint8_t *data);

/**
 * @breif CAN RX callback function for allocator ack message processing.
 *
 * @param header
 * @param data
 */
void can_rx_can_id_allocator_ack(const can_header_t *header,
                                 const uint8_t *data);

/**
 * @breif Begin CAN ID allocator state machine.
 *
 * @param allocator
 *
 * @return Success status.
 * @retval true -> Allocator state machine started successfully.
 * @retval false -> Error.
 */
bool can_id_allocator_start(allocator_config_t allocator);

/**
 * @breif Manually end CAN ID allocator discovery state.
 *
 * @return Success status.
 * @retval true -> Allocator ended discovery successfully.
 * @retval false -> Error.
 */
bool can_id_allocator_end_discovery(void);

/**
 * @breif Run CAN ID allocator state machine.
 */
void can_id_allocator_state_machine(void);

#endif
