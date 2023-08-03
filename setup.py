NAME = "yasimavr"
VERSION = "0.0.1"
DESCRIPTION = "Yet Another SIMulator for AVR"
LICENSE = "GPLv3"
AUTHOR = "Clement Savergne"
AUTHOR_EMAIL = "csavergne@yahoo.com"
URL = "https://github.com/clesav/yasimavr"


import glob
import os
import shutil
import sys
import dataclasses

from setuptools import setup, Extension
from setuptools.extension import Library
from setuptools.command.build_ext import build_ext
from distutils.sysconfig import get_config_var

from sipbuild.module import module as sip_module
from sipbuild.pyproject import PyProject

#Necessary in virtual building environment, to import from bindings.project
p = os.path.dirname(__file__)
if p not in sys.path:
    sys.path.append(p)

from bindings.project import yasimavr_bindings_project, yasimavr_bindings_builder


@dataclasses.dataclass
class LibraryData:
    name: str
    src_path : str

    def get_sources(self):
        src = glob.glob(os.path.join(self.src_path, '**/*.cpp'), recursive=True)
        src = [os.path.normpath(p) for p in src]
        return src

    def get_headers(self):
        hdr = glob.glob(os.path.join(self.src_path, '**/*.h'), recursive=True)
        hdr = [os.path.normpath(p) for p in hdr]
        return hdr

LIBRARIES = {
    'core': LibraryData('core', 'lib_core/src'),
    'arch_avr': LibraryData('arch_avr', 'lib_arch_avr/src'),
    'arch_xt': LibraryData('arch_xt', 'lib_arch_xt/src'),
}

GCC_ARGS = [
    '-std=c++17',
    '-O3',
    '-fmessage-length=0'
]


#Reimplementation of bindings_builder and bindings_project
#Here, we just want to generate the source code from the SIP files
#and create the Extension instances. We leave the compilation
#and linking to classic setuptools
class _BindingsSubPackageBuilder(yasimavr_bindings_builder):

    def __init__(self, project):
        super().__init__(project)
        self.ext_modules = None


    def build(self):
        '''build override: just generate the source code and convert
        to Extension instances
        '''
        self._generate_bindings()
        self._generate_scripts()

        self.ext_modules = []
        for buildable in self.project.buildables:
            ext = self._convert_to_extension(buildable)
            self.ext_modules.append(ext)


    def _convert_to_extension(self, buildable):
        '''Sub-function that converts a Buildable object
        returned by the sipbuild source generation step into a
        setuptools Extension object
        Some code is copied from sipbuild
        '''

        buildable.debug = self.project.gdb

        if self.project.no_line_directive:
            self._strip_line_directives(buildable)

        # Handle preprocessor macros.
        define_macros = []
        for macro in buildable.define_macros:
            parts = macro.split('=', maxsplit=1)
            name = parts[0]
            try:
                value = parts[1]
            except IndexError:
                value = None

            define_macros.append((name, value))

        #Make the paths relative to the project root dir
        old_build_dir = buildable.build_dir
        buildable.build_dir = os.getcwd()
        buildable.make_names_relative()
        buildable.build_dir = old_build_dir

        #Create the corresponding Extension instance and fill it
        ext = Extension(buildable.fq_name,
                        buildable.sources,
                        define_macros=define_macros,
                        extra_compile_args=buildable.extra_compile_args + GCC_ARGS,
                        extra_link_args=buildable.extra_link_args,
                        extra_objects=buildable.extra_objects,
                        include_dirs=buildable.include_dirs,
                        libraries=buildable.libraries,
                        library_dirs=buildable.library_dirs,
                        py_limited_api=buildable.uses_limited_api)

        #Add an extra attribute for the pep484 file
        if buildable.bindings.pep484_pyi:
            pyi_filename = buildable.target + '.pyi'
            ext.pyi_file = os.path.join(buildable.build_dir, pyi_filename)
        else:
            ext.pyi_file = None

        return ext


class _BindingsSubPackageProject(yasimavr_bindings_project):

    def __init__(self, sip_args, build_temp, build_lib):
        super().__init__()

        self.arguments = sip_args
        self.build_dir = self.build_temp = build_temp
        self.build_lib = build_lib

        self.setup(PyProject(), 'build', '')


    def configure(self, pyproject, section_name, tool):
        super().configure(pyproject, section_name, tool)
        self.builder_factory = _BindingsSubPackageBuilder


