#ifndef CAN_NODE_H
#define CAN_NODE_H

/**
 * This library provides several interfaces for working with the CAN
 * node hardware developed by the ASL. This includes a standard status
 * message and backend data stores.
 *
 * Expected usage includes for a new node (eg. hil_node) to create a
 * .c file (HilNode.c) and to create an init function (HilNodeInit(void))
 * and to populate the nodeId variable with a value from the CAN_NODE_ID
 * enum. This function should also do any necessary non-PIC-peripheral
 * configuration. Additionally its header file (HilNode.h) should fully 
 * define the bits in the status and errors global variables defined in
 * this library.
 */

/**
 * This macro provides a way to handle fatal errors on the CAN node, where a red error LED is
 * available. This macro turns that LED on then sits and spins in a forever-loop.
 */
#define FATAL_ERROR() while(1)

#endif // CAN_NODE_H
