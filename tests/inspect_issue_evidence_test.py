"""F4 inspector schema/domain/cap contract on synthetic rows only; no player report is read.

    python -I tests/inspect_issue_evidence_test.py <repo>/tools/baseline [synthetic-saturated.jsonl]
The optional second argument is the fixture artifact baseline_issue_evidence_test writes
(<build>/baseline/issue-evidence-test/synthetic-saturated.jsonl), never a profile report.
"""
import importlib
import json
import sys
import tempfile
from pathlib import Path

SIX = ("frame", "draw", "host", "graphics", "publication", "audio")
NINE = ("frame", "draw", "bk_draw", "host", "graphics", "publication", "audio",
        "player_transform", "player_projection")
FRAME_BITS = {1: {"layout": 1, "frame": 2}, 2: {"layout": 1, "frame": 2, "cpu625": 4},
              3: {"layout": 1, "frame": 2, "cpu625": 4, "legacy_player": 8, "secondary_route": 16}}
DRAW_BITS = {"layout": 1, "frame": 2, "root": 4, "projection": 8, "context": 16,
             "player_join": 32, "player_slot": 64, "skeletal_join": 128}
LEGACY = ("player_owner", "player_model", "player_asset", "player_admissions", "player_refusals")
PRIMARY = {"view": 2, "ledger_bytes": 7584, "ledger_highwater": 11376}
SECONDARY = {"opens": 3, "faults": 1, "seen_main": 2, "seen_late": 1, "unauthenticated": 4,
             "admissions": 2, "ledger_bytes": 5648}
REMOVE = object()


def frame(schema, valid, **override):
    """One frame row shaped as the serializer writes it for `schema`; override replaces fields."""
    legacy = bool(valid & 8) if schema == 3 else True
    row = {"type": "frame", "sequence": 1, "time_us": 100, "valid": valid}
    row.update({name: 0x345 if legacy else None for name in LEGACY})
    if schema >= 2:
        row.update(player_last_reason=0xFFFFFFFF if legacy and valid & 1 else None,
                   bridge_next_block=29 if valid & 1 else None,
                   cpu625_next_block=2047 if valid & 4 else None)
        if legacy and valid & 1:
            row["player_primary"] = dict(PRIMARY)
    if (schema == 2 and valid & 1) or (schema == 3 and valid & 16):
        row["secondary_route"] = dict(SECONDARY)
    row.update(override)
    return {key: value for key, value in row.items() if value is not REMOVE}


def report(path, schema, domains, frames, frame_bits=None, pad=0):
    rows = [{"type": "report", "schema": schema, "marker": 17}]
    coverage = {"type": "coverage", "unavailable": ["collision"], "limits": ["empty_domain_is_no_observation"]}
    if schema >= 2:
        coverage["frame_valid_bits"] = FRAME_BITS[schema] if frame_bits is None else frame_bits
        coverage["draw_valid_bits"] = DRAW_BITS
    rows.append(coverage)
    for name in domains:
        events = [dict(row, sequence=i + 1) for i, row in enumerate(frames)] if name == "frame" else []
        if name == "host" and pad:
            events = [{"type": "host", "sequence": 1, "time_us": 100, "pad": "a" * pad}]
        times = [event["time_us"] for event in events]
        rows.append({"type": "domain", "domain": name, "window_us": 10000000, "end_us": 1000,
                     "first_us": times[0] if times else 0, "last_us": times[-1] if times else 0,
                     "retained": len(events), "retained_span_us": times[-1] - times[0] if times else 0})
        rows.extend(events)
    rows.append({"type": "completion", "complete": True, "error": ""})
    path.write_text("".join(json.dumps(row) + "\n" for row in rows), encoding="utf-8")
    return path


def main(tools, saturated=None):
    sys.path.insert(0, tools)
    check = importlib.import_module("inspect_issue_evidence").inspect

    def rejected(path, why):
        try:
            check(path)
        except ValueError:
            return
        raise AssertionError(why)

    with tempfile.TemporaryDirectory() as directory:
        names = iter(range(1000))
        def new():
            return Path(directory) / ("report-%d.jsonl" % next(names))
        # Schema 1/2 six-domain reports keep validating under their unchanged rules.
        check(report(new(), 1, SIX, [frame(1, 3)]))
        check(report(new(), 2, SIX, [frame(2, 7), frame(2, 3), frame(2, 2)]))
        # The current serializer's nine domains under schema 2 (the stale-inspector gap).
        check(report(new(), 2, NINE, [frame(2, 7)]))
        rejected(report(new(), 1, NINE, [frame(1, 3)]), "schema 1 predates the extended domains")
        rejected(report(new(), 2, SIX[:-1], [frame(2, 3)]), "missing domain")
        rejected(report(new(), 2, NINE[:-1], [frame(2, 3)]), "partial extended domain set")
        rejected(report(new(), 2, NINE, [frame(2, 15)]), "schema 2 keeps its 0..7 frame bits")
        # Schema 3: legacy player words and groups exist exactly with their bits.
        summary = check(report(new(), 3, NINE, [frame(3, 31), frame(3, 15), frame(3, 7), frame(3, 2)]))
        assert summary["schema"] == 3 and set(summary["domains"]) == set(NINE)
        rejected(report(new(), 3, SIX, [frame(3, 7)]), "schema 3 needs the current domains")
        rejected(report(new(), 3, NINE, [frame(3, 7, player_owner=0)]), "a disabled owner's zero is not evidence")
        rejected(report(new(), 3, NINE, [frame(3, 7, player_last_reason=0)]), "unavailable reason must be null")
        rejected(report(new(), 3, NINE, [frame(3, 31, player_owner=None)]), "an available word must not be null")
        rejected(report(new(), 3, NINE, [frame(3, 7, player_primary=dict(PRIMARY))]), "primary group without its bit")
        rejected(report(new(), 3, NINE, [frame(3, 15, secondary_route=dict(SECONDARY))]),
                 "secondary group without its bit")
        rejected(report(new(), 3, NINE, [frame(3, 15, player_primary=REMOVE)]), "primary group missing with its bit")
        rejected(report(new(), 3, NINE, [frame(3, 31, secondary_route={"opens": 1})]), "incomplete secondary group")
        rejected(report(new(), 3, NINE, [frame(3, 23)]), "secondary_route requires legacy_player")
        rejected(report(new(), 3, NINE, [frame(3, 10)]), "legacy_player requires layout")
        rejected(report(new(), 3, NINE, [frame(3, 39)]), "unknown frame bit")
        rejected(report(new(), 3, NINE, [frame(3, 7)], frame_bits=FRAME_BITS[2]), "schema-3 bit map required")
        rejected(report(new(), 4, NINE, [frame(3, 7)], frame_bits=FRAME_BITS[3]), "unknown schema")
        # Byte caps: six-domain reports keep 8 MiB; the extended set may use the serializer's 64 MiB.
        big = 8 * 1024 * 1024
        rejected(report(new(), 2, SIX, [frame(2, 3)], pad=big), "six-domain report keeps the 8 MiB cap")
        check(report(new(), 2, NINE, [frame(2, 3)], pad=big))
        check(report(new(), 3, NINE, [frame(3, 7)], pad=big))
    if saturated:
        summary = check(Path(saturated))
        assert summary["schema"] == 3 and set(summary["domains"]) == set(NINE), "saturated fixture is schema 3"
    print("inspect_issue_evidence_test: ok" + (" (saturated fixture inspected)" if saturated else ""))


if __name__ == "__main__":
    main(sys.argv[1], sys.argv[2] if len(sys.argv) > 2 else None)
