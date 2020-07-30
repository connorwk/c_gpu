#!/bin/sh

if [ -z "$(docker container ls -a | grep sdl2-build)" ]
then
	docker run -d \
	  -it \
	  --name sdl2-build \
	  --mount type=bind,source="$(pwd)",target=/build-path \
	  sdl2-build-env:1.0
else
	docker start -a sdl2-build
fi
