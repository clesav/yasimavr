# Project configuration for the yasim-avr python bindings
#
# Copyright 2021 Clement Savergne <csavergne@yahoo.com>
#
# This file is part of yasim-avr.
#
# yasim-avr is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# yasim-avr is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with yasim-avr.  If not, see <http://www.gnu.org/licenses/>.


'''
This module's sole purpose is to add two options to sip-build:

'--gdb': This option allows to make a debug version of the bindings without
having to run the building script with a debug version of python.
It's achieved by setting the debug flag after code generation
but before compilation.
The python debug libraries are still needed for linking though.

'--no-line-directive': This option strips the generated code of all
#line preprocessor directives, making it slightly easier to debug the bindings
'''

import os
import shutil
from sipbuild import Project, Option, SetuptoolsBuilder


class yasimavr_bindings_project(Project):

    def get_options(self):
        options = super().get_options()

        gdb_opt = Option('gdb',
                         option_type=bool,
                         help='enable debug for the compiler',
                         metavar='ENABLE',
                         default=False)
        options.append(gdb_opt)

        line_opt = Option('no_line_directive',
                          option_type=bool,
                          help='remove the line directives from the generated code',
                          metavar='DISABLE',
                          default=False)
        options.append(line_opt)

        return options


class yasimavr_bindings_builder(SetuptoolsBuilder):

    def _strip_line_directives(self, buildable):
        for fp_src in buildable.sources:
            if not os.path.basename(fp_src).startswith('sip'): continue

            lines = open(fp_src).readlines()
            new_lines = [line
                         for line in lines
                         if not line.strip().startswith('#line ')]

            f = open(fp_src, 'w')
            for line in new_lines:
                f.write(line)
            f.close()


    def _build_extension_module(self, buildable):
        buildable.debug = self.project.gdb

        #If the no-line-directive option is set, strip all generated
        #source files of any line starting by the #line preprocessor directive
        if self.project.no_line_directive:
            self._strip_line_directives(buildable)

        super()._build_extension_module(buildable)
        
        for installable in buildable.installables:
            for f in installable.files:
                if os.path.splitext(f)[1] in ('.pyd', '.pyi', '.so'):
                    shutil.copy(f, '.')
