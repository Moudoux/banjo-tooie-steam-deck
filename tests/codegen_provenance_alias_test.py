"""write_receipt keeps logical names for junctioned required inputs; validate stays exact."""
import os
from pathlib import Path
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from codegen_provenance import validate, write_receipt  # noqa: E402

CLANG = 'deps/LLVM/bin/clang.exe'
REQUIRED = ('tools/a.py', CLANG)


def link_directory(link, target):
    if os.name == 'nt':
        import _winapi
        _winapi.CreateJunction(str(target), str(link))
    else:
        os.symlink(target, link, target_is_directory=True)


class AliasedRequiredInputTest(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        base = Path(self.temporary.name).resolve()
        self.root, shared = base / 'repo', base / 'shared' / 'deps'
        (shared / 'LLVM' / 'bin').mkdir(parents=True)
        self.clang = shared / 'LLVM' / 'bin' / 'clang.exe'
        self.clang.write_bytes(b'clang')
        self.header = shared / 'outside.h'
        self.header.write_text('#pragma once\n')
        (self.root / 'tools').mkdir(parents=True)
        (self.root / 'tools' / 'a.py').write_text('pass\n')
        (self.root / 'generated').mkdir()
        (self.root / 'generated' / 'render_layout.h').write_text('#pragma once\n')
        self.links = [self.root / 'deps']
        link_directory(self.links[0], shared)

    def tearDown(self):
        for link in self.links:
            os.rmdir(link) if os.name == 'nt' else os.unlink(link)
        self.temporary.cleanup()

    def write(self, required=REQUIRED):
        inputs = [self.root / name for name in REQUIRED] + [self.header]
        return write_receipt(self.root, inputs, {}, required_inputs=required)

    def test_ordinary_and_aliased_inputs_validate(self):
        receipt = self.write()
        self.assertIn('tools/a.py', receipt['inputs'])
        self.assertIn(CLANG, receipt['inputs'])
        self.assertIn(str(self.header), receipt['inputs'])  # non-required outside input stays absolute
        self.assertFalse(any(name.startswith(str(self.clang.parent)) for name in receipt['inputs']))
        validate(self.root, required_inputs=REQUIRED, replay=True)

    def test_changed_aliased_content_is_rejected(self):
        self.write()
        self.clang.write_bytes(b'different')
        with self.assertRaisesRegex(ValueError, 'Codegen input changed: deps/LLVM/bin/clang.exe'):
            validate(self.root, required_inputs=REQUIRED, replay=True)

    def test_missing_required_input_still_fails(self):
        self.write()
        with self.assertRaisesRegex(ValueError, 'missing required input'):
            validate(self.root, required_inputs=REQUIRED + ('deps/LLVM/bin/ld.lld.exe',), replay=True)

    def test_two_required_names_for_one_target_are_rejected(self):
        self.links.append(self.root / 'deps2')
        link_directory(self.links[1], self.clang.parents[2])
        with self.assertRaisesRegex(ValueError, 'Ambiguous required input alias'):
            self.write(REQUIRED + ('deps2/LLVM/bin/clang.exe',))


if __name__ == '__main__':
    unittest.main()
