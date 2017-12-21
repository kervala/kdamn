#!/bin/sh

# Structure:
#
# |- Distribution
# |- <id>.pkg
#    |- Bom
#    |- PackageInfo
#    |- Payload

# Uncompress PKG : xar -xf <file.pkg>
# Uncompress Payload : cpio -iv < Payload

PKGDIR=$1
APPDIR=$2
PKGFILE=$3

CURRENTDIR=$(pwd)

if [ -z "$PKGDIR" ] || [ -z "$APPDIR" ] || [ -z "$PKGFILE" ]
then
  echo "Syntax: $0 <root directory of package> <root directory of .app bundle> <full path to PKG to create>"
  exit 1
fi

if [ ! -e "$PKGDIR/Distribution" ]
then
  echo "Unable to find Distribution file"
  exit 1
fi

PACKAGEDIR=$(ls -1 -d $PKGDIR/*.pkg)

# TODO: check only one line

if [ ! -e "$PACKAGEDIR/PackageInfo" ]
then
  echo "Unable to find PackageInfo file"
  exit 1
fi

# variables to change in files
NUMBER_OF_FILES=$(find $APPDIR | wc -l)
INSTALL_KBYTES=$(du -b -s $APPDIR | cut -f1)
INSTALL_KBYTES=$(($INSTALL_KBYTES/1024))

sed -i "s/INSTALL_KBYTES/"$INSTALL_KBYTES"/g" $PKGDIR/Distribution
sed -i "s/INSTALL_KBYTES/"$INSTALL_KBYTES"/g" $PACKAGEDIR/PackageInfo
sed -i "s/NUMBER_OF_FILES/"$NUMBER_OF_FILES"/g" $PACKAGEDIR/PackageInfo

# create Bom
echo "Creating Bom..."
cd $APPDIR
cd ..
mkbom -u 0 -g 0 . $PACKAGEDIR/Bom

RET=$?

if [ "$RET" -ne 0 ]
then
  echo "mkbom -u 0 -g 0 $APPDIR $PACKAGEDIR/Bom returned $RET"
  exit 1
fi

echo "Creating Payload..."
cd $APPDIR
find . | cpio -o --format odc --owner 0:80 | gzip -c -9 > $PACKAGEDIR/Payload

echo "Creating PKG..."
cd $PKGDIR
xar --compression none -cf $PKGFILE *

cd $CURRENTDIR
