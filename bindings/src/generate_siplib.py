# generate_siplib.py
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
#
# You should have received a copy of the GNU General Public License
# along with yasim-avr.  If not, see <http://www.gnu.org/licenses/>.

from sipbuild import module as sipmod
from sipbuild.module.main import main as module_main
import sys
import os
import shutil
import tarfile

sys.argv = [sys.argv[0],
            '--sdist',
            '--sip-h',
            '--target-dir', '../siplib',
            'yasimavr.lib._sip']
module_main()

#shutil.copyfile('siplib-makefile', '../siplib/Makefile')

sip_abi_version = sipmod.abi_version.resolve_abi_version('')
abi_major_version = sip_abi_version.split('.')[0]
module_version = sipmod.abi_version.get_sip_module_version(abi_major_version)
print(module_version)

sdist_name = 'yasimavr_lib__sip-' + module_version

old_pwd = os.getcwd()
os.chdir('../siplib')

with tarfile.open(sdist_name + '.tar.gz') as f:
    def is_within_directory(directory, target):

        abs_directory = os.path.abspath(directory)
        abs_target = os.path.abspath(target)

        prefix = os.path.commonprefix([abs_directory, abs_target])

        return prefix == abs_directory

    def safe_extract(tar, path=".", members=None, *, numeric_owner=False):

        for member in tar.getmembers():
            member_path = os.path.join(path, member.name)
            if not is_within_directory(path, member_path):
                raise Exception("Attempted Path Traversal in Tar File")

        tar.extractall(path, members, numeric_owner=numeric_owner)


    safe_extract(f)

if not os.path.isdir(sdist_name):
    raise Exception()

if os.path.isdir('sip'):
    shutil.rmtree('sip')

os.rename(sdist_name, 'sip')

os.chdir(old_pwd)
