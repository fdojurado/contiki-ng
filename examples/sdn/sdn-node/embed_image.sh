#!/bin/sh

# embeds an exe/elf file into a the Flocklab XML config file

SED=sed
unamestr=`uname`
if [[ "$unamestr" == "Darwin" ]] ; then
    SED=gsed
    type $SED >/dev/null 2>&1 || {
        echo >&2 "$SED it's not installed. Try: brew install gnu-sed" ;
        exit 1;
    }
fi
XMLFILE=flocklab.xml

if [ $# -lt 1 ];
then
  echo "usage: $0 [input file (exe)] [output file (xml)]"
  exit 1
fi

if [ $# -gt 1 ] && [ -f $2 ];
then
  XMLFILE=$2
fi

if [ ! -f $1 ];
then
  echo "file $1 not found"
  exit 1
fi

if [ ! -f $XMLFILE ];
then
  echo "file $XMLFILE not found"
  exit 1
fi

B64FILE="$1.b64"

base64 $1 > $B64FILE

$SED -i -n '1h;1!H;${ g;s/<data>.*<\/data>/<data>\n<\/data>/;p}' $XMLFILE
$SED -i "/<data>/r ${B64FILE}" $XMLFILE
rm $B64FILE

echo "image $1 embedded into $XMLFILE"
