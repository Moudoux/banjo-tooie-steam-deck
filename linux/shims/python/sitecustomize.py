"""Run Windows executables through Wine, as WSL interop does for the Windows build.

Loaded through PYTHONPATH by linux/build.sh only; repository scripts are unchanged.
"""
import subprocess

_popen_init = subprocess.Popen.__init__


def _init(self, args, *rest, **kwargs):
    if isinstance(args, (list, tuple)) and args and str(args[0]).lower().endswith('.exe'):
        args = ['wine', *map(str, args)]
    _popen_init(self, args, *rest, **kwargs)


subprocess.Popen.__init__ = _init
