# Changelog

---

<details markdown="1">
  <summary>Table of Contents</summary>

<!-- TOC -->
* [Changelog](#changelog)
  * [v0.1.0 (2026-01-13)](#v010--2026-01-13-)
  * [v0.2.0 (2026-03-08)](#v020--2026-03-08-)
  * [v0.3.0 (2026-03-10)](#v030--2026-03-10-)
  * [v0.3.2 (2026-03-16)](#v032--2026-03-16-)
  * [v0.3.3 (2026-03-18)](#v033--2026-03-18-)
  * [v0.3.5 (2026-04-18)](#v035--2026-04-18-)
  * [v0.3.6 (2026-04-22)](#v036--2026-04-22-)
  * [v0.3.7 (2026-05-01)](#v037--2026-05-01-)
  * [v0.3.8 (2026-05-02)](#v038--2026-05-02-)
  * [v0.4.0 (2026-07-06)](#v040--2026-07-06-)
  * [v0.4.1 (2026-07-10)](#v041--2026-07-10-)
  * [v0.5.0 (2026-07-10)](#v050--2026-07-10-)
  * [v0.6.0 (2026-09-01)](#v060--2026-09-01-)
<!-- TOC -->

</details>

---

## [v0.1.0 (2026-01-13)](https://github.com/scalpelspace/can_driver/releases/tag/v0.1.0)

- Initial release.

---

## [v0.2.0 (2026-03-08)](https://github.com/scalpelspace/can_driver/releases/tag/v0.2.0)

- Implement application-layer node-addressed CAN ID protocol.
    - Add `can_id.c` and `can_id.h`.
    - Implement CAN ID allocation (allocator and allocatee) logic.
        - Add `can_id_allocatee.c` and `can_id_allocatee.h`.
        - Add `can_id_allocation_dbc.c` and `can_id_allocation_dbc.h`.
        - Add `can_id_allocator.c` and `can_id_allocator.h`.
- Add merged DBC python script `generate_merged_dbc.py`.
- Improve QOL print messages in `generate_can_defs.py`.

---

## [v0.3.0 (2026-03-10)](https://github.com/scalpelspace/can_driver/releases/tag/v0.3.0)

- Update `generate_can_defs.py` for per CAN message index enum type generation
  with consolidated `..._IDX_COUNT` value.
    - Remove old `dbc_message_count` implementation, backwards compatibility
      loss.
- Update `can_id_scalpelspace.dbc` for specific transmitter and receiver nodes.
- Swap local code DBCs (`can_id_allocation_dbc.c` and `can_id_allocation_dbc.h`)
  to near full auto-generated versions.
    - Allocation DBC messages renamed to use the short form "ACK".
    - Previously used a custom structure, updated to use index enum design.

> **Post Release Notes:**
> - The v0.3.0 entry in `CHANGELOG.md` uses the incorrect hyperlink. Incorrectly
    linked to v0.2.1, but should be to v0.3.0.

---

## [v0.3.2 (2026-03-16)](https://github.com/scalpelspace/can_driver/releases/tag/v0.3.2)

- Fix `CHANGELOG.md` for bad hyperlink on release v0.3.0.
- Reduce allocation DBC memory usage by removing per-node ACK message records.
    - Previously each of the 30 assignable node IDs had a dedicated ACK DBC
      entry. Now only a reference ACK entry (node ID 0) is generated in code.
    - The node ID bits in the CAN ID are set at runtime via bit operations.
    - The source DBC retains the full per-node ACK records as the true
      definition. The generated code is a deliberate reduction from the true
      DBC.
    - Update documentation accordingly.
- Implement CAN Message ID and Node ID validation in `can_id_allocator.c`
  `can_rx_can_id_allocator_ack()`.
- Fix `can_rx_can_id_allocatee_advertise` renamed to
  `can_rx_can_id_allocator_advertise` to correctly reflect owning module.

---

## [v0.3.3 (2026-03-18)](https://github.com/scalpelspace/can_driver/releases/tag/v0.3.3)

- Add per-repo node ID support to `generate_merged_dbc.py`.
    - Repo specs now accept `url[@branch][#node_id]` format.
    - CAN IDs are patched at merge time using the ScalpelSpace ID scheme.
    - Allocation protocol messages (message_id 56..63) are excluded from
      patching.
    - Transmitter node names in `BU_` and `BO_` lines are suffixed by node ID
      (e.g. `MOMENTUM` -> `MOMENTUM_02`). Shared roles (`LISTENER`, `REQUESTER`,
      `COMMANDER`) are not suffixed. Message names are always preserved as-is.
    - Duplicate node ID assignments across repos produce a warning.
    - Update related documentation.

---

## [v0.3.5 (2026-04-18)](https://github.com/scalpelspace/can_driver/releases/tag/v0.3.5)

- Implement initial assignment strategy allowing custom developer
  implementations to Node ID assignment based on UID.
    - Added strategy function pointer `node_id_assignment_strategy_t` to
      `allocator_config_t`.
    - If NULL, behaviour is unchanged (FIFO). Three built-in strategies
      provided:
        - `can_id_strategy_fifo`: assignment by discovery order (previous
          hardcoded behaviour).
        - `can_id_strategy_uid_ascending`: deterministic assignment sorted by
          48-bit UID value.
        - `can_id_strategy_uid_table`: fixed UID to Node ID lookup table, with
          fallback sequential assignment for unknown nodes
          (`can_id_strategy_uid_table_set` to configure).
- `search_received_uids` marked static.
- Update `generate_merged_dbc.py` to support local DBC files.
    - Backwards compatible using the same repos file.
    - Update documentation accordingly.
- Update `generate_merged_dbc.py` to fix missing signal receiver Node ID suffix
  patching.
- Fix minor documentation formatting.
- Rename `LICENSE.txt` to `LICENSE`.

---

## [v0.3.6 (2026-04-22)](https://github.com/scalpelspace/can_driver/releases/tag/v0.3.6)

- Fix `sign_extend_u32()` to perform arithmetic (not logical) right shift so
  negative signed signals decode correctly.

---

## [v0.3.7 (2026-05-01)](https://github.com/scalpelspace/can_driver/releases/tag/v0.3.7)

- Reorder state machine logic so the allocation session completion callbacks
  fire after state is set to IDLE.
    - Previously, `allocatee_assigned_func` and `allocator_assigned_func` were
      called before `allocatee_state`/`allocator_state` were set to IDLE. Now
      the callbacks fire last, so state is already IDLE on callback entry.

---

## [v0.3.8 (2026-05-02)](https://github.com/scalpelspace/can_driver/releases/tag/v0.3.8)

- Improve state machine logic to allow recovery from potential state hangs.
    - Change return type of `can_rx_can_id_allocatee_discovery()` from `void` to
      `bool`.
        - Returns true only when a valid DISCOVER is accepted and the session
          starts.
    - Remove state guards in `can_id_allocator_start()` and
      `can_id_allocatee_start()` to allow for a forced reset from any state.

---

## [v0.4.0 (2026-07-06)](https://github.com/scalpelspace/can_driver/releases/tag/v0.4.0)

- Update `generate_can_defs.py` for design and performance improvements.
    - Added simple DBC multiplexing support (selector/dependent signal roles
      plus mux helpers).
    - Replaced fixed 8-slot signal arrays with per-message exact-size const
      arrays via pointer.
    - Update `can_id_allocation_dbc.c` accordingly.
- Update `generate_merged_dbc.py` to suffix message names with node ID in merged
  DBC to ensure unique names across node instances.

---

## [v0.4.1 (2026-07-10)](https://github.com/scalpelspace/can_driver/releases/tag/v0.4.1)

- Cleanup `CHANGELOG.md` for formatting and syntax consistency.
- Update `.gitignore` to exclude DBC tooling outputs and local inputs
  (`workspace/`, `project.dbc`, `repos.txt`).
- Add optional `--symbol-name` argument to `generate_can_defs.py` to set the
  generated `can_message_t` array symbol name (default: `dbc_messages`).
    - Formalizes the previously manual rename to `allocation_dbc` used to avoid
      symbol conflicts when linking alongside a device DBC in the same build.
    - Update documentation accordingly.
- Update `generate_merged_dbc.py` to error cleanly when no `.dbc` files are
  found (previously raised an unhandled `IndexError`).
- Fix reserved identifier header guard in `can_driver.h`
  (`__CAN_DRIVER_H` -> `CAN_DRIVER__CAN_DRIVER_H`) for consistency with other
  headers.
- Add defensive input guards to `physical_to_raw()` for NULL signal and zero bit
  length inputs (previously undefined behaviour), now returns 0.
- Fix `decode_signal()` to return `double` literals (`0.0f` -> `0.0`).

---

## [v0.5.0 (2026-07-10)](https://github.com/scalpelspace/can_driver/releases/tag/v0.5.0)

- Fix out-of-bounds write in `can_id_allocator.c` on ACK from Node ID 30.
    - Replace `assigned_node_ids` (indexed by Node ID, which overflowed the
      30-entry array at Node ID 30) with `acked_node_ids`, index-aligned with
      the discovered UID arrays.
    - The `allocator_assigned_func` callback now receives ACKed Node IDs
      index-aligned with the UID arrays, matching the documented strategy
      convention. Previously it received UID indexes indexed by Node ID.
    - The callback `node_count` parameter now reports the discovered node count
      (the valid length of all passed arrays), entries that did not ACK hold
      Node ID 0.
- Fix unsigned 32-bit signal decoding in `can_driver.c`.
    - `raw_to_physical()` cast unsigned raw values through `int32_t`, so 32-bit
      unsigned signals above `INT32_MAX` decoded as negative. Unsigned values
      now convert directly to `double`. Signals of 31 bits or fewer are
      unaffected.
- Fix allocator UID search (`search_received_uids`) to only scan discovered
  entries.
    - Previously all 30 slots were scanned, so an ACK carrying UID 0 could match
      an empty (zero-initialized) slot and count as a valid assignment.
- Add duplicate ADVERTISE and ACK rejection in `can_id_allocator.c`.
    - Retransmitted ADVERTISE messages previously consumed extra discovery slots
      and received multiple Node ID assignments, deadlocking the allocator in
      the AWAIT_ACK state (ACK count could never reach the assigned count).
    - Retransmitted ACK messages previously double-counted, with the same
      deadlock result.
- Fix assignment handling of nodes unresolved by the assignment strategy.
    - The allocator now skips ASSIGN transmission for entries left at Node ID 0
      (e.g. `can_id_strategy_uid_table` with a full bus).
    - The allocatee now rejects ASSIGN messages carrying reserved Node IDs (0 =
      unassigned, 31 = broadcast) or values above the assignable range.
- Validate required function pointers in `can_id_allocator_start()` and
  `can_id_allocatee_start()`.
    - Return `false` if `can_tx_func` (allocator/allocatee) or
      `get_uid_hash48_func` (allocatee) is NULL, instead of hard faulting
      mid-protocol. Previously documented as "Always true".
    - Success callbacks (`allocator_assigned_func`, `allocatee_assigned_func`)
      are now optional and skipped when NULL.
- Reset ACK tracking state on `can_id_allocator_start()` for clean session
  restarts.
- Guard the allocatee ACK transmission on `can_id_pack()` success to avoid
  transmitting a malformed CAN ID (defensive, unreachable after the assignment
  range validation above).
- Update `README.md` implementer notes for the above behaviours and fix stale
  strategy documentation.
    - Custom assignment strategies are configured via
      `allocator_config_t::strategy` (previously incorrectly referenced the
      private `node_id_strategy` helper).
    - Fix `can_id_strategy_uid_table` fallback description in
      `can_id_allocator.h`: unknown nodes receive the lowest free Node IDs
      (previously claimed IDs start above the highest table-mapped ID).
- Add `.gitattributes`.

---

## [v0.6.0 (2026-09-01)](https://github.com/scalpelspace/can_driver/releases/tag/v0.6.0)

- Fix allocators handing out a Node ID that is already in use.
    - Assigned nodes stayed silent during discovery, so the allocator could not
      see them and could assign their Node ID to another node.
    - Every allocatee now answers DISCOVER. After ACKing it returns to awaiting
      discovery instead of going idle, so it re-advertises on the next session.
- Add the `alloc_mode` field to ADVERTISE (`message_id` 57, payload byte 7).
    - Replaces the unused `RESERVED_ADVERTISE` byte. The full 8-bit field is
      taken, only bit 0 is defined and bits 1..7 stay reserved.
    - `0` = reassignable (default), `1` = not reassignable. `0` is the
      reassignable value so firmware predating the field, which transmitted byte
      7 as a zeroed reserved byte, decodes correctly.
    - The allocatee packs `alloc_mode` masked to the defined bit so the reserved
      bits are always transmitted as 0.
    - Nodes advertising `1` are marked reserved by the allocator: their Node ID
      is removed from the pool offered to the assignment strategy and no ASSIGN
      is transmitted for them.
- Carry the current Node ID in the ADVERTISE CAN ID.
    - Assigned nodes advertise at `0x720 | node_id` (0x720..0x73E), unassigned
      nodes keep advertising at 0x720, matching the existing ACK pattern.
    - `can_rx_can_id_allocator_advertise()` validates the message ID and Node ID
      out of the CAN ID (as `can_rx_can_id_allocator_ack()` already did)
      instead of matching a single fixed CAN ID.
    - The source DBC gains the per-node ADVERTISE records (`node_id_advertise`
      renamed to `node_id_advertise_00`..`_31`) as the true definition, while
      the generated code DBC keeps only the Node 0 record. This is the reduction
      already used for ACK, the Node ID bits are set at runtime via bit
      operations.
    - `CAN_ID_ALLOCATION_DBC_IDX_NODE_ID_ADVERTISE` renamed to
      `CAN_ID_ALLOCATION_DBC_IDX_NODE_ID_ADVERTISE_00`, backwards compatibility
      loss.
- Add `allocatee_config_t::alloc_mode` and `allocatee_config_t::node_id`.
    - `node_id` seeds the Node ID the allocatee starts with (0 = unassigned), so
      a node holding an ID across a reset advertises it immediately.
    - `can_id_allocatee_start()` now also returns `false` for an undefined
      `alloc_mode`, a `node_id` above the assignable range, or
      `CAN_ALLOC_MODE_NOT_REASSIGNABLE` combined with `node_id = 0`.
    - Allocatees configured as `CAN_ALLOC_MODE_NOT_REASSIGNABLE` ignore ASSIGN.
- Replace the Node ID assignment strategy parameters with
  `node_id_assignment_ctx_t`, backwards compatibility loss.
    - Strategies now receive the discovered UID arrays, per-node reserved flags,
      node count and a `reserved_mask` bitmap of the Node IDs held by nodes that
      refuse reassignment.
    - `can_id_strategy_fifo`, `can_id_strategy_uid_ascending` and
      `can_id_strategy_uid_table` assign the lowest free Node IDs, skipping
      reserved nodes and the Node IDs they hold.
    - The allocator drops any assignment that lands on a reserved Node ID or
      falls outside the assignable range, so a custom strategy that ignores
      `reserved_mask` cannot cause a collision.
    - Assignment results are cleared before each strategy run, so entries a
      strategy leaves untouched no longer carry over from a previous session.
- Ignore ACKs from reserved nodes in `can_rx_can_id_allocator_ack()`.
    - Reserved nodes are never assigned, counting their ACK would leave the
      allocator stuck in the AWAIT_ACK state.
- Update `README.md` and header documentation for the above behaviours.
