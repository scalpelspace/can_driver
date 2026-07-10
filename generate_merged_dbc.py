"""Merged DBC generator.

Clone multiple git repos (each containing one or more .dbc files at the root
directory) and/or source local DBC files and merge them into a single merged
.dbc file.

Each repo entry can optionally specify a branch and a node ID using the format:
    url[@branch][#node_id]

Each file path entry can optionally specify a node ID using the format:
    dbc_file_path[#node_id]

Node ID must be in the assignable range (1..30). If omitted, defaults to 0
(unassigned), and CAN IDs are left as-is (base DBC state).

Example:
    ```shell
    # Unix.
    python3 generate_merged_dbc.py --repos-file repos.txt --out project.dbc --workdir workspace
    ```

    repos.txt example:
    ```
    https://github.com/your_org/repo_a.git
    https://github.com/your_org/repo_b.git@main
    https://github.com/your_org/repo_c.git@main#1
    https://github.com/your_org/repo_d.git@main#2
    https://github.com/your_org/repo_d.git@main#3
    ./local/my_device.dbc#4
    /absolute/path/to/other_device.dbc#5
    ```

    Creates a merged DBC named "project.dbc" with:
      - repo_a messages (main) patched to node_id=0.
      - repo_b messages (main) patched to node_id=0.
      - repo_c messages (main) patched to node_id=1.
      - repo_d messages (main) patched to node_id=2 and node_id=3 (2 instances).
      - ./local/my_device.dbc messages patched to node_id=4.
      - /absolute/path/to/other_device.dbc messages patched to node_id=5.
"""

from __future__ import annotations

import argparse
import re
import shutil
import subprocess
import sys
from dataclasses import dataclass, field
from pathlib import Path
from typing import Optional

REPOS: list[str] = [
    # "https://github.com/your_org/repo1.git",
    # "https://github.com/your_org/repo2.git@main#2"
]

# ScalpelSpace CAN ID scheme constants.
NODE_ID_BITS = 5
NODE_ID_MASK = (1 << NODE_ID_BITS) - 1  # 0x1F
MESSAGE_ID_SHIFT = NODE_ID_BITS  # 5
NODE_ID_UNASSIGNED = 0
NODE_ID_BROADCAST = 31
CAN_ID_MAX_ASSIGNABLE = 30  # 1..30 inclusive.

# Node names that represent shared roles rather than specific devices, these are
# not suffixed.
SHARED_NODE_ROLES = {"LISTENER", "REQUESTER", "COMMANDER"}

MSG_START_RE = re.compile(r"^BO_\s+(\d+)\s+(\w+)\s*:\s*(\d+)\s+(\S+)")
NODE_LINE_RE = re.compile(r"^BU_:\s*(.*)$")
# Optional multiplexer token (M, m<N> or m<N>M) between signal name and colon.
SG_LINE_RE = re.compile(
    r"^(\s*SG_\s+\w+\s*(?:M|m\d+M?)?\s*:.+\"[^\"]*\")\s+(\S+)$"
)

# Non-BO_ lines that reference a CAN ID as their first numeric field. These
# must be ID-patched alongside their message during node ID assignment.
ID_REF_LINE_RES = [
    re.compile(r"^(VAL_\s+)(\d+)(\s+.*)$"),
    re.compile(r"^(CM_\s+SG_\s+)(\d+)(\s+.*)$"),
    re.compile(r"^(CM_\s+BO_\s+)(\d+)(\s+.*)$"),
    re.compile(r"^(BA_\s+\"[^\"]*\"\s+SG_\s+)(\d+)(\s+.*)$"),
    re.compile(r"^(BA_\s+\"[^\"]*\"\s+BO_\s+)(\d+)(\s+.*)$"),
    re.compile(r"^(SIG_VALTYPE_\s+)(\d+)(\s+.*)$"),
    re.compile(r"^(BO_TX_BU_\s+)(\d+)(\s+.*)$"),
    re.compile(r"^(SG_MUL_VAL_\s+)(\d+)(\s+.*)$"),
]


def patch_can_id(can_id: int, node_id: int) -> int:
    """Repack a ScalpelSpace CAN ID with a new node_id.

    Extracts the message_id from bits 10..5 and repacks with the given node_id
    in bits 4..0.

    Args:
        can_id: Original 11-bit CAN ID (node_id assumed 0 in base DBC).
        node_id: Assignable node ID (1..30).

    Returns:
        Patched 11-bit CAN ID.
    """
    message_id = (can_id >> MESSAGE_ID_SHIFT) & 0x3F  # Extract bits 10..5.
    return (message_id << MESSAGE_ID_SHIFT) | (node_id & NODE_ID_MASK)


