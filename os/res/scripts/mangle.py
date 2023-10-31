#!/usr/bin/env python3
import sys

if len(sys.argv) <= 1:
    print("Usage: {} [NAMESPACE...] DATA-NAME".format(sys.argv[0]), file=sys.stderr)
    exit(1)

_, *namespaces = sys.argv

def unqualified_name(name):
    return f"{len(name)}{name}"

def nested_name(*names):
    return f"_ZN{''.join(unqualified_name(x) for x in names)}E"

if len(namespaces) == 1:
    print(namespaces[0])
else:
    print(nested_name(*namespaces))
