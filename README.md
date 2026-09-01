# can_driver

Low level simplified CAN bus (classic) communication drivers.

![black_formatter](https://github.com/scalpelspace/can_driver/actions/workflows/black_formatter.yaml/badge.svg)

---

<details markdown="1">
  <summary>Table of Contents</summary>

<!-- TOC -->
* [can_driver](#can_driver)
  * [1 CAN Bus Drivers](#1-can-bus-drivers)
    * [1.1 CAN Message Signalize Size Limit](#11-can-message-signalize-size-limit)
  * [2 CAN ID ScalpelSpace Node Scheme](#2-can-id-scalpelspace-node-scheme)
    * [2.1 Node ID Allocation Protocol](#21-node-id-allocation-protocol)
    * [2.2 Implementer Notes](#22-implementer-notes)
  * [3 Generate Merged DBC python Script](#3-generate-merged-dbc-python-script)
    * [3.1 Usage](#31-usage)
    * [3.2 Repo File Format](#32-repo-file-format)
    * [3.3 Node ID Patching](#33-node-id-patching)
    * [3.4 Notable Behaviour](#34-notable-behaviour)
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
    - Pass `--symbol-name <name>` to rename the generated `can_message_t` array
      (default: `dbc_messages`). Required when linking multiple generated DBC
      files into one build to avoid symbol conflicts.

### 1.1 CAN Message Signalize Size Limit

To support lower-performance, commonly used MCUs, the signal packing/unpacking
API limits individual signal values to `uint32_t`.

Signals larger than 32 bits must be represented as multiple <= 32-bit signals.

---

## 2 CAN ID ScalpelSpace Node Scheme

To ensure ecosystem functionality, ScalpelSpace specific node devices use a
custom CAN ID standard. Building off the 11-bit classic CAN ID structure, 2
fields are allocated to support message arbitration and node identification.

1. `message_id`: High level message type to classify general data content. (bits
   10..5, 6-bit field, range 0..63).
2. `node_id`: Individual device node on the network. (bits 4..0, 5-bit field,
   range 0..31).
    - `0`: Reserved for "unassigned".
    - `31`: Reserved for "broadcast".
    - Allows up to 30 unique reporting devices on a single network.

The following table outlines the 11-bit allocation for the 2 fields:

| Bit index    | 10  |  9  |  8  |  7  |  6  |  5  |  4   |  3   |  2   |  1   |  0   |
|--------------|:---:|:---:|:---:|:---:|:---:|:---:|:----:|:----:|:----:|:----:|:----:|
| `message_id` | msg | msg | msg | msg | msg | msg |      |      |      |      |      |
| `node_id`    |     |     |     |     |     |     | node | node | node | node | node |

- Bit index 10 = MSB, 0 = LSB.
- Standard 11-bit CAN frames only (IDE = 0). Extended IDs are not used.

Drivers are implemented in the following files:

1. [can_id.c](can_id.c)
2. [can_id.h](can_id.h)

Local code DBCs are implemented in the following files
(using [generate_can_defs.py](generate_can_defs.py)):

1. [can_id_allocation_dbc.h](can_id_allocation_dbc.h)
2. [can_id_allocation_dbc.c](can_id_allocation_dbc.c)

> **Note:**
> 1. The message array is generated with `--symbol-name allocation_dbc` to avoid
     symbol conflicts with per-device DBC files in the same build (the default
     symbol name is `dbc_messages`).
> 2. The source DBC retains the per-node ADVERTISE and ACK records as the true
     definition. The generated code DBC uses a deliberate reduction, only
     maintaining the Node 0 ADVERTISE and ACK records.

To create a merged DBC file from multiple sources (git repos)
[generate_merged_dbc.py](generate_merged_dbc.py) is used.

### 2.1 Node ID Allocation Protocol

Before a node can participate in the network it must be assigned a `node_id`
by a designated main allocator device. This is handled automatically at startup
via a 4-step handshake:

```
Allocator                          Allocatee(s)
    |                                   |
    |--- DISCOVER (broadcast) --------> |  Allocator opens discovery window.
    |                                   |
    | <---------- ADVERTISE (each) -----|  Every node replies with a UID hash48,
    |                                   |  its current `node_id` and its
    |                                   |  `alloc_mode`.
    |                                   |
    |--- ASSIGN (broadcast, per UID) -> |  Allocator assigns `node_id` per UID.
    |                                   |
    | <-------------- ACK (per node) ---|  Each node confirms its assignment.
    |                                   |
```

Each discovery run is tagged with an incrementing `session_id` (uint8_t, wraps
at 255) so that messages from a previous session are discarded.

Node identity is established using a **48-bit UID hash**, split across three
16-bit segments. Implementers must supply a `get_uid_hash48_func_t` callback
that returns values derived from hardware (e.g. MCU die ID). The hash must be
stable across resets and unique per device. Colliding hashes cause assignment
failure: the second node to advertise is treated as a duplicate of the first and
is silently excluded from the session.

Reserved `message_id` values for the allocation protocol:

| `message_id` | Name        |   `node_id`    | Full CAN ID | Description                          |
|:------------:|-------------|:--------------:|:-----------:|--------------------------------------|
|      56      | `DISCOVER`  | 31 (broadcast) |    0x71F    | Allocator opens discovery window.    |
|      57      | `ADVERTISE` |    current     | 0x720-0x73F | Node broadcasts its UID hash48.      |
|      58      | `ASSIGN`    | 31 (broadcast) |    0x75F    | Allocator assigns a UID a `node_id`. |
|      59      | `ACK`       |    assigned    | 0x760-0x77F | Node confirms its assignment.        |

ADVERTISE carries the `node_id` the sending node currently holds, so a node that
has never been assigned advertises at 0x720 (`CAN_ID_NODE_ID_UNASSIGNED`). Byte
7 of the payload is `alloc_mode`:

| `alloc_mode` | Name               | Meaning                                        |
|:------------:|--------------------|------------------------------------------------|
|      0       | `reassignable`     | Accepts a new `node_id` (default).             |
|      1       | `not reassignable` | Holds a fixed `node_id`, refuses reassignment. |

Only bit 0 is defined, bits 1..7 are reserved and transmitted as 0. Value 0
means "reassignable" so that firmware predating the field, which transmitted
byte 7 as a zeroed reserved byte, still decodes correctly.

`message_id` values 60..63 are reserved for future allocation use. Values 1..55
are free for application messages.

### 2.2 Implementer Notes

- **Single network instance only.** All allocator and allocatee state is held in
  module-level statics. Only one instance of each can run per build target.

- **Discovery window is manually closed.** The allocator does not use a fixed
  timer to end discovery. The host application must call
  `can_id_allocator_end_discovery()` when it decides the window is over. Plan
  for this in startup sequencing.

- **No built-in timeouts.** If an expected message never arrives (e.g. a node
  fails to ACK), the state machine stalls indefinitely. Both
  `can_id_allocator_start()` and `can_id_allocatee_start()` are safe to call
  from any state. They fully reset the state machine and begin a fresh session.
  Use an external watchdog or deadline timer to call them if startup reliability
  is required.
    - **Allocator:** arm the deadline timer at the `can_id_allocator_start()`
      call site, discovery begins immediately.
    - **Allocatee:** arm the deadline timer when
      `can_rx_can_id_allocatee_discovery()` returns `true`, which confirms that
      a valid session is in flight.

- **Single allocator per network.** Running more than one allocator
  simultaneously will produce conflicting DISCOVER and ASSIGN messages.
  Arbitrate allocator role at the application layer if needed.

- **Every node answers DISCOVER, including assigned ones.** A node that already
  holds a Node ID returns to awaiting discovery after ACKing, and advertises
  again on the next DISCOVER using its current Node ID in the CAN ID. This is
  what lets the allocator see which Node IDs are already in use instead of
  handing out an ID that is already taken.
    - The allocatee starts with `allocatee_config_t::node_id` (0 = unassigned),
      so a node that keeps its ID across a reset can advertise it immediately.

- **Nodes can refuse reassignment.** An allocatee configured with
  `allocatee_config_t::alloc_mode = CAN_ALLOC_MODE_NOT_REASSIGNABLE` advertises
  that it holds a fixed Node ID. The allocator marks that Node ID reserved,
  removes it from the pool offered to the assignment strategy, and sends no
  ASSIGN for that node. Such a node never ACKs, so it is reported through
  `allocator_assigned_func` with the Node ID it advertised.
    - A node that refuses reassignment must be told which ID it holds, so
      `can_id_allocatee_start()` rejects `CAN_ALLOC_MODE_NOT_REASSIGNABLE`
      combined with `node_id = 0`.
    - Two nodes hardcoded to the same Node ID is a wiring/configuration error
      the protocol cannot resolve. Both are marked reserved and neither is
      reassigned.

- **Node ID assignment order defaults to FIFO.** When
  `allocator_config_t::strategy` is NULL, nodes are assigned the lowest free
  Node IDs in the order their ADVERTISE messages are received, starting from
  `node_id = 1` and skipping any ID held by a reserved node.
    - FIFO assignment is not deterministic across power cycles if multiple nodes
      boot simultaneously. If stable node ID mapping matters, use one of the
      built-in strategies (`can_id_strategy_uid_ascending` or
      `can_id_strategy_uid_table`) or provide a custom
      `node_id_assignment_strategy_t` via `allocator_config_t::strategy`.
    - Nodes left unresolved by a strategy (`node_id = 0`, e.g.
      `can_id_strategy_uid_table` with no free Node IDs remaining) are skipped:
      no ASSIGN is transmitted for them and they do not join the session.
      Allocatees also reject ASSIGN messages carrying a reserved Node ID (0 or
        31) as a defence against a misbehaving allocator.
    - Custom strategies receive a `node_id_assignment_ctx_t` describing the
      session: the discovered UID arrays, which entries are reserved, and a
      `reserved_mask` bitmap of the Node IDs already in use. The allocator drops
      any assignment that lands on a reserved Node ID or falls outside the
      assignable range.

- **Configurations are validated on start.** `can_id_allocator_start()` and
  `can_id_allocatee_start()` return `false` (without starting) if required
  function pointers are NULL: `can_tx_func` for both, plus `get_uid_hash48_func`
  for the allocatee. The success callbacks (`allocator_assigned_func`,
  `allocatee_assigned_func`) are optional and may be NULL.
    - `can_id_allocatee_start()` also rejects an undefined `alloc_mode` and a
      `node_id` above the assignable range (31 = broadcast and up).

- **Duplicate messages within a session are ignored.** Retransmitted ADVERTISE
  and ACK messages (e.g. from CAN controller automatic retransmission) are
  detected by UID and dropped, so they do not consume discovery slots or
  double-count acknowledgements. Nodes re-advertise normally in response to a
  new DISCOVER since each `can_id_allocator_start()` resets discovery state.

## 3 Generate Merged DBC python Script

`generate_merged_dbc.py` clones multiple git repos and merges their root-level
`.dbc` files into a single combined `.dbc`. Each repo represents one node on the
ScalpelSpace CAN bus, and an optional node ID can be assigned per repo to patch
all CAN IDs to match the ScalpelSpace ID scheme at merge time.

### 3.1 Usage

```shell
python3 generate_merged_dbc.py --repos-file repos.txt --out project.dbc --workdir workspace
```

### 3.2 Repo File Format

One entry per line, 2 possible options:

1. `url[@branch][#node_id]`
    ```
    # Unassigned (node_id=0, CAN IDs left as-is).
    https://github.com/your_org/repo_a.git
    
    # Specific branch, unassigned.
    https://github.com/your_org/repo_b.git@main
    
    # Assigned node IDs.
    https://github.com/your_org/repo_c.git@main#1
    https://github.com/your_org/repo_d.git@main#2
    https://github.com/your_org/repo_d.git@main#3
    ```
2. `dbc_file_path[#node_id]`
    ```
    # Unassigned (node_id=0, CAN IDs left as-is).
    ./local/my_device.dbc
    /absolute/path/to/other_device.dbc
    
    # Assigned node IDs.
    ./local/my_device.dbc#1
    /absolute/path/to/other_device.dbc#2
    ```

Lines beginning with `#` are treated as comments.

### 3.3 Node ID Patching

When a node ID is specified, all CAN IDs in that repo's DBC are repacked using
the ScalpelSpace scheme (`message_id << 5 | node_id`). If no node ID is given,
CAN IDs are left unchanged (base DBC state, `node_id=0`).

Device node names in `BU_`, transmitter fields and message names are suffixed
with the node ID (e.g. `MOMENTUM` -> `MOMENTUM_02`, `mcu_temperature` ->
`mcu_temperature_02`) to uniquely identify each instance in the merged output.
Signal definitions are always preserved as-is from the source DBC.

Shared role names (`LISTENER`, `REQUESTER`, `COMMANDER`) are never suffixed.

### 3.4 Notable Behaviour

- **Local DBC files are supported.** Entries ending in `.dbc` or resolving to an
  existing file path are used directly without cloning. Node ID patching applies
  identically to remote repos. Local and remote entries can be freely mixed in
  the same repos file.
- **Allocation protocol messages are never patched.** Messages with
  `message_id >= 56` (CAN IDs 1792+) are reserved for the ScalpelSpace node ID
  allocation protocol and are left unchanged regardless of the assigned node ID.
- **Duplicate node IDs produce a warning.** Assigning the same node ID to
  multiple repos will cause CAN ID collisions in the merged output.
- **Conflicting CAN IDs keep the first occurrence.** If two repos define the
  same CAN ID after patching, the first is kept and a warning is emitted.
- **Same repo, multiple instances is supported.** The same repo URL can appear
  multiple times with different node IDs to represent multiple identical devices
  on the bus.
