                                  kdAmn

What is it?
-----------

kdAmn is a dAmn (deviantART Messaging Network) client written in C++ and
using Qt framework published under GPLv3 license.

Features
--------

* Implements dAmn protocol and some DiFi commands
* Multi-platform (Windows, GNU/Linux, OS X and possibly all other platforms supported by Qt)
* Download of thumbnails/icons/avatars/etc...
* oEmbed support for several sites, when a link is detected it displays a thumbnail
* OAuth2 authentication
* Timestamp display
* Users and commands auto-completion
* Multi-lines messages
* Save channels and connection parameters in an INI file
* System tray icon with notifications when someone mention our name or talks
* Log files in HTML or test for each channel
* Drag and drop to upload a file to Stash
* Upload screenshot to Stash
* Encode all HTML special characters because DA chat only supports ASCII
* Download and install new versions when detected
* Sound notification
* Support animated images
* Allow to create custom CSS for chat
* Check regularly for notes
* Display and send notes
* English and French translations

The Latest Version
------------------

* Windows and OS X version can be downloaded from:

http://dev.kervala.net/projects/kdamn/files

* Debian/Ubuntu version can be installed from PPA:

https://launchpad.net/~kervala/+archive/ubuntu/ppa

Install kdamn package from PPA ppa:kervala/ppa

You can add in your sources:
ppa:kervala/ppa

Or type:
sudo add-apt-repository ppa:kervala

apt-key adv --keyserver keyserver.ubuntu.com --recv-keys 5E354EB2
deb http://ppa.launchpad.net/kervala/ppa/ubuntu trusty main

And install kdamn package.

* Sources

Official repository (Mercurial): http://hg.kervala./kdamn
Bitbucket mirror (Mercurial): https://bitbucket.org/kervala/kdamn
Github mirror (Git): https://github.com/kervala/kdamn

Contributes
-----------

If you want to fill bug reports, send patches, etc... you can go on tracker:
http://dev.kervala.net/projects/kdamn/

If you want to contribute new translations, you can connect to Transifex service:
https://www.transifex.com/organization/kervala/dashboard/kdamn

Compilation
-----------

kdAmn is using Qt framework (4 and 5 are supported) and CMake
to manage projects files.

Last version of CMake can be downloaded from:
http://www.cmake.org/download/

Last version of Qt can be downloaded from:
http://www.qt.io/download-open-source/

* Windows

Be sure to have a working Visual C++ and you downloaded and installed
(or compiled) Qt and CMake.

You should install TortoiseHg to download sources :
http://tortoisehg.bitbucket.org

With TortoiseHg, clone Mercurial repository at:
http://hg.kervala./kdamn

Launch CMake and fill "Where is the source code" and
"Where to build the binaries" with directories you put sources and
where you want to compile.

* Ubuntu, Debian and derived

# install necessary packages 
sudo apt-get install mecurial debhelper cmake pkg-config qtbase5-dev \
  qttools5-dev-tools libqt5svg5-dev qttools5-dev qtmultimedia5-dev \
  libqt5svg5 qt5-image-formats-plugins libxmu-dev

# to download sources
hg clone http://hg.kervala./kdamn

cd kdamn

# to create a Debian package
debuild -b

You can also simply build it with :

mkdir build
cd build

cmake ..
make -j4

sudo make install
 
* OS X

Be sure to have Mercurial, CMake and Qt 5.x installed from MacPorts or
official sites as well as Xcode command-line tools.

Type :

# to download sources
hg clone http://hg.kervala./kdamn

cd kdamn
mkdir build
cd build

# to build kdAmn
cmake -DWITH_STATIC_EXTERNAL=ON -DQTDIR=/usr/local/Qt-5.4.0 ..
make -j4

# to create a PKG file
make packages

Licensing
---------

 kdAmn is a deviantART Messaging Network client
 Copyright (C) 2013-2015  Cédric OCHS

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.

 Please see the file called COPYING.

Contacts
--------

Cédric OCHS <kervala@gmail.com>
