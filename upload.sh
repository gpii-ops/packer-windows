#!/bin/bash

set -e

VAGRANT_VM_RELEASE=$1
VAGRANT_VM_ARCH=$2
VAGRANT_VM_FLAVOUR=$3
VAGRANT_VM_VERSION=$4
VAGRANT_USER="gpii-ops"
VAGRANT_VM_NAME="windows10-${VAGRANT_VM_RELEASE}-eval-${VAGRANT_VM_ARCH}-${VAGRANT_VM_FLAVOUR}"
VAGRANT_VM_FILENAME="windows10-${VAGRANT_VM_RELEASE}-eval-${VAGRANT_VM_ARCH}-${VAGRANT_VM_FLAVOUR}-virtualbox-v${VAGRANT_VM_VERSION}.box"

# Upload the box
gcloud auth activate-service-account --key-file "${HOME}/gcloud-auth.json"
gsutil cp "${HOME}/boxes/${VAGRANT_VM_FILENAME}" gs://vagrant.raisingthefloor.org/

#https://www.vagrantup.com/docs/vagrant-cloud/api.html#creating-a-usable-box-from-scratch
# Create a new box
curl \
  --header "Content-Type: application/json" \
  --header "Authorization: Bearer $VAGRANT_CLOUD_TOKEN" \
  https://app.vagrantup.com/api/v1/boxes \
  --data "{ \"box\": { \"username\": \"${VAGRANT_USER}\", \"name\": \"${VAGRANT_VM_NAME}\", \"is_private\": false } }"

# Create a new version
curl \
  --header "Content-Type: application/json" \
  --header "Authorization: Bearer $VAGRANT_CLOUD_TOKEN" \
  "https://app.vagrantup.com/api/v1/box/${VAGRANT_USER}/${VAGRANT_VM_NAME}/versions" \
  --data "{ \"version\": { \"version\": \"${VAGRANT_VM_VERSION}\" } }"

FILENAME_MD5SUM=($(md5sum ${HOME}/boxes/${VAGRANT_VM_FILENAME}))

# Create a new provider
curl \
  --header "Content-Type: application/json" \
  --header "Authorization: Bearer $VAGRANT_CLOUD_TOKEN" \
  "https://app.vagrantup.com/api/v1/box/${VAGRANT_USER}/${VAGRANT_VM_NAME}/version/${VAGRANT_VM_VERSION}/providers" \
  --data "{ \"provider\": { \"name\": \"virtualbox\", \"url\": \"https://storage.googleapis.com/vagrant.raisingthefloor.org/${VAGRANT_VM_FILENAME}\", \"checksum_type\": \"md5\", \"checksum\": \"${FILENAME_MD5SUM}\" } }"

rm "${HOME}/boxes/${VAGRANT_VM_FILENAME}"
