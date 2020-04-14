#!/bin/bash
set +e

if [ $# -lt 3 ]; then
    echo "Try  ./build.sh <RELEASE(LTSC|1909)> <ARCH(x64|x86)> <FLAVOUR(base|universal) <VERSION(optional)>"
    echo "i.e  ./build.sh LTSC x64 base"
    echo "i.e2 ./build.sh 1909 x86 universal v0.1_TEST"
    exit 1
fi

export RELEASE=$1
export ARCH=$2
export FLAVOUR=$3
export VERSION="${4:-v$(date +'%Y%m%d')}"

if [ "${RELEASE}" = "LTSC" ]; then
    export ISO_URL="https://software-download.microsoft.com/download/pr/17763.1.180914-1434.rs5_release_CLIENT_LTSC_EVAL_${ARCH}FRE_en-us.iso"
    if [ "${ARCH}" = "x64" ]; then
        export ISO_MD5="188ab801e4ce01a26973c345643f8d80"
        export GUEST_OS_TYPE="Windows10_64"
    elif [ "${ARCH}" = "x86" ]; then
        export ISO_MD5="9ae1967c3bf933499212f4663957ac1f"
        export GUEST_OS_TYPE="Windows10"
    else
        echo "Architecture ${ARCH} not defined"
        exit 1
    fi
elif [ "${RELEASE}" = "1909" ]; then
    export ISO_URL="https://software-download.microsoft.com/download/pr/18363.418.191007-0143.19h2_release_svc_refresh_CLIENTENTERPRISEEVAL_OEMRET_${ARCH}FRE_en-us.iso"
    if [ "${ARCH}" = "x64" ]; then
        export ISO_MD5="b3cd4bae54e74f1ca497216a3f347ca7"
        export GUEST_OS_TYPE="Windows10_64"
    elif [ "${ARCH}" = "x86" ]; then
        export ISO_MD5="b700ff7684be392f266b22289ca75979"
        export GUEST_OS_TYPE="Windows10"
    else
        echo "Architecture ${ARCH} not defined"
        exit 1
    fi
else
    echo "Release ${RELEASE} not defined"
    exit 1
fi

packer build -only=virtualbox-iso windows_10_${FLAVOUR}.json
