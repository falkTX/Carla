# SPDX-License-Identifier: GPL-2.0-or-later

"""Bounded rotating session logging for Franca."""

import atexit
import faulthandler
import logging
from logging.handlers import RotatingFileHandler
import os
from pathlib import Path
import platform
import sys
import threading

from qt_compat import qt_config

if qt_config == 5:
    from PyQt5.QtCore import QT_VERSION_STR, PYQT_VERSION_STR, QtMsgType, qInstallMessageHandler
else:
    from PyQt6.QtCore import QT_VERSION_STR, PYQT_VERSION_STR, QtMsgType, qInstallMessageHandler


DEFAULT_MAX_BYTES = 2 * 1024 * 1024  # 2 MiB per log file
DEFAULT_BACKUP_COUNT = 5             # ~12 MiB total cap on disk

_logger = None
_log_dir = None
_log_file = None
_crash_file_handle = None
_installed = False
_prev_qt_handler = None
_prev_excepthook = None
_prev_threading_excepthook = None
_orig_stdout = None
_orig_stderr = None


def default_log_dir():
    env_dir = os.environ.get("FRANCA_LOG_DIR")
    if env_dir:
        return Path(env_dir).expanduser()
    state_home = os.environ.get("XDG_STATE_HOME")
    base = Path(state_home).expanduser() if state_home else (Path.home() / ".local" / "state")
    return base / "franca" / "logs"


def get_log_dir():
    return _log_dir or default_log_dir()


def get_log_file():
    return _log_file or (get_log_dir() / "franca.log")


class _StreamTee:
    """Mirror stdout/stderr writes into the rotating logger without breaking console output."""

    def __init__(self, stream, logger, level, prefix):
        self._stream = stream
        self._logger = logger
        self._level = level
        self._prefix = prefix
        self._buffer = ""
        self._lock = threading.Lock()
        self._local = threading.local()

    def write(self, data):
        if self._stream is not None:
            try:
                self._stream.write(data)
            except Exception:
                pass
        if not data or getattr(self._local, "busy", False):
            return len(data) if data else 0
        self._local.busy = True
        try:
            with self._lock:
                self._buffer += data
                while "\n" in self._buffer:
                    line, self._buffer = self._buffer.split("\n", 1)
                    text = line.rstrip("\r")
                    if text:
                        self._logger.log(self._level, "[%s] %s", self._prefix, text)
        finally:
            self._local.busy = False
        return len(data)

    def flush(self):
        if self._stream is not None:
            try:
                self._stream.flush()
            except Exception:
                pass
        if getattr(self._local, "busy", False):
            return
        self._local.busy = True
        try:
            with self._lock:
                text = self._buffer.rstrip("\r\n")
                self._buffer = ""
                if text:
                    self._logger.log(self._level, "[%s] %s", self._prefix, text)
        finally:
            self._local.busy = False

    def isatty(self):
        return bool(self._stream and hasattr(self._stream, "isatty") and self._stream.isatty())

    def fileno(self):
        if self._stream is not None and hasattr(self._stream, "fileno"):
            return self._stream.fileno()
        raise OSError("Stream has no fileno")

    def __getattr__(self, name):
        return getattr(self._stream, name)


def _qt_message_handler(mode, context, message):
    level = logging.INFO
    if mode == QtMsgType.QtDebugMsg:
        level = logging.DEBUG
    elif mode == QtMsgType.QtWarningMsg:
        level = logging.WARNING
    elif mode in (QtMsgType.QtCriticalMsg, QtMsgType.QtFatalMsg):
        level = logging.ERROR

    if _logger is not None:
        _logger.log(level, "[Qt] %s", message)

    if _prev_qt_handler is not None:
        try:
            _prev_qt_handler(mode, context, message)
        except Exception:
            pass


def _excepthook(exc_type, exc_value, exc_tb):
    if _logger is not None:
        _logger.critical("Uncaught exception", exc_info=(exc_type, exc_value, exc_tb))
    if _prev_excepthook is not None:
        _prev_excepthook(exc_type, exc_value, exc_tb)


