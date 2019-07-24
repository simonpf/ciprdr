"""
ciprdr.api
==========

The :code:`api` sub-module defines the bindings for the C++ routines for
 decompression and parsing of the images files. Moreover, it defines Python
 classes that act as interface to the low-level C++ functions.
"""

import os
import glob
import ctypes as c
import datetime
import numpy as np

################################################################################
# Load DLL and defined functions.
################################################################################

path = os.path.join(os.path.dirname(__file__), "..", "ciprdr_cpp*.so")
lib  = glob.glob(path)[0]
cip_api = c.cdll.LoadLibrary(lib)

#
# Structure definitions
#

class TimeField(c.Structure):
    _fields_ = [("year", c.c_short),
                ("month", c.c_short),
                ("day", c.c_short),
                ("hour", c.c_short),
                ("minute", c.c_short),
                ("second", c.c_short),
                ("milliseconds", c.c_short),
                ("weekday", c.c_short)]

    def to_datetime(self):
        return datetime.datetime(self.year, self.month, self.day,
                                 self.hour, self.minute, self.second,
                                 self.milliseconds * 1000)

class ParticleImageStruct(c.Structure):
    _fields_ = [("v_air", c.c_ulong),
                ("count", c.c_ulong),
                ("microseconds", c.c_ulong),
                ("milliseconds", c.c_ulong),
                ("seconds", c.c_ulong),
                ("minutes", c.c_ulong),
                ("hours", c.c_ulong),
                ("slices", c.c_ulong),
                ("valid", c.c_bool),
                ("header", c.c_byte * 64),
                ("image", c.POINTER(c.c_byte))]

    @property
    def __array__(self):
        if self.slices == 0:
            return np.zeros((0, 64))
        return np.ctypeslib.as_array(self.image, shape = (self.slices, 64))


cip_api.read_index_file.argtypes = [c.c_char_p]
cip_api.read_index_file.restype = c.c_void_p

cip_api.get_next_index.argtypes = [c.c_void_p]
cip_api.get_next_index.restype = TimeField

cip_api.destroy_index_file.argtypes   = [c.c_void_p]
cip_api.destroy_index_file.restype = None

cip_api.read_image_file.argtypes   = [c.c_char_p]
cip_api.read_image_file.restype = c.c_void_p

cip_api.destroy_image_file.argtypes   = [c.c_void_p]
cip_api.destroy_image_file.restype = None

cip_api.set_timestamp_index.argtypes = [c.c_void_p, c.c_ulong]
cip_api.set_timestamp_index.restype = TimeField

cip_api.get_particle_image.argtypes   = [c.c_void_p]
cip_api.get_particle_image.restype = c.POINTER(ParticleImageStruct)

cip_api.destroy_particle_image.argtypes   = [c.c_void_p]
cip_api.destroy_particle_image.restype = None

################################################################################
# PadsImageFile
################################################################################

class PadsImageFile:

    def __init__(self, filename):
        self.ptr = cip_api.read_image_file(filename.encode())
        self.set_timestamp_index(0)

    def __del__(self):
        if not self is None:
            if not self.ptr is None:
                cip_api.destroy_image_file(self.ptr)
                self.ptr = None

    def set_timestamp_index(self, index):
        ts = cip_api.set_timestamp_index(self.ptr, index)
        self._time = datetime.datetime(ts.year,
                                      ts.month,
                                      ts.day,
                                      ts.hour,
                                      ts.minute,
                                      ts.second,
                                      ts.milliseconds * 1000)
        return ts

    def get_particle_image(self):
        ptr = cip_api.get_particle_image(self.ptr)
        struct = ptr[0]
        return ParticleImage(ptr, struct, self._time)

    @property
    def n_timestamps(self):
        return (c.c_ulong * 2).from_address(self.ptr)[0]

    @property
    def timestamp_index(self):
        return (c.c_ulong * 2).from_address(self.ptr)[1]

class PadsIndexFile:

    def __init__(self, filename):
        self.ptr = cip_api.read_index_file(filename.encode())

    def __del__(self):
        if not self is None:
            if not self.ptr is None:
                cip_api.destroy_index_file(self.ptr)
                self.ptr = None

    def get_next_timestamp(self):
        return cip_api.get_next_index(self.ptr)

    @property
    def n_timestamps(self):
        return c.c_long.from_address(self.ptr).value

class ParticleImage:

    def __init__(self, ptr_, struct_, timestamp_):
        self.ptr = ptr_
        self.struct = struct_
        self.timestamp = timestamp_

    def __del__(self):
        if not self is None:
            if not self.ptr is None:
                cip_api.destroy_particle_image(self.ptr)
                self.ptr = None

    @property
    def airspeed(self):
        return self.struct.v_air

    @property
    def particle_counter(self):
        return self.struct.count

    @property
    def slices(self):
        return self.struct.slices

    @property
    def time(self):
        dt = datetime.timedelta(
            hours = self.struct.hours,
            minutes = self.struct.minutes,
            seconds = self.struct.seconds,
            milliseconds = self.struct.milliseconds,
            microseconds =  self.struct.microseconds
        )
        return self.timestamp + dt

    @property
    def data(self):
        return np.array(self.struct.__array__, copy = True)
