"""Validate an F4 schema-1, -2 or -3 report and summarize coverage; no gameplay verdict."""
import argparse
import json
from pathlib import Path

DOMAINS = {"frame", "draw", "host", "graphics", "publication", "audio"}
MAX_BYTES = 8 * 1024 * 1024
# The current serializer adds BK root and RT64 player domains under its raised cap (issue_evidence.hpp).
EXTENDED_DOMAINS = DOMAINS | {"bk_draw", "player_transform", "player_projection"}
EXTENDED_MAX_BYTES = 64 * 1024 * 1024
SCHEMAS = (1, 2, 3)
# Six-domain reports keep their 8 MiB cap; schema 3 is only written with the extended set.
SCHEMA_DOMAINS = {1: (DOMAINS,), 2: (DOMAINS, EXTENDED_DOMAINS), 3: (EXTENDED_DOMAINS,)}
# Schema 2 frame rows add these; each is null unless its validity bit is set.
FRAME_FIELDS_2 = {"player_last_reason": 1, "bridge_next_block": 1, "cpu625_next_block": 4}
# Schema 3 gates every legacy player word on legacy_player instead of layout.
LEGACY_PLAYER_FIELDS = ("player_owner", "player_model", "player_asset", "player_admissions", "player_refusals")
FRAME_FIELDS_3 = {"player_last_reason": 8, "bridge_next_block": 1, "cpu625_next_block": 4,
                  **dict.fromkeys(LEGACY_PLAYER_FIELDS, 8)}
# Schema 3 frame groups appear exactly when their bit is set.
FRAME_GROUPS_3 = {"player_primary": (8, ("view", "ledger_bytes", "ledger_highwater")),
                  "secondary_route": (16, ("opens", "faults", "seen_main", "seen_late", "unauthenticated",
                                           "admissions", "ledger_bytes"))}
FRAME_VALID_BITS = {1: {"layout": 1, "frame": 2}, 2: {"layout": 1, "frame": 2, "cpu625": 4},
                    3: {"layout": 1, "frame": 2, "cpu625": 4, "legacy_player": 8, "secondary_route": 16}}
DRAW_BITS_1 = {"layout": 1, "frame": 2, "root": 4, "projection": 8, "context": 16}
DRAW_VALID_BITS = {1: DRAW_BITS_1, 2: {**DRAW_BITS_1, "player_join": 32, "player_slot": 64, "skeletal_join": 128}}
DRAW_VALID_BITS[3] = DRAW_VALID_BITS[2]  # Schema 3 changes frame rows only.
# Schema 2 draw join groups appear exactly when their bit is set.
PLAYER_FIELDS = ("player_owner", "player_owner_slot", "player_table_match", "player_control_slot", "player_outcome")
SKELETAL_FIELDS = ("skeletal_calls", "skeletal_admissions", "skeletal_refusals", "skeletal_reason")


def is_word(value):
    return type(value) is int and 0 <= value <= 0xFFFFFFFF


def draw_fields(row, schema):
    """Exact schema-2 join groups; unjoined draws carry neither group."""
    if schema == 1:
        if any(name in row for name in PLAYER_FIELDS + SKELETAL_FIELDS):
            raise ValueError("schema-1 draw row carries schema-2 join fields")
        return
    valid = row.get("valid")
    if type(valid) is not int or not 0 <= valid <= 255 or (valid & 0xA0 and not valid & 1) or \
            (valid & 64 and not valid & 32):
        raise ValueError("invalid schema-2 draw validity bits")
    for fields, bit in ((PLAYER_FIELDS, 32), (SKELETAL_FIELDS, 128)):
        if any((name in row) != bool(valid & bit) for name in fields):
            raise ValueError("draw join fields must appear exactly with their bit")
    if valid & 32:
        if not all(is_word(row[name]) for name in ("player_owner", "player_control_slot", "player_outcome")):
            raise ValueError("invalid player join word")
        slot, match = row["player_owner_slot"], row["player_table_match"]
        if (valid & 64 and not (is_word(slot) and match in (0, 1) and type(match) is int)) or \
                (not valid & 64 and (slot is not None or match is not None)):
            raise ValueError("player slot fields must match the player_slot bit")
    if valid & 128:
        calls, admissions, refusals = (row[name] for name in SKELETAL_FIELDS[:3])
        if calls != 1 or (admissions, refusals) not in ((1, 0), (0, 1)) or \
                any(type(value) is not int for value in (calls, admissions, refusals)):
            raise ValueError("skeletal join requires one counted admission or refusal")
        if (is_word(row["skeletal_reason"]) if refusals else row["skeletal_reason"] is None) is not True:
            raise ValueError("skeletal reason must exist only for a refusal")


