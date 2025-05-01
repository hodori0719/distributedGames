#!/bin/sh

ssh-add ~/.ssh/id_rsa
ssh-add ~/.ssh/identity

while true; do
	cd ~/www-local/pubsub/Merc; svn up; make doc
	sleep 21600
done
