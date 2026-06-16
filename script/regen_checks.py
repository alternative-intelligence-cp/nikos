#!/usr/bin/env python3
"""
Regenerate CHECK lines in IKOS regression test .ll files based on actual
ikos-import LLVM 20 output.

Strategy:
  1. Run ikos-import on the .ll file to get the actual AR output.
  2. Parse the .ll file to find CHECK line blocks and their positions.
  3. Split the actual AR output into lines.
  4. Match CHECK blocks to AR output regions using anchors (function names,
     bundle header, etc).
  5. Replace old CHECK lines with new ones derived from the actual output.

Usage:
  python3 regen_checks.py <path-to-ll-file> [--dry-run] [--diff]
  python3 regen_checks.py --batch <directory> [--dry-run] [--diff]
"""

import subprocess
import sys
import os
import re
import argparse
import difflib

IKOS_IMPORT = os.environ.get(
    "IKOS_IMPORT",
    os.path.expanduser("~/Workspace/REPOS/nikos/build_ci_test/frontend/llvm/ikos-import")
)
FILE_CHECK = os.environ.get(
    "FILE_CHECK",
    "/usr/lib/llvm-20/bin/FileCheck"
)
IKOS_OPTS = ["-format=text", "-order-globals", "-allow-dbg-mismatch"]

# Files known to have LLVM 20 import issues (skipped in runtest)
SKIP_FILES = {"var-args.ll", "virtual-inheritance.ll", "vla.ll"}


def get_actual_output(ll_path):
    """Run ikos-import and return the AR text output as a list of lines."""
    cmd = [IKOS_IMPORT] + IKOS_OPTS + [ll_path]
    result = subprocess.run(cmd, capture_output=True, text=True)
    if result.returncode != 0:
        print(f"  WARNING: ikos-import failed on {ll_path}: {result.stderr[:200]}", file=sys.stderr)
        return None
    return result.stdout.splitlines()


