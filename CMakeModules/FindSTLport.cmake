#
#  CMake custom modules
#  Copyright (C) 2011-2015  Cedric OCHS
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

FIND_PACKAGE_HELPER(STLport iostream
  RELEASE stlport_cygwin stlport_gcc stlport_x stlport_x.5.2 stlport_statix stlport_static
  DEBUG stlport_cygwin_debug stlport_cygwin_stldebug stlport_gcc_debug stlport_gcc_stldebug stlportstld_x stlportstld_x.5.2 stlportd_statix stlportd_static)