def validate_node_id(node_id: int, spec: str) -> bool:
    """Validate node_id is within the assignable range."""
    if node_id == NODE_ID_UNASSIGNED:
        return True  # 0 is valid (unassigned/base state).
    if node_id == NODE_ID_BROADCAST:
        print(
            f"[ERROR] node_id=31 is reserved for broadcast: {spec}",
            file=sys.stderr,
        )
        return False
    if not (1 <= node_id <= CAN_ID_MAX_ASSIGNABLE):
        print(
            f"[ERROR] node_id={node_id} out of assignable range (1..30): {spec}",
            file=sys.stderr,
        )
        return False
    return True


@dataclass
class OrderedSet:
    items: list[str] = field(default_factory=list)
    seen: set[str] = field(default_factory=set)

    def add(self, line: str) -> None:
        if line not in self.seen:
            self.seen.add(line)
            self.items.append(line)


@dataclass
class DBCDoc:
    header_lines: list[str] = field(default_factory=list)
    nodes: OrderedSet = field(default_factory=OrderedSet)
    messages: dict[int, list[str]] = field(default_factory=dict)
    message_order: list[int] = field(default_factory=list)
    other_lines: OrderedSet = field(default_factory=OrderedSet)


@dataclass
class RepoSpec:
    url: str
    branch: Optional[str]
    node_id: int  # 0 = unassigned (base DBC, no patching).
    local_path: Optional[Path] = None  # Set if entry is a local file path.


def _split_repo_spec(spec: str) -> RepoSpec:
    """Parse a repo spec of the form url[@branch][#node_id] or
    path/to/file.dbc[#node_id].
    """
    node_id = NODE_ID_UNASSIGNED

    # Extract node_id suffix (#N).
    if "#" in spec:
        spec, node_id_str = spec.rsplit("#", 1)
        try:
            node_id = int(node_id_str.strip())
        except ValueError:
            print(
                f"[WARN] Invalid node_id '{node_id_str}' in spec '{spec}', "
                f"defaulting to 0.",
                file=sys.stderr,
            )
            node_id = NODE_ID_UNASSIGNED

    # Check if this is a local file path (ends in .dbc or exists on disk).
    candidate = Path(spec.strip())
    if candidate.suffix == ".dbc" or candidate.exists():
        return RepoSpec(
            url="", branch=None, node_id=node_id, local_path=candidate
        )

    # Extract branch suffix (@branch).
    branch = None
    if "@" in spec:
        url, branch = spec.rsplit("@", 1)
        branch = branch.strip() or None
    else:
        url = spec

    return RepoSpec(url=url.strip(), branch=branch, node_id=node_id)


def run(cmd: list[str], cwd: Optional[Path] = None) -> None:
    proc = subprocess.run(cmd, cwd=str(cwd) if cwd else None)
    if proc.returncode != 0:
        raise RuntimeError(f"Command failed: {' '.join(cmd)}")


def git_clone_or_update(url: str, branch: Optional[str], dst: Path) -> None:
    if dst.exists() and (dst / ".git").exists():
        run(["git", "fetch", "--all", "--prune"], cwd=dst)
        if branch:
            run(["git", "checkout", branch], cwd=dst)
        run(["git", "pull", "--ff-only"], cwd=dst)
        return

    if dst.exists():
        shutil.rmtree(dst)

    cmd = ["git", "clone"]
    if branch:
        cmd += ["--branch", branch]
    cmd += [url, str(dst)]
    run(cmd)


def find_dbc_files_root_only(repo_root: Path) -> list[Path]:
    """Return .dbc files only in the repo root (no recursion)."""
    return sorted([p for p in repo_root.glob("*.dbc") if p.is_file()])


