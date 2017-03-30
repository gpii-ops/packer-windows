#!/bin/bash
set +e

cd boxes
BOXIMAGE=$(ls -tc -1 | head -1)
cd ..
vagrant box add --name $BOXIMAGE boxes/$BOXIMAGE
sed "s/_VMBOX_/$BOXIMAGE/g" tests/Vagrantfile.template > Vagrantfile
vagrant up --no-provision
vagrant provision
vagrant destroy -f
vagrant box remove $BOXIMAGE