def _threading_excepthook(args):
    if _logger is not None:
        _logger.critical(
            "Uncaught exception in thread %s",
            getattr(args.thread, "name", "<unknown>"),
            exc_info=(args.exc_type, args.exc_value, args.exc_traceback),
        )
    if _prev_threading_excepthook is not None:
        _prev_threading_excepthook(args)


def log_message(text, level=logging.INFO):
    if _logger is None or not text:
        return
    for line in str(text).splitlines():
        cleaned = line.rstrip()
        if cleaned:
            _logger.log(level, "%s", cleaned)


def _shutdown():
    global _crash_file_handle
    if _logger is not None:
        _logger.info("======= Franca session ended (pid=%d) =======", os.getpid())
        for handler in list(_logger.handlers):
            try:
                handler.flush()
            except Exception:
                pass
    if _crash_file_handle is not None:
        try:
            faulthandler.disable()
            _crash_file_handle.flush()
            _crash_file_handle.close()
        except Exception:
            pass
        _crash_file_handle = None


def setup_logging(log_dir=None, max_bytes=DEFAULT_MAX_BYTES, backup_count=DEFAULT_BACKUP_COUNT,
                  capture_stdio=True, install_qt=True):
    """Initialize bounded rotating session logging and return the active log file Path."""
    global _logger, _log_dir, _log_file, _crash_file_handle, _installed
    global _prev_qt_handler, _prev_excepthook, _prev_threading_excepthook
    global _orig_stdout, _orig_stderr

    target_dir = Path(log_dir).expanduser() if log_dir is not None else default_log_dir()
    target_dir.mkdir(parents=True, exist_ok=True)
    target_file = target_dir / "franca.log"
    crash_file = target_dir / "franca-crash.log"

    logger = logging.getLogger("franca")
    logger.setLevel(logging.DEBUG)
    logger.propagate = False

    # Close any existing handlers (e.g. if reconfigured in tests)
    for old_handler in list(logger.handlers):
        logger.removeHandler(old_handler)
        try:
            old_handler.close()
        except Exception:
            pass

    handler = RotatingFileHandler(
        str(target_file),
        maxBytes=max_bytes,
        backupCount=backup_count,
        encoding="utf-8",
    )
    handler.setFormatter(logging.Formatter(
        "%(asctime)s.%(msecs)03d [%(levelname)s] %(message)s",
        datefmt="%Y-%m-%d %H:%M:%S",
    ))
    logger.addHandler(handler)

    _logger = logger
    _log_dir = target_dir
    _log_file = target_file

    # Keep crash traceback file bounded by truncating if it grew past 512 KiB
    try:
        if _crash_file_handle is not None:
            try:
                faulthandler.disable()
                _crash_file_handle.close()
            except Exception:
                pass
        mode = "w" if (crash_file.exists() and crash_file.stat().st_size > 512 * 1024) else "a"
        _crash_file_handle = open(crash_file, mode, encoding="utf-8")
        faulthandler.enable(file=_crash_file_handle, all_threads=True)
    except Exception:
        _crash_file_handle = None

    if not _installed:
        _installed = True
        _prev_excepthook = sys.excepthook
        sys.excepthook = _excepthook
        if hasattr(threading, "excepthook"):
            _prev_threading_excepthook = threading.excepthook
            threading.excepthook = _threading_excepthook
        atexit.register(_shutdown)

    if install_qt:
        prev = qInstallMessageHandler(_qt_message_handler)
        if prev is not _qt_message_handler:
            _prev_qt_handler = prev

    if capture_stdio:
        if _orig_stdout is None:
            _orig_stdout = sys.stdout
        if _orig_stderr is None:
            _orig_stderr = sys.stderr
        sys.stdout = _StreamTee(_orig_stdout, logger, logging.INFO, "stdout")
        sys.stderr = _StreamTee(_orig_stderr, logger, logging.WARNING, "stderr")

    logger.info(
        "======= Franca session started (pid=%d, python=%s, qt=%s, pyqt=%s, os=%s) =======",
        os.getpid(),
        sys.version.split()[0],
        QT_VERSION_STR,
        PYQT_VERSION_STR,
        platform.platform(),
    )
    logger.info("Log file: %s (max %d KiB x %d backups)", target_file, max_bytes // 1024, backup_count)
    return target_file
