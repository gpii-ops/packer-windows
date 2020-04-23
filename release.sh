#!/bin/bash

VAGRANT_VM_RELEASE=$1
VAGRANT_VM_ARCH=$2
VAGRANT_VM_FLAVOUR=$3
VAGRANT_VM_VERSION=$4
VAGRANT_USER="gpii-ops"
VAGRANT_VM_NAME="windows10-${VAGRANT_VM_RELEASE}-eval-${VAGRANT_VM_ARCH}-${VAGRANT_VM_FLAVOUR}"

# Release the version
curl \
  --silent \
  --header "Authorization: Bearer $VAGRANT_CLOUD_TOKEN" \
  "https://app.vagrantup.com/api/v1/box/${VAGRANT_USER}/${VAGRANT_VM_NAME}/version/${VAGRANT_VM_VERSION}/release" \
  --request PUT