def parse_dbc(path: Path) -> DBCDoc:
    doc = DBCDoc()
    lines = path.read_text(errors="ignore").splitlines()

    for ln in lines:
        m = NODE_LINE_RE.match(ln.strip())
        if m:
            nodes = [n for n in m.group(1).split() if n.strip()]
            for n in nodes:
                doc.nodes.add(n)

    i = 0
    while i < len(lines):
        line = lines[i].rstrip("\n")

        m = MSG_START_RE.match(line)
        if m:
            can_id = int(m.group(1))
            block = [line]

            i += 1
            while i < len(lines):
                nxt = lines[i].rstrip("\n")
                if MSG_START_RE.match(nxt):
                    break
                if nxt.startswith(
                    (
                        "CM_",
                        "BA_",
                        "BA_DEF_",
                        "BA_DEF_DEF_",
                        "VAL_",
                        "CAT_",
                        "CAT_DEF_",
                        "FILTER",
                        "EV_DATA_",
                        "ENVVAR_DATA_",
                        "SGTYPE_",
                        "SGTYPE_VAL_",
                        "BA_DEF_SGTYPE_",
                        "BA_SGTYPE_",
                        "SIG_TYPE_REF_",
                        "VAL_TABLE_",
                        "SIG_GROUP_",
                        "SIG_VALTYPE_",
                        "SIGTYPE_VALTYPE_",
                        "BO_TX_BU_",
                        "BA_DEF_REL_",
                        "BA_REL_",
                        "BA_DEF_DEF_REL_",
                        "BU_SG_REL_",
                        "BU_EV_REL_",
                        "BU_BO_REL_",
                        "SG_MUL_VAL_",
                        "NS_",
                        "BS_",
                        "BU_:",
                        "VERSION",
                    )
                ):
                    break
                block.append(nxt)
                i += 1

            while block and block[-1].strip() == "":
                block.pop()

            if can_id not in doc.messages:
                doc.messages[can_id] = block
                doc.message_order.append(can_id)
            else:
                if doc.messages[can_id] != block:
                    print(
                        f"[WARN] {path.name}: conflicting BO_ {can_id}. "
                        f"Keeping first occurrence.",
                        file=sys.stderr,
                    )
            continue

        if line.strip():
            if not line.startswith("BU_:") and not line.startswith("\t"):
                doc.other_lines.add(line)

        i += 1

    return doc


def apply_node_id(doc: DBCDoc, node_id: int, repo_name: str) -> DBCDoc:
    """Return a new DBCDoc with all CAN IDs patched for the given node_id.

    Allocation protocol messages (message_id >= 56) are never patched -
    these are managed by the allocation protocol itself.

    Node names in BU_ and message names are suffixed with the node_id (e.g.
    NODE_NAME -> NODE_NAME_02, message_name -> message_name_02) to uniquely
    identify each node instance in the merged output. Transmitter fields on BO_
    lines are updated to match the suffixed node name.

    Args:
        doc: Parsed DBC document.
        node_id: Node ID to apply (1..30). 0 = no patching.
        repo_name: Used for warning messages on collision.

    Returns:
        New DBCDoc with patched CAN IDs, suffixed node and message names.
    """
    if node_id == NODE_ID_UNASSIGNED:
        return doc  # No patching needed.

    patched = DBCDoc()

    # Patch CAN ID references in non-message lines (VAL_, CM_, BA_, etc.) so
    # they follow their message to its repacked CAN ID.
    patched.other_lines = OrderedSet()
    for ln in doc.other_lines.items:
        for pattern in ID_REF_LINE_RES:
            m = pattern.match(ln)
            if m:
                ref_id = int(m.group(2))
                ref_message_id = (ref_id >> MESSAGE_ID_SHIFT) & 0x3F
                if ref_message_id < 56:  # Skip allocation protocol IDs.
                    ln = f"{m.group(1)}{patch_can_id(ref_id, node_id)}{m.group(3)}"
                break
        patched.other_lines.add(ln)

    # Build suffixed node name mapping: NODE_NAME -> NODE_NAME_02.
    node_suffix = f"_{node_id:02d}"
    patched.nodes = OrderedSet()
    node_name_map: dict[str, str] = {}
    for n in doc.nodes.items:
        if n in SHARED_NODE_ROLES:
            node_name_map[n] = n  # No suffix.
            patched.nodes.add(n)
        else:
            suffixed = f"{n}{node_suffix}"
            node_name_map[n] = suffixed
            patched.nodes.add(suffixed)

    for old_id in doc.message_order:
        block = doc.messages[old_id]

        # Extract message_id from the CAN ID (bits 10..5).
        message_id = (old_id >> MESSAGE_ID_SHIFT) & 0x3F

        # Skip allocation protocol messages (message_id 56..63).
        # These are managed by the allocation protocol, not the node DBC.
        if message_id >= 56:
            patched.messages[old_id] = block
            patched.message_order.append(old_id)
            continue

        new_id = patch_can_id(old_id, node_id)

        # Patch transmitter/receivers.
        # Message name is preserved exactly as defined in the source DBC.
        new_block = []
        for line in block:
            m = MSG_START_RE.match(line)
            if m:
                # Patch BO_ line: update CAN ID, suffix the message name and
                # the transmitter node name.
                msg_name = f"{m.group(2)}{node_suffix}"
                dlc = m.group(3)
                transmitter = m.group(4)
                suffixed_transmitter = node_name_map.get(
                    transmitter, transmitter
                )
                line = f"BO_ {new_id} {msg_name}: {dlc} {suffixed_transmitter}"
            else:
                # Patch SG_ receiver node names: suffix the node name.
                sg_m = SG_LINE_RE.match(line)
                if sg_m:
                    sg_body = sg_m.group(1)
                    receiver = sg_m.group(2)
                    suffixed_receiver = node_name_map.get(receiver, receiver)
                    line = f"{sg_body} {suffixed_receiver}"
            new_block.append(line)

        if new_id in patched.messages:
            print(
                f"[WARN] Patched CAN ID {new_id} collision for node_id="
                f"{node_id} in {repo_name}. Keeping first occurrence.",
                file=sys.stderr,
            )
        else:
            patched.messages[new_id] = new_block
            patched.message_order.append(new_id)

    return patched


