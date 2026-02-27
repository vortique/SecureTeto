import ctypes
import os
import sys


class ArchiveHeader(ctypes.Structure):
    _fields_ = [
        ("magic", ctypes.c_char * 4),
        ("version", ctypes.c_uint32),
        ("file_count", ctypes.c_uint64),
        ("file_table_offset", ctypes.c_uint64),
        ("data_table_offset", ctypes.c_uint64),
    ]


class ArchiveFileEntry(ctypes.Structure):
    _fields_ = [
        ("filename", ctypes.c_char * 512),
        ("offset", ctypes.c_uint64),
        ("size", ctypes.c_uint64),
        ("type", ctypes.c_uint8),
    ]


def load_lib():
    """
    Loads the SecureTeto library dynamically based on the current platform.

    Returns:
        ctypes.CDLL: The loaded library.
    """
    base = os.path.join(os.path.dirname(__file__), "../bin/lib")

    if sys.platform == "win32":
        lib = ctypes.CDLL(os.path.join(base, "libsecu_engine.dll"))
    elif sys.platform.startswith("linux"):
        lib = ctypes.CDLL(os.path.join(base, "libsecu_engine.so"))
    elif sys.platform == "darwin":
        lib = ctypes.CDLL(os.path.join(base, "libsecu_engine.dylib"))
    else:
        raise Exception("Unsupported platform")

    return lib


def create_archive(archive_path: str, dir_path: str):
    """
    Creates an archive from the specified directory.

    Args:
        archive_path (str): The path to the archive file to create.
        dir_path (str): The path to the directory to archive.

    Returns:
        int: 0 on success, error code otherwise.
    """
    lib = load_lib()
    lib.create_archive.argtypes = [ctypes.c_char_p, ctypes.c_char_p]
    lib.create_archive.restype = ctypes.c_int

    result: int = lib.create_archive(
        archive_path.encode("utf-8"), dir_path.encode("utf-8")
    )

    return result


def extract_archive(archive_path: str, dir_path: str):
    """
    Extracts the contents of an archive to the specified directory.

    Args:
        archive_path (str): The path to the archive file.
        dir_path (str): The path to the directory to extract to.

    Returns:
        int: 0 on success, error code otherwise.
    """
    lib = load_lib()
    lib.extract_archive.argtypes = [ctypes.c_char_p, ctypes.c_char_p]
    lib.extract_archive.restype = ctypes.c_int

    result: int = lib.extract_archive(
        archive_path.encode("utf-8"), dir_path.encode("utf-8")
    )

    return result
