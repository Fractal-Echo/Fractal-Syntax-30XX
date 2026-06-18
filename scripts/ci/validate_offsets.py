#!/usr/bin/env python3
from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path


REQUIRED = {
    "BASE": {"LocalPlayer", "EntityList"},
    "POINTER": {
        "Health",
        "Energy",
        "XCoord",
        "ZCoord",
        "MWeapon",
        "QWeapon",
        "WWeapon",
        "EWeapon",
        "PhysicalDamage",
        "AbilityDamage",
        "Speed",
        "JumpHeight",
    },
    "STATIC": {
        "SoulChips",
        "Memoria",
        "Bolts",
        "TitanShards",
        "Color",
        "IonCube",
        "BlueCube",
    },
    "PATCH": {
        "EnergyPatch",
        "JumpPatch",
        "InstantKill",
        "ScrapCmp",
        "Scrap",
        "RainbowPatch",
    },
}

HEX_RE = re.compile(r"^0x[0-9a-fA-F]+$")
BYTE_RE = re.compile(r"^[0-9a-fA-F]{2}$")


def fail(message: str) -> None:
    print(f"error: {message}", file=sys.stderr)
    raise SystemExit(1)


def parse_ini_like(path: Path) -> dict[str, dict[str, str]]:
    sections: dict[str, dict[str, str]] = {}
    current: str | None = None

    for line_number, raw in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        line = raw.strip()
        if not line or line.startswith(("#", ";")):
            continue

        if line.startswith("[") and line.endswith("]"):
            current = line[1:-1].strip()
            if not current:
                fail(f"{path}:{line_number}: empty section name")
            sections.setdefault(current, {})
            continue

        if current is None:
            fail(f"{path}:{line_number}: key/value before any section")

        if "=" not in line:
            fail(f"{path}:{line_number}: expected key=value")

        key, value = (part.strip() for part in line.split("=", 1))
        if not key:
            fail(f"{path}:{line_number}: empty key")
        if key in sections[current]:
            fail(f"{path}:{line_number}: duplicate key {current}.{key}")
        sections[current][key] = value

    return sections


def validate_hex_offset(value: str, label: str) -> None:
    if not HEX_RE.match(value):
        fail(f"{label}: expected hexadecimal offset like 0x1234, got {value!r}")


def validate_bytes(value: str, label: str) -> int:
    parts = value.split()
    if not parts:
        fail(f"{label}: byte list is empty")
    for part in parts:
        if not BYTE_RE.match(part):
            fail(f"{label}: invalid byte {part!r}; expected two hex characters")
    return len(parts)


def validate(path: Path) -> None:
    sections = parse_ini_like(path)

    for section, keys in REQUIRED.items():
        if section not in sections:
            fail(f"missing [{section}] section")
        missing = sorted(keys - sections[section].keys())
        if missing:
            fail(f"[{section}] missing required keys: {', '.join(missing)}")

    for key, value in sections["BASE"].items():
        validate_hex_offset(value, f"BASE.{key}")

    for key, value in sections["STATIC"].items():
        validate_hex_offset(value, f"STATIC.{key}")

    for key, value in sections["POINTER"].items():
        if ":" not in value:
            fail(f"POINTER.{key}: expected BaseName:offset[,offset...]")
        base_name, offsets = (part.strip() for part in value.split(":", 1))
        if base_name not in sections["BASE"]:
            fail(f"POINTER.{key}: unknown base {base_name!r}")
        offset_parts = [part.strip() for part in offsets.split(",") if part.strip()]
        if not offset_parts:
            fail(f"POINTER.{key}: no pointer offsets")
        for index, offset in enumerate(offset_parts):
            validate_hex_offset(offset, f"POINTER.{key}[{index}]")

    for key, value in sections["PATCH"].items():
        parts = [part.strip() for part in value.split("|")]
        if len(parts) != 3:
            fail(f"PATCH.{key}: expected address|enabled bytes|disabled bytes")
        address, enabled, disabled = parts
        validate_hex_offset(address, f"PATCH.{key}.address")
        enabled_len = validate_bytes(enabled, f"PATCH.{key}.enabled")
        disabled_len = validate_bytes(disabled, f"PATCH.{key}.disabled")
        if enabled_len != disabled_len:
            fail(
                f"PATCH.{key}: enabled byte count {enabled_len} does not match "
                f"disabled byte count {disabled_len}"
            )


def main() -> int:
    parser = argparse.ArgumentParser(description="Validate Fractal Syntax offset config.")
    parser.add_argument("path", nargs="?", default="Main.txt", help="offset file path")
    args = parser.parse_args()

    path = Path(args.path)
    if not path.is_file():
        fail(f"offset file not found: {path}")

    validate(path)
    print(f"validated {path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