def parse_base_header(path: Path) -> tuple[list[str], list[str]]:
    lines = path.read_text(errors="ignore").splitlines()
    header: list[str] = []
    for ln in lines:
        if MSG_START_RE.match(ln):
            break
        header.append(ln.rstrip("\n"))
    return header, []


def merge_docs(base_header: list[str], docs: list[DBCDoc]) -> DBCDoc:
    out = DBCDoc()
    out.header_lines = list(base_header)

    for d in docs:
        for n in d.nodes.items:
            out.nodes.add(n)

    for d in docs:
        for can_id in d.message_order:
            block = d.messages[can_id]
            if can_id not in out.messages:
                out.messages[can_id] = block
                out.message_order.append(can_id)
            else:
                if out.messages[can_id] != block:
                    print(
                        f"[WARN] conflicting BO_ {can_id} across inputs. "
                        f"Keeping first occurrence.",
                        file=sys.stderr,
                    )

    for d in docs:
        for ln in d.other_lines.items:
            if ln.startswith(("VERSION", "NS_", "BS_", "BU_:")):
                continue
            out.other_lines.add(ln)

    return out


def render_merged(doc: DBCDoc) -> str:
    header = list(doc.header_lines)

    merged_bu = "BU_:"
    if doc.nodes.items:
        merged_bu = "BU_: " + " ".join(doc.nodes.items)

    replaced = False
    for i, ln in enumerate(header):
        if ln.startswith("BU_:"):
            header[i] = merged_bu
            replaced = True
            break
    if not replaced:
        header.append("")
        header.append(merged_bu)

    msg_lines: list[str] = []
    for can_id in doc.message_order:
        msg_lines.extend(doc.messages[can_id])
        msg_lines.append("")

    other_lines = list(doc.other_lines.items)
    if other_lines and (not other_lines[0].strip()):
        other_lines = [ln for ln in other_lines if ln.strip()]

    out_lines: list[str] = []
    out_lines.extend(header)
    if out_lines and out_lines[-1].strip() != "":
        out_lines.append("")
    out_lines.extend(msg_lines)

    if other_lines:
        if out_lines and out_lines[-1].strip() != "":
            out_lines.append("")
        out_lines.extend(other_lines)
        if out_lines and out_lines[-1].strip() != "":
            out_lines.append("")

    return "\n".join(out_lines).rstrip() + "\n"


