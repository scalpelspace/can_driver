# can_driver

Low level simplified CAN bus (classic) communication drivers.

![black_formatter](https://github.com/scalpelspace/can_driver/actions/workflows/black_formatter.yaml/badge.svg)

---

<details markdown="1">
  <summary>Table of Contents</summary>

<!-- TOC -->

* [can_driver](#can_driver)
    * [1 CAN Bus Drivers](#1-can-bus-drivers)
    * [2 CAN ID ScalpelSpace Node Scheme](#2-can-id-scalpelspace-node-scheme)

<!-- TOC -->

</details>

---

## 1 CAN Bus Drivers

CAN drivers are implemented in the following files:

1. [can_driver.c](can_driver.c)
2. [can_driver.h](can_driver.h)

The CAN driver is intended to integrate with a `C` based DBC structure.

- To translate a DBC file into the custom CAN `C` based
  structures, [generate_can_defs.py](generate_can_defs.py) is used to generate
  the following files:

    1. example_dbc.c
    2. example_dbc.h

    - These generated files declare the message and signals in the appropriate
      type structs.

---

## 2 CAN ID ScalpelSpace Node Scheme

To ensure ecosystem functionality, ScalpelSpace specific node devices use a
custom CAN ID standard. Building off the 11-bit classic CAN ID structure, 3
fields are allocated to support message arbitration and node identification.

1. `priority`: High level priority in the CAN bus.
2. `message_type`: High level message type to classify general data content.
3. `node_id`: Individual device node on the network.
    - `0`: Reserved for "unassigned".
    - `31`: Reserved for "broadcast".
    - Allows up to 30 unique reporting devices on a single network.

The following table outlines the 11-bit allocation for the 3 fields:

| Bit index      | 10 | 9 | 8  | 7  | 6  | 5  | 4  | 3  | 2  | 1  | 0  |
|----------------|:--:|:-:|:--:|:--:|:--:|:--:|:--:|:--:|:--:|:--:|:--:|
| `priority`     | p  | p |    |    |    |    |    |    |    |    |    |
| `message_type` |    |   | mt | mt | mt | mt |    |    |    |    |    |
| `node_id`      |    |   |    |    |    |    | ni | ni | ni | ni | ni |

- Bit index 10 = MSB, 0 = LSB.

Drivers are implemented in the following files:

1. [can_id.c](can_id.c)
2. [can_id.h](can_id.h)
