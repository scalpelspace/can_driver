# Changelog

---

<details markdown="1">
  <summary>Table of Contents</summary>

<!-- TOC -->
* [Changelog](#changelog)
  * [v0.1.0 (2026-01-13)](#v010--2026-01-13-)
  * [v0.2.0 (2026-03-08)](#v020--2026-03-08-)
  * [v0.3.0 (2026-03-10)](#v030--2026-03-10-)
  * [v0.3.2 (TBD)](#v032--tbd-)
<!-- TOC -->

</details>

---

## [v0.1.0 (2026-01-13)](https://github.com/scalpelspace/can_driver/releases/tag/v0.1.0)

- Initial release.

---

## [v0.2.0 (2026-03-08)](https://github.com/scalpelspace/can_driver/releases/tag/v0.2.0)

- Implement application-layer node-addressed CAN ID protocol.
    - Add [`can_id.c`](can_id.c) and [`can_id.h`](can_id.h).
    - Implement CAN ID allocation (allocator and allocatee) logic.
        - Add [`can_id_allocatee.c`](can_id_allocatee.c) and [
          `can_id_allocatee.h`](can_id_allocatee.h).
        - Add [`can_id_allocation_dbc.c`](can_id_allocation_dbc.c) and [
          `can_id_allocation_dbc.h`](can_id_allocation_dbc.h).
        - Add [`can_id_allocator.c`](can_id_allocator.c) and [
          `can_id_allocator.h`](can_id_allocator.h).
- Add merged DBC python script [
  `generate_merged_dbc.py`](generate_merged_dbc.py).
- Improve QOL print messages in [`generate_can_defs.py`](generate_can_defs.py).

---

## [v0.3.0 (2026-03-10)](https://github.com/scalpelspace/can_driver/releases/tag/v0.3.0)

- Update [`generate_can_defs.py`](generate_can_defs.py) for per CAN message
  index enum type generation with consolidated `..._IDX_COUNT` value.
    - Remove old `dbc_message_count` implementation, backwards compatibility
      loss.
- Update [`can_id_scalpelspace.dbc`](can_id_scalpelspace.dbc) for specific
  transmitter and receiver nodes.
- Swap local code DBCs ([`can_id_allocation_dbc.c`](can_id_allocation_dbc.c)
  and [`can_id_allocation_dbc.h`](can_id_allocation_dbc.h)) to near full
  auto-generated versions.
    - Allocation DBC messages renamed to use the short form "ACK".
    - Previously used a custom structure, updated to use index enum design.

> **Post Release Notes:**
> - The `v0.3.0` entry in [`CHANGELOG.md`](CHANGELOG.md) uses the
    incorrect hyperlink. Incorrectly linked to `v0.2.1`, but should be to
    `v0.3.0`.

---

## [v0.3.2 (TBD)](https://github.com/scalpelspace/can_driver/releases/tag/v0.3.2)

- Fix [`CHANGELOG.md`](CHANGELOG.md) for bad hyperlink on release `v0.3.0`.
- Reduce allocation DBC memory usage by removing per-node ACK message records.
    - Previously each of the 30 assignable node IDs had a dedicated ACK DBC
      entry. Now only a reference ACK entry (node ID 0) is generated in code.
    - The node ID bits in the CAN ID are set at runtime via bit operations.
    - The source DBC retains the full per-node ACK records as the true
      definition. The generated code is a deliberate reduction from the
      true DBC.
- Implement CAN Message ID and Node ID validation in
  [`can_id_allocator.c`](can_id_allocator.c) `can_rx_can_id_allocator_ack()`.
- Fix `can_rx_can_id_allocatee_advertise` renamed to
  `can_rx_can_id_allocator_advertise` to correctly reflect owning module.