def load_repos_from_file(path: Path) -> list[str]:
    repos: list[str] = []
    for ln in path.read_text(errors="ignore").splitlines():
        ln = ln.strip()
        if not ln or ln.startswith("#"):
            continue
        repos.append(ln)
    return repos


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument(
        "--workdir", default="dbc_merge_work", help="Where to clone repos"
    )
    ap.add_argument(
        "--repos-file",
        help="Text file: one repo URL per line (url[@branch][#node_id])",
    )
    ap.add_argument(
        "--out", default="merged.dbc", help="Output merged DBC path"
    )
    args = ap.parse_args()

    workdir = Path(args.workdir).resolve()
    workdir.mkdir(parents=True, exist_ok=True)

    raw_repos = REPOS
    if args.repos_file:
        raw_repos = load_repos_from_file(Path(args.repos_file).resolve())

    if not raw_repos:
        print(
            "[ERROR] No repos provided. "
            "Edit REPOS in the script or pass --repos-file.",
            file=sys.stderr,
        )
        return 2

    # Parse and validate repo specs.
    specs: list[RepoSpec] = []
    for raw in raw_repos:
        spec = _split_repo_spec(raw)
        if not validate_node_id(spec.node_id, raw):
            return 2
        specs.append(spec)

    # Warn on duplicate node_id assignments (excluding 0).
    seen_node_ids: dict[int, str] = {}
    for spec in specs:
        if spec.node_id != NODE_ID_UNASSIGNED:
            if spec.node_id in seen_node_ids:
                print(
                    f"[WARN] node_id={spec.node_id} assigned to multiple repos: "
                    f"'{seen_node_ids[spec.node_id]}' and '{spec.url}'. "
                    f"CAN ID collisions likely.",
                    file=sys.stderr,
                )
            else:
                seen_node_ids[spec.node_id] = spec.url

    # Clone/update repos.
    # Resolve repos: clone remote URLs, resolve local paths.
    repo_dirs: list[tuple[Optional[Path], RepoSpec]] = []
    for idx, spec in enumerate(specs, start=1):
        if spec.local_path is not None:
            # Local DBC file - no cloning needed.
            path = spec.local_path.resolve()
            if not path.exists():
                print(
                    f"[ERROR] Local DBC not found: {path}",
                    file=sys.stderr,
                )
                return 2
            if not path.is_file() or path.suffix != ".dbc":
                print(
                    f"[ERROR] Local path is not a .dbc file: {path}",
                    file=sys.stderr,
                )
                return 2
            print(
                f"[INFO] Using local DBC: {path}"
                + f" -> node_id={spec.node_id}"
            )
            repo_dirs.append((None, spec))
        else:
            # Remote git repo - clone/update.
            safe = re.sub(
                r"[^A-Za-z0-9._-]+",
                "_",
                spec.url.strip().split("/")[-1].replace(".git", ""),
            )
            dst = workdir / f"{idx:02d}_{safe}_n{spec.node_id:02d}"
            print(
                f"[INFO] Syncing {spec.url}"
                + (f" (branch {spec.branch})" if spec.branch else "")
                + f" -> node_id={spec.node_id}"
            )
            git_clone_or_update(spec.url, spec.branch, dst)
            repo_dirs.append((dst, spec))

    # Find and parse root-level DBCs, applying node ID patches.
    all_dbcs: list[Path] = []
    docs: list[DBCDoc] = []

    for rd, spec in repo_dirs:
        if spec.local_path is not None:
            # Local file - use directly.
            dbc_path = spec.local_path.resolve()
            all_dbcs.append(dbc_path)
            print(
                f"  - {dbc_path}"
                + (f" [node_id={spec.node_id}]" if spec.node_id else "")
            )
            parsed = parse_dbc(dbc_path)
            patched = apply_node_id(parsed, spec.node_id, dbc_path.stem)
            docs.append(patched)
        else:
            # Cloned repo - find root-level DBCs.
            dbcs = find_dbc_files_root_only(rd)
            all_dbcs.extend(dbcs)
            for dbc_path in dbcs:
                print(
                    f"  - {dbc_path}"
                    + (f" [node_id={spec.node_id}]" if spec.node_id else "")
                )
                parsed = parse_dbc(dbc_path)
                patched = apply_node_id(parsed, spec.node_id, dbc_path.stem)
                docs.append(patched)

    if not all_dbcs:
        print(
            "[ERROR] No .dbc files found in any repo root or local path.",
            file=sys.stderr,
        )
        return 2

    # Use first DBC as header template.
    base_header, _ = parse_base_header(all_dbcs[0])

    merged = merge_docs(base_header=base_header, docs=docs)
    out_text = render_merged(merged)

    out_path = Path(args.out).resolve()
    out_path.write_text(out_text)

    print(f"[OKAY] Generated merged DBC -> {out_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
