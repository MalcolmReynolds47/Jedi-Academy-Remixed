"""Exercise replacement, rollback and repair without an installed game."""
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest
from unittest.mock import patch

import install


class DeploymentTest(unittest.TestCase):
    def setUp(self):
        temp = TemporaryDirectory()
        self.addCleanup(temp.cleanup)
        self.game = Path(temp.name) / "GameData"
        self.payload = Path(temp.name) / "payload"
        self.game.mkdir()
        self.payload.mkdir()
        (self.game / "a").write_bytes(b"old launcher")
        (self.game / "other.cfg").write_bytes(b"custom")
        for name in ("a", "b", "c"):
            (self.payload / name).write_bytes(name.encode())
        self.closed = patch.object(install, "check_closed")
        self.closed.start()
        self.addCleanup(self.closed.stop)

    def deploy(self):
        install.deploy(self.payload, self.game, None)

    def test_upgrade_and_reinstall(self):
        self.deploy()
        backups = list((self.game / "JA-R Backups").glob("*/a"))
        self.assertEqual([path.read_bytes() for path in backups], [b"old launcher"])
        timestamp = (self.game / "a").stat().st_mtime_ns
        self.deploy()
        self.assertEqual((self.game / "a").stat().st_mtime_ns, timestamp)
        self.assertEqual(list((self.game / "JA-R Backups").glob("*/a")), backups)
        self.assertEqual((self.game / "other.cfg").read_bytes(), b"custom")

    def test_partial_upgrade_rolls_back_new_and_replaced_files(self):
        replace = install.replace_file

        def fail_last(source, destination):
            if destination == self.game / "c":
                raise PermissionError("locked file")
            replace(source, destination)

        with patch.object(install, "replace_file", side_effect=fail_last):
            with self.assertRaises(PermissionError):
                self.deploy()
        self.assertEqual((self.game / "a").read_bytes(), b"old launcher")
        self.assertFalse((self.game / "b").exists())
        self.assertFalse((self.game / "c").exists())
        self.assertEqual((self.game / "other.cfg").read_bytes(), b"custom")


if __name__ == "__main__":
    unittest.main()