class yasimavr_build_ext(build_ext):


    def _resolve_libraries(self, ext):
        '''Utility function to handle dynamic library reference for linking
        A library referenced such as 'foo.bar' is for example converted
        into foo/libbar.cp37-win_amd64.dll.a
        That is designed for dependence between libraries/extensions built in
        the same setup
        '''
        for lib_name in ext.libraries[:]:
            #If a library referenced for linking is also to be built by this command
            if lib_name in self.ext_map:
                #Add the directory of the library to library_dirs
                lib_ext = self.ext_map[lib_name]
                lib_path = os.path.dirname(self.get_ext_fullpath(lib_ext.name))
                if lib_path not in ext.library_dirs:
                    ext.library_dirs.append(lib_path)

                #Add the library filename to libraries in a way that is recognized by the linker
                ext_suffix = get_config_var('EXT_SUFFIX')
                s = lib_ext.name.split('.')[-1]
                lib_name_for_linker = os.path.splitext(s + ext_suffix)[0]
                ext.libraries.remove(lib_name)
                ext.libraries.append(lib_name_for_linker)

                #Library objects are built shared in NT and static everywhere else
                #For static libraries, library references aren't transitives, so we
                #copy references across.
                if os.name != 'nt' and isinstance(lib_ext, Library):
                    for sub_lib_name in lib_ext.libraries:
                        if sub_lib_name not in ext.libraries:
                            ext.libraries.append(sub_lib_name)


    def finalize_options(self):
        #Due to library/extension dependencies with each other,
        #we can't support parallelizing
        self.parallel = False

        #Necessary to allow finalize_options() to be called multiple times
        self.swig_opts = None

        super().finalize_options()

        for ext in self.distribution.ext_modules:
            self._resolve_libraries(ext)


    def run(self):
        #=====================================================================
        #Create the SIP extension module to build
        #=====================================================================

        #Purge the temporary source directory
        siplib_temp = os.path.join(self.build_temp, 'siplib')
        shutil.rmtree(siplib_temp, ignore_errors=True)
        self.mkpath(siplib_temp)

        #Copy the siplib sources
        sip_abi_major_version = sip_module.resolve_abi_version(None).split('.')[0]
        siplib_sources = sip_module.copy_nonshared_sources(sip_abi_major_version, siplib_temp)

        #Use the sipbuild facilities to create the customised header files
        sip_module.module('yasimavr.lib._sip',
                          abi_version=None,
                          project=None,
                          sdist=False,
                          setup_cfg=True,
                          sip_h=True,
                          sip_rst=True,
                          target_dir=siplib_temp)

        #Add on-the-fly the siplib extension module to be built by setuptools
        siplib_extension = Extension('yasimavr.lib._sip', sources = siplib_sources)
        self.distribution.ext_modules.append(siplib_extension)

        #=====================================================================
        #Generate the SIP bindings source code and add to the extensions to build
        ##=====================================================================

        sipbuild_temp = os.path.abspath(os.path.join(self.build_temp, 'bindings'))
        self.mkpath(siplib_temp)

        sip_args= []
        if self.debug:
            sip_args += ['--tracing', '--gdb', '--no-line-directive']

        old_cwd = os.getcwd()
        os.chdir('bindings');
        sip_project = _BindingsSubPackageProject(sip_args, sipbuild_temp, self.build_lib)
        os.chdir(old_cwd)

        sip_project.build()

        #Add on-the-fly the binding extension modules to be built
        self.distribution.ext_modules.extend(sip_project.builder.ext_modules)

        #=====================================================================
        #Re-finalize the setup
        ##=====================================================================
        self.finalize_options()

        #=====================================================================
        #Copy the PEP484 files
        #=====================================================================
        for ext in sip_project.builder.ext_modules:
            if ext.pyi_file:
                ext_path = self.get_ext_fullpath(ext.name)
                ext_dir = os.path.dirname(ext_path)
                self.mkpath(ext_dir)
                pyi_path = os.path.join(ext_dir, os.path.basename(ext.pyi_file))
                self.copy_file(ext.pyi_file, pyi_path)

        #=====================================================================
        #Copy the header files in the include folder
        ##=====================================================================
        include_dir = os.path.dirname(self.get_ext_fullpath(NAME + '.include._'))
        for lib in LIBRARIES.values():
            hdr_list = lib.get_headers()
            for hdr in hdr_list:
                hdr_path = os.path.relpath(hdr, lib.src_path)
                hdr_pkg_path = os.path.join(include_dir, lib.name, hdr_path)
                self.mkpath(os.path.dirname(hdr_pkg_path))
                self.copy_file(hdr, hdr_pkg_path)

        #=====================================================================
        #run
        ##=====================================================================
        super().run()


setup(
    name = NAME,
    version = VERSION,
    description = DESCRIPTION,
    long_description = open("README.md").read(),
    author = AUTHOR,
    author_email = AUTHOR_EMAIL,
    license = LICENSE,
    license_files = ["LICENSE"],
    url = URL,

    python_requires = ">=3.7",
    platforms = "Any",
    dependencies = ["pyYAML", "pyvcd"],

    packages = [
        "yasimavr",
        "yasimavr.lib",
        "yasimavr.utils",
        "yasimavr.device_library",
        "yasimavr.device_library.builders",
        "yasimavr.device_library.configs",
        "yasimavr.cli"
    ],
    package_dir = {"yasimavr" : "py/yasimavr"},
    include_package_data = True,

    ext_modules = [
        Library(name='yasimavr.lib._core',
                sources=LIBRARIES['core'].get_sources(),
                libraries=['elf'],
                define_macros=[('YASIMAVR_DLL', None)],
                extra_compile_args=GCC_ARGS),

        Library(name='yasimavr.lib._arch_avr',
                sources=LIBRARIES['arch_avr'].get_sources(),
                include_dirs=['lib_core/src'],
                libraries=['yasimavr.lib._core'],
                extra_compile_args=GCC_ARGS),

        Library(name='yasimavr.lib._arch_xt',
                sources=LIBRARIES['arch_xt'].get_sources(),
                include_dirs=['lib_core/src'],
                libraries=['yasimavr.lib._core'],
                extra_compile_args=GCC_ARGS),
    ],

    cmdclass = {
        'build_ext': yasimavr_build_ext,
    },
)
