import ctypes
import sys


def load_lib():
    """
    Loads the SecureTeto library dynamically based on the current platform.

    Returns:
        ctypes.CDLL: The loaded library.
    """
    lib: ctypes.CDLL
    
    if sys.platform == "win32":
        lib = ctypes.CDLL("../bin/lib/libsecu_engine.dll")
    elif sys.platform == "linux":
        lib = ctypes.CDLL("../bin/lib/libsecu_engine.so")
    elif sys.platform == "darwin":
        lib = ctypes.CDLL("../bin/lib/libsecu_engine.dylib")
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
