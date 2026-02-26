/*******************************************************************************
 * @file can_id_allocation_dbc.c
 * @brief DBC for CAN ID standard (allocation) for ScalpelSpace specific nodes.
 *******************************************************************************
 */

#ifndef CAN_DRIVER__CAN_ID_ALLOCATION_DBC_H
#define CAN_DRIVER__CAN_ID_ALLOCATION_DBC_H

/** Includes. *****************************************************************/

#include "can_driver.h"

/** Public variables. *********************************************************/

extern const can_message_t message_discover;
extern const can_message_t message_advertise;
extern const can_message_t message_assign;
extern const can_message_t can_id_ack_dbc[32];

#endif
