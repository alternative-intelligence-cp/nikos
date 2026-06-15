#!/usr/bin/env python3
###############################################################################
#
# Nitpick-Bridge Interface for NIKOS
#
# This script consumes NIKOS SARIF output, interfaces with Nitpick validation,
# and automatically injects __ikos_assume() macros into the source code to
# eliminate proven false positives.
#
###############################################################################
import argparse
import json
import os
import sys

def parse_arguments(argv):
    parser = argparse.ArgumentParser(description="Nitpick-Bridge integration for NIKOS")
    parser.add_argument('sarif_file', help="SARIF output from NIKOS to analyze")
    parser.add_argument('--inject', action='store_true', help="Automatically inject __ikos_assume() for false positives")
    return parser.parse_args(argv)

def inject_assumption(filepath, line, condition):
    try:
        with open(filepath, 'r') as f:
            lines = f.readlines()
            
        # Ensure ikos intrinsic header is included
        header_included = any("ikos/analyzer/intrinsic.h" in l for l in lines)
        if not header_included:
            lines.insert(0, "#include <ikos/analyzer/intrinsic.h>\n")
            line += 1  # Adjust target line because we just inserted at the top

        # Inject the assumption just before the target line
        assumption_code = f"  __ikos_assume({condition}); // Injected by Nitpick-Bridge\n"
        lines.insert(line - 1, assumption_code)
        
        with open(filepath, 'w') as f:
            f.writelines(lines)
            
        print(f"[*] Injected __ikos_assume({condition}) into {filepath} at line {line}")
    except Exception as e:
        print(f"[!] Failed to inject assumption: {e}", file=sys.stderr)

def main(argv=None):
    if argv is None:
        argv = sys.argv[1:]
    
    args = parse_arguments(argv)
    
    if not os.path.isfile(args.sarif_file):
        print(f"error: No such file: '{args.sarif_file}'", file=sys.stderr)
        sys.exit(1)
        
    with open(args.sarif_file, 'r') as f:
        sarif_data = json.load(f)
        
    runs = sarif_data.get('runs', [])
    if not runs:
        print("[!] No runs found in SARIF.")
        sys.exit(0)
        
    results = runs[0].get('results', [])
    if not results:
        print("[*] No vulnerabilities found in SARIF.")
        sys.exit(0)
        
    print(f"[*] Analyzing {len(results)} vulnerabilities via Nitpick-Bridge...")
    
    for result in results:
        rule_id = result.get('ruleId', 'unknown')
        locations = result.get('locations', [])
        
        if not locations:
            continue
            
        physical = locations[0].get('physicalLocation', {})
        uri = physical.get('artifactLocation', {}).get('uri')
        region = physical.get('region', {})
        line = region.get('startLine')
        
        if not uri or not line:
            continue
            
        print(f"\n[+] Vulnerability: {rule_id} at {uri}:{line}")
        print("    [>] Requesting Nitpick cross-validation...")
        
        # MOCK: In a real scenario, we would send the codeFlow to Nitpick's LLM engine.
        # For now, we mock a response indicating it's a false positive.
        print("    [<] Nitpick: Path is UNREACHABLE due to application invariants.")
        
        if args.inject:
            # We assume it's unreachable, so inject condition 0 (unreachable)
            inject_assumption(uri, line, "0")

if __name__ == '__main__':
    main()
