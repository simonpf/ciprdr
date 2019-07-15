from setuptools import setup, Extension, find_packages


ciprdr_cpp = Extension('ciprdr_cpp', sources = ['src/cip.cpp'])

setup (name = 'ciprdr',
       version = '0.0',
       description = 'Package for reading CIP particle images.',
       packages = find_packages(),
       author = 'Simon Pfreundschuh',
       author_email = 'simon.pfreundschuh@chalmers.se',
       long_description = '''
       Functions for decompression and parsing of CIP PAD greyscale
       images.
       ''',
       ext_modules = [ciprdr_cpp])

