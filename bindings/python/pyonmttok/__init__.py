import sys

if sys.platform == "win32":
    import os
    import ctypes
    import pkg_resources

    module_name = sys.modules[__name__].__name__

    add_dll_directory = getattr(os, "add_dll_directory", None)
    if add_dll_directory is not None:
        add_dll_directory(pkg_resources.resource_filename(module_name, ""))

    ctypes.CDLL(pkg_resources.resource_filename(module_name, "icudt.dll"))
    ctypes.CDLL(pkg_resources.resource_filename(module_name, "icuuc.dll"))
    ctypes.CDLL(pkg_resources.resource_filename(module_name, "OpenNMTTokenizer.dll"))

from ._ext import *