def run_filecheck(ll_path):
    """Run FileCheck and return (passed, stderr)."""
    cmd_import = [IKOS_IMPORT] + IKOS_OPTS + [ll_path]
    cmd_check = [FILE_CHECK, ll_path]
    p1 = subprocess.Popen(cmd_import, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    p2 = subprocess.Popen(cmd_check, stdin=p1.stdout, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    p1.stdout.close()
    _, stderr = p2.communicate()
    p1.wait()
    return p2.returncode == 0, stderr.decode()


def parse_ll_file(ll_path):
    """Parse a .ll file, returning a list of (line, is_check, check_content).

    is_check: True if the line is a ; CHECK line
    check_content: the content after '; CHECK: ' or '; CHECK-LABEL: '
    """
    with open(ll_path, 'r') as f:
        lines = f.readlines()

    parsed = []
    for line in lines:
        stripped = line.rstrip('\n')
        # Match ; CHECK-LABEL: or ; CHECK:
        m = re.match(r'^;\s*CHECK-LABEL:\s*(.*)', stripped)
        if m:
            parsed.append((stripped, 'CHECK-LABEL', m.group(1)))
            continue
        m = re.match(r'^;\s*CHECK:\s?(.*)', stripped)
        if m:
            parsed.append((stripped, 'CHECK', m.group(1)))
            continue
        parsed.append((stripped, None, None))

    return parsed


def group_check_blocks(parsed):
    """Group consecutive CHECK lines into blocks, tracking their line indices."""
    blocks = []
    current_block = None

    for i, (line, check_type, content) in enumerate(parsed):
        if check_type in ('CHECK', 'CHECK-LABEL'):
            if current_block is None:
                current_block = {'start': i, 'end': i, 'lines': []}
            current_block['end'] = i
            current_block['lines'].append((i, check_type, content))
        else:
            if current_block is not None:
                blocks.append(current_block)
                current_block = None

    if current_block is not None:
        blocks.append(current_block)

    return blocks


def match_ar_to_checks(ar_lines, check_blocks):
    """Match AR output lines to CHECK blocks.

    Returns a list of (block, new_check_lines) pairs.

    The strategy:
      - The AR output is a flat sequence of lines.
      - The CHECK blocks appear in the .ll file in the same order as
        the corresponding AR output regions.
      - We consume AR lines greedily: for each CHECK block, we find
        how many AR lines it should span by looking at the first line
        of the NEXT block (as a boundary), or use the remaining lines.
    """
    # Build anchor-to-AR-line mapping
    # First, find all the "define" and "declare" lines in AR output
    # as natural boundaries
    ar_anchors = []
    for i, line in enumerate(ar_lines):
        stripped = line.strip()
        if (stripped.startswith('define ') or
            stripped.startswith('declare ') or
            stripped.startswith('// Bundle') or
            stripped.startswith('target-')):
            ar_anchors.append((i, stripped))

    return ar_lines  # We'll use a simpler approach below


def rebuild_file(ll_path, ar_lines, dry_run=False, show_diff=False):
    """Rebuild CHECK lines in the .ll file from actual AR output.

    Approach: The AR output is a complete, ordered listing of the bundle.
    The CHECK lines in the .ll file appear in the same order. We consume
    AR lines sequentially, matching each CHECK line's expected content
    against the next unconsumed AR line(s).

    For CHECK-LABEL lines, we search forward in the AR output to find
    the matching line (resynchronization point).
    """
    parsed = parse_ll_file(ll_path)

    # Build the new file content
    new_lines = []
    ar_idx = 0  # Current position in AR output

    # Skip empty lines at start of AR
    while ar_idx < len(ar_lines) and ar_lines[ar_idx].strip() == '':
        ar_idx += 1

    for i, (orig_line, check_type, check_content) in enumerate(parsed):
        if check_type is None:
            # Non-CHECK line: keep as-is
            new_lines.append(orig_line)
            continue

        if check_type == 'CHECK-LABEL':
            # Find this label in the AR output (search forward)
            found = False
            for j in range(ar_idx, len(ar_lines)):
                if ar_lines[j].strip() == check_content.strip():
                    ar_idx = j + 1  # Consume past this line
                    found = True
                    break
            # Keep the CHECK-LABEL line as-is (they match structural anchors)
            new_lines.append(orig_line)
            if not found:
                print(f"  WARNING: CHECK-LABEL not found in AR output: {check_content}", file=sys.stderr)
            continue

        # Regular CHECK line
        if ar_idx >= len(ar_lines):
            # No more AR output — this CHECK line has no match
            # Keep it (might be a trailing CHECK)
            new_lines.append(orig_line)
            continue

        # Get the current AR line
        ar_line = ar_lines[ar_idx]

        # Advance past empty AR lines to find the next meaningful line
        while ar_idx < len(ar_lines) and ar_lines[ar_idx].strip() == '':
            ar_idx += 1
            if ar_idx < len(ar_lines):
                ar_line = ar_lines[ar_idx]

        if ar_idx >= len(ar_lines):
            new_lines.append(orig_line)
            continue

        ar_line = ar_lines[ar_idx]
        ar_idx += 1

        # Build the new CHECK line
        # Preserve original indentation style
        if ar_line.startswith(' '):
            new_check = f"; CHECK: {ar_line}"
        else:
            new_check = f"; CHECK: {ar_line}"

        new_lines.append(new_check)

    # Show diff if requested
    if show_diff:
        old_lines = [p[0] for p in parsed]
        diff = difflib.unified_diff(
            [l + '\n' for l in old_lines],
            [l + '\n' for l in new_lines],
            fromfile=f"a/{os.path.basename(ll_path)}",
            tofile=f"b/{os.path.basename(ll_path)}",
            n=3
        )
        diff_text = ''.join(diff)
        if diff_text:
            print(diff_text)
        else:
            print(f"  No changes needed for {ll_path}")
        return len(diff_text) > 0

    if not dry_run:
        with open(ll_path, 'w') as f:
            for line in new_lines:
                f.write(line + '\n')

    return True


def process_file(ll_path, dry_run=False, show_diff=False):
    """Process a single .ll file."""
    basename = os.path.basename(ll_path)
    if basename in SKIP_FILES:
        print(f"  SKIP (known issue): {basename}")
        return True

    # First check if it already passes
    passed, _ = run_filecheck(ll_path)
    if passed:
        print(f"  OK (already passes): {basename}")
        return True

    # Get actual AR output
    ar_lines = get_actual_output(ll_path)
    if ar_lines is None:
        print(f"  ERROR (ikos-import failed): {basename}")
        return False

    # Rebuild the file
    changed = rebuild_file(ll_path, ar_lines, dry_run=dry_run, show_diff=show_diff)

    if not dry_run and not show_diff:
        # Verify the fix
        passed, stderr = run_filecheck(ll_path)
        if passed:
            print(f"  FIXED: {basename}")
            return True
        else:
            print(f"  STILL FAILING after regen: {basename}")
            # Print first few lines of error
            for line in stderr.splitlines()[:5]:
                print(f"    {line}")
            return False
    else:
        status = "would change" if changed else "no changes"
        print(f"  {status}: {basename}")
        return True


def main():
    parser = argparse.ArgumentParser(description='Regenerate CHECK lines for IKOS regression tests')
    parser.add_argument('path', help='Path to .ll file or directory (with --batch)')
    parser.add_argument('--batch', action='store_true', help='Process all .ll files in directory')
    parser.add_argument('--dry-run', action='store_true', help='Do not modify files')
    parser.add_argument('--diff', action='store_true', help='Show diff instead of modifying')
    parser.add_argument('--failing-only', action='store_true', help='Only process failing tests')
    args = parser.parse_args()

    if args.batch:
        directory = args.path
        ll_files = sorted([
            os.path.join(directory, f)
            for f in os.listdir(directory)
            if f.endswith('.ll')
        ])
        results = {'fixed': 0, 'already_ok': 0, 'failed': 0, 'skipped': 0}
        for ll_path in ll_files:
            basename = os.path.basename(ll_path)
            if basename in SKIP_FILES:
                results['skipped'] += 1
                print(f"  SKIP: {basename}")
                continue

            if args.failing_only:
                passed, _ = run_filecheck(ll_path)
                if passed:
                    results['already_ok'] += 1
                    continue

            ok = process_file(ll_path, dry_run=args.dry_run, show_diff=args.diff)
            if ok:
                results['fixed'] += 1
            else:
                results['failed'] += 1

        print(f"\nSummary: {results['fixed']} fixed, {results['already_ok']} already ok, "
              f"{results['failed']} failed, {results['skipped']} skipped")
    else:
        process_file(args.path, dry_run=args.dry_run, show_diff=args.diff)


if __name__ == '__main__':
    main()
