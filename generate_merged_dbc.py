"""Merged DBC generator.

Clone multiple git repos (each containing one or more .dbc files at the root
directory) and merge them into a single merged .dbc file.

Example:
    ```shell
    # Unix.
    python3 generate_merged_dbc.py --repos-file repos.txt --out project.dbc --workdir workspace
    ```
    Creates a merged DBC named "project.dbc" using repo URLs in "repos.txt"
    (one repo URL per line). DBC merge work done in "workspace" directory.
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
    # "https://github.com/your_org/repo2.git@main"
]

MSG_START_RE = re.compile(r"^BO_\s+(\d+)\s+")
NODE_LINE_RE = re.compile(r"^BU_:\s*(.*)$")


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


def _split_repo_branch(spec: str) -> tuple[str, Optional[str]]:
    if "@" in spec:
        url, branch = spec.rsplit("@", 1)
        branch = branch.strip() or None
        return url.strip(), branch
    return spec.strip(), None


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
                        f"[WARN] {path.name}: conflicting BO_ {can_id}. Keeping first occurrence.",
                        file=sys.stderr,
                    )
            continue

        if line.strip():
            if not line.startswith("BU_:") and not line.startswith("\t"):
                doc.other_lines.add(line)

        i += 1

    return doc


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
                        f"[WARN] conflicting BO_ {can_id} across inputs. Keeping first occurrence.",
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


def pick_root_dbc(repo_dirs: list[Path]) -> Optional[Path]:
    for rd in repo_dirs:
        dbcs = find_dbc_files_root_only(rd)
        if dbcs:
            return dbcs[0]
    return None


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument(
        "--workdir", default="dbc_merge_work", help="Where to clone repos"
    )
    ap.add_argument(
        "--repos-file",
        help="Text file: one repo URL per line (optionally url@branch)",
    )
    ap.add_argument(
        "--out", default="merged.dbc", help="Output merged DBC path"
    )
    args = ap.parse_args()

    workdir = Path(args.workdir).resolve()
    workdir.mkdir(parents=True, exist_ok=True)

    repos = REPOS
    if args.repos_file:
        repos = load_repos_from_file(Path(args.repos_file).resolve())

    if not repos:
        print(
            "[ERROR] No repos provided. "
            "Edit REPOS in the script or pass --repos-file.",
            file=sys.stderr,
        )
        return 2

    # Clone/update repos.
    repo_dirs: list[Path] = []
    for idx, spec in enumerate(repos, start=1):
        url, branch = _split_repo_branch(spec)
        safe = re.sub(
            r"[^A-Za-z0-9._-]+",
            "_",
            url.strip().split("/")[-1].replace(".git", ""),
        )
        dst = workdir / f"{idx:02d}_{safe}"
        print(
            f"[INFO] Syncing {url}" + (f" (branch {branch})" if branch else "")
        )
        git_clone_or_update(url, branch, dst)
        repo_dirs.append(dst)

    # Find root-level DBCs.
    all_dbcs: list[Path] = []
    for rd in repo_dirs:
        all_dbcs.extend(find_dbc_files_root_only(rd))

    if not all_dbcs:
        print(
            "[ERROR] No root-level .dbc files found in repos.", file=sys.stderr
        )
        return 2

    print(f"[INFO] Found {len(all_dbcs)} .dbc file(s)")
    for p in all_dbcs:
        print(f"  - {p}")

    # Use first DBC as header template.
    base_header, _ = parse_base_header(all_dbcs[0])

    docs: list[DBCDoc] = [parse_dbc(p) for p in all_dbcs]
    merged = merge_docs(base_header=base_header, docs=docs)
    out_text = render_merged(merged)

    out_path = Path(args.out).resolve()
    out_path.write_text(out_text)

    print(f"[OKAY] Generated merged DBC -> {out_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
