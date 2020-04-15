#!/bin/bash

VAGRANT_VM_RELEASE=$1
VAGRANT_VM_FLAVOUR=$2
VAGRANT_VM_ARCH=$3
VAGRANT_VM_VERSION=$4
VAGRANT_USER="gpii-ops"
VAGRANT_VM_NAME="windows10-${VAGRANT_VM_RELEASE}-eval-${VAGRANT_VM_ARCH}-${VAGRANT_VM_FLAVOUR}"
VAGRANT_VM_FILENAME="windows10-${VAGRANT_VM_RELEASE}-eval-${VAGRANT_VM_ARCH}-${VAGRANT_VM_FLAVOUR}-virtualbox-${VAGRANT_VM_VERSION}.box"

# Upload the box
# TODO: Command to upload the box.

#https://www.vagrantup.com/docs/vagrant-cloud/api.html#creating-a-usable-box-from-scratch
# Create a new box
curl \
  --header "Content-Type: application/json" \
  --header "Authorization: Bearer $VAGRANT_CLOUD_TOKEN" \
  https://app.vagrantup.com/api/v1/boxes \
  --data '{ "box": { "username": "${VAGRANT_USER}", "name": "${VAGRANT_VM_NAME}" } }'

# Create a new version
curl \
  --header "Content-Type: application/json" \
  --header "Authorization: Bearer $VAGRANT_CLOUD_TOKEN" \
  https://app.vagrantup.com/api/v1/box/${VAGRANT_USER}/${VAGRANT_VM_NAME}/versions \
  --data '{ "version": { "version": "${VAGRANT_VM_VERSION}" } }'

# Create a new provider
curl \
  --header "Content-Type: application/json" \
  --header "Authorization: Bearer $VAGRANT_CLOUD_TOKEN" \
  https://app.vagrantup.com/api/v1/box/${VAGRANT_USER}/${VAGRANT_VM_NAME}/version/${VAGRANT_VM_VERSION}/providers \
  --data '{ "provider": { "name": "virtualbox", "url": "http://wherever-in-the-cloud.net/virtual-machines/vagrant/virtualbox/${VAGRANT_VM_FILENAME}" } }'