def frame_fields(row, schema):
    """Schema 1 rows predate the fields; schema 2 rows must carry all of them; schema 3 also gates
    the legacy player words and groups on their availability bits."""
    if schema == 1:
        if any(name in row for name in FRAME_FIELDS_2):
            raise ValueError("schema-1 frame row carries schema-2 fields")
        return
    valid = row.get("valid")
    if schema == 2:
        if type(valid) is not int or not 0 <= valid <= 7 or (valid & 4 and not valid & 1):
            raise ValueError("invalid schema-2 frame validity bits")
        fields = FRAME_FIELDS_2
    else:
        if type(valid) is not int or not 0 <= valid <= 31 or (valid & 12 and not valid & 1) or \
                (valid & 16 and not valid & 8):
            raise ValueError("invalid schema-3 frame validity bits")
        fields = FRAME_FIELDS_3
        for name, (bit, members) in FRAME_GROUPS_3.items():
            if (name in row) != bool(valid & bit):
                raise ValueError("frame groups must appear exactly with their bit")
            group = row.get(name)
            if name in row and (not isinstance(group, dict) or set(group) != set(members) or
                                not all(is_word(group[key]) for key in members)):
                raise ValueError("invalid frame group words")
    for name, bit in fields.items():
        if name not in row:
            raise ValueError("missing schema-%d frame field" % schema)
        value = row[name]
        if not valid & bit:
            if value is not None:
                raise ValueError("unauthenticated frame field must be null")
        elif type(value) is not int or not 0 <= value <= 0xFFFFFFFF:
            raise ValueError("invalid authenticated frame word")


def inspect(path):
    with path.open("rb") as source:
        raw = source.read(EXTENDED_MAX_BYTES + 1)
    if len(raw) > EXTENDED_MAX_BYTES:
        raise ValueError("report exceeds byte cap")
    rows = [json.loads(line) for line in raw.splitlines()]
    if not rows or any(not isinstance(row, dict) for row in rows):
        raise ValueError("report must contain JSON objects")
    schema = rows[0].get("schema")
    if rows[0].get("type") != "report" or type(schema) is not int or schema not in SCHEMAS:
        raise ValueError("unsupported or missing report header")
    if rows[-1].get("type") != "completion" or rows[-1].get("complete") is not True:
        raise ValueError("incomplete report; do not treat missing events as clean evidence")
    domains, events, coverage = {}, {}, None
    for row in rows[1:-1]:
        kind = row.get("type")
        if kind == "coverage":
            if coverage is not None:
                raise ValueError("duplicate coverage row")
            coverage = row
        elif kind == "domain":
            name = row.get("domain")
            if name not in EXTENDED_DOMAINS or name in domains:
                raise ValueError("unknown or duplicate domain")
            domains[name], events[name] = row, []
        elif kind in EXTENDED_DOMAINS and kind in domains:
            sequence, timestamp = row.get("sequence"), row.get("time_us")
            if type(sequence) is not int or sequence < 1 or type(timestamp) is not int or timestamp < 0:
                raise ValueError("invalid event sequence/time")
            if events[kind] and (sequence <= events[kind][-1][0] or timestamp < events[kind][-1][1]):
                raise ValueError("domain events are out of order")
            if kind == "draw":
                for name in ("root", "projection"):
                    words = row.get(name)
                    if not isinstance(words, list) or len(words) != 16 or any(
                        type(word) is not int or not 0 <= word <= 0xFFFFFFFF for word in words
                    ):
                        raise ValueError("invalid fixed-matrix words")
                draw_fields(row, schema)
            elif kind == "frame":
                frame_fields(row, schema)
            events[kind].append((sequence, timestamp))
        else:
            raise ValueError("unknown or misplaced row")
    if set(domains) not in SCHEMA_DOMAINS[schema] or coverage is None:
        raise ValueError("missing domain or coverage declaration")
    if set(domains) == DOMAINS and len(raw) > MAX_BYTES:
        raise ValueError("report exceeds byte cap")
    for key in ("unavailable", "limits"):
        values = coverage.get(key)
        if not isinstance(values, list) or not values or any(not isinstance(value, str) or not value for value in values):
            raise ValueError("missing explicit coverage limits")
    bits = coverage.get("frame_valid_bits")
    if (schema >= 2 or bits is not None) and bits != FRAME_VALID_BITS[schema]:
        raise ValueError("frame validity bits differ from the report schema")
    bits = coverage.get("draw_valid_bits")
    if (schema >= 2 or bits is not None) and bits != DRAW_VALID_BITS[schema]:
        raise ValueError("draw validity bits differ from the report schema")
    for name, domain in domains.items():
        if len(events[name]) != domain.get("retained"):
            raise ValueError("retained event count mismatch")
        end, window = domain.get("end_us"), domain.get("window_us")
        if type(end) is not int or end < 0 or type(window) is not int or window != 10000000 or any(
            not end - window <= timestamp <= end for _, timestamp in events[name]
        ):
            raise ValueError("invalid domain time window")
        first = events[name][0][1] if events[name] else 0
        last = events[name][-1][1] if events[name] else 0
        if (domain.get("first_us"), domain.get("last_us"), domain.get("retained_span_us")) != (first, last, last - first):
            raise ValueError("coverage time boundaries differ from retained events")
    return {"schema": schema, "marker": rows[0].get("marker"), "bytes": len(raw),
            "domains": domains, "coverage": coverage,
            "meaning": "serialization valid; empty, invalid or truncated domains remain evidence gaps"}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("report", type=Path)
    arguments = parser.parse_args()
    try:
        print(json.dumps(inspect(arguments.report), indent=2))
    except (OSError, ValueError, TypeError) as error:
        parser.exit(1, f"Invalid F4 report: {error}\n")
