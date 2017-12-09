#!/bin/sh

# Structure:
#
# |- Distribution
# |- <id>.pkg
#    |- Bom
#    |- PackageInfo
#    |- Payload

PKGDIR=$1
APPDIR=$2
PKGFILE=$3

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

echo "PACKAGEDIR $PACKAGEDIR"
# TODO: check only one line

if [ ! -e "$PACKAGEDIR/PackageInfo" ]
then
  echo "Unable to find PackageInfo file"
  exit 1
fi

# create Bom
echo "Creating Bom..."
mkbom -u 0 -g 0 $APPDIR $PACKAGEDIR/Bom

RET=$?

if [ "$RET" -ne 0 ]
then
  echo "mkbom -u 0 -g 0 $APPDIR PACKAGEDIR/Bom returned $RET"
  exit 1
fi

echo "Creating Payload..."
find $APPDIR | cpio -o --format odc --owner 0:80 | gzip -c -9 > $PACKAGEDIR/Payload

echo "Creating PKG..."
xar --compression none -cf $PKGFILE $PKGDIR/*
