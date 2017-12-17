from distutils.core import setup, Extension

energi_hash_module = Extension('energi_hash',
                                 sources = ['energimodule.c',
                                            'energi.c',
                                            'sha3/keccak-tiny.c',
                                            'sha3/keccak.c'],
                               include_dirs=['.', './sha3'])

setup (name = 'energi_hash',
       version = '0.0.1',
       description = 'Binding for energi proof of work hashing.',
       ext_modules = [energi_hash_module])
