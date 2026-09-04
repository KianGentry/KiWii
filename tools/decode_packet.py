#!/usr/bin/env python3
"""Small placeholder for packet-capture decoding experiments."""

import argparse
from pathlib import Path


def main() -> None:
    parser = argparse.ArgumentParser(description="Inspect a captured packet fixture")
    parser.add_argument("capture", type=Path)
    args = parser.parse_args()
    print(f"capture decoding is not implemented yet: {args.capture}")


if __name__ == "__main__":
    main()
