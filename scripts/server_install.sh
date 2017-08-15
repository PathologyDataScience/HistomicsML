#!/bin/bash


if [ ! -e ../al_server/bin/al_serverd ]; then
	echo '!!!! al_serverd needs to be built first'
	exit
fi

cp ../al_server/bin/al_serverd /usr/local/bin
cp ../al_server/scripts/al_server /etc/init.d/

mkdir /etc/al_server
cp ../al_server/scripts/al_server.conf /etc/al_server	
