"""Test registration, context and outcomes for linux/tests/run.py."""
import contextlib
import time
import traceback
from dataclasses import dataclass, field
from pathlib import Path

from .player import ROOT, DEFAULT_BINARY

FIXTURES = ROOT / 'local/tests/fixtures'
REGISTRY = []


class Skip(Exception):
    """The test cannot run here (missing fixture, tool or hardware)."""


class Failure(AssertionError):
    pass


@dataclass
class Test:
    name: str
    tier: str           # 'static', 'quick' or 'full'
    feature: str        # The linux/PORT_NOTES.md feature it covers.
    trigger: str        # For a fix: what originally showed the bug, which the test reproduces.
    function: object
    module: str = ''


def test(tier, feature, trigger='', parts=()):
    """parts: (name, feature) of each ctx.part() the test runs, for --list."""
    def register(function):
        function.parts = parts
        REGISTRY.append(Test(function.__name__, tier, feature, trigger, function, function.__module__.split('.')[-1]))
        return function
    return register


@dataclass
class Context:
    run_dir: Path
    binary: Path = DEFAULT_BINARY
    notes: list = field(default_factory=list)
    parts: list = field(default_factory=list)

    def fixture(self, name):
        path = FIXTURES / name
        if not path.exists():
            raise Skip('fixture missing: %s (see linux/tests/README.md)' % path.relative_to(ROOT))
        return path

    def note(self, text):
        """A measurement worth keeping in the summary even when the test passes."""
        self.notes.append(text)

    @contextlib.contextmanager
    def part(self, name, feature, trigger=''):
        """One feature checked inside a shared session; a failure is recorded and the session goes on."""
        record = dict(name=name, feature=feature, trigger=trigger, outcome='PASS', detail='', notes=[])
        self.parts.append(record)
        notes = self.notes
        self.notes = record['notes']
        try:
            yield
        except Skip as error:
            record.update(outcome='SKIP', detail=str(error))
        except Failure as error:
            record.update(outcome='FAIL', detail=str(error))
        except Exception as error:
            if type(error).__name__ == 'RunError':
                record.update(outcome='FAIL', detail=str(error))
            else:
                record.update(outcome='ERROR', detail=traceback.format_exc())
        finally:
            self.notes = notes


def check(condition, message):
    if not condition:
        raise Failure(message)


def wait_until(predicate, timeout, interval=0.1, message='condition'):
    deadline = time.time() + timeout
    while True:
        value = predicate()
        if value:
            return value
        if time.time() > deadline:
            raise Failure('%s not met within %ss' % (message, timeout))
        time.sleep(interval)
