#!/bin/bash
set +e

if [ $# -lt 3 ]; then
    echo "Try  ./test.sh <RELEASE(LTSC|1909)> <ARCH(x64|x86)> <FLAVOUR(base|universal) <VERSION(optional)>"
    echo "i.e  ./test.sh LTSC x64 base"
    echo "i.e2 ./test.sh 1909 x86 universal v0.1_TEST"
    exit 1
fi

export RELEASE=$1
export ARCH=$2
export FLAVOUR=$3
export VERSION="${4:-v$(date +'%Y%m%d')}"

cp Vagrantfile.test Vagrantfile
BOXIMAGE=windows10-$RELEASE-eval-$ARCH-$FLAVOUR-virtualbox-v$VERSION
vagrant box add --name $BOXIMAGE boxes/$BOXIMAGE.box
vagrant up --no-provision
vagrant provision
vagrant destroy -f
vagrant box remove $BOXIMAGE
