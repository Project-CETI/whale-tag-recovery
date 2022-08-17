#!/bin/bash
#
# clang-format-all: a tool to run clang-format on an entire project
# Copyright (C) 2016 Evan Klitzke <evan@eklitzke.org>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

function usage {
    echo "Usage: $0 DIR..."
    exit 1
}

if [ $# -eq 0 ]; then
    usage
fi

find ./$1/ -iname '*.h' -o -iname '*.c' | xargs clang-format -i