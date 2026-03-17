# Changelog

---

<details markdown="1">
  <summary>Table of Contents</summary>

<!-- TOC -->
* [Changelog](#changelog)
  * [v0.1.0 (2026-01-13)](#v010--2026-01-13-)
  * [v0.2.0 (2026-03-08)](#v020--2026-03-08-)
  * [v0.3.0 (2026-03-10)](#v030--2026-03-10-)
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
> - Error: This CHANGELOG record has the incorrect hyperlink. Incorrectly
    directs to `v0.2.1`, should be `v0.3.0`.
