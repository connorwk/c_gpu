#!/bin/sh
docker run -d \
  -it \
  --name sdl2-build \
  --mount type=bind,source="$(pwd)",target=/build-path \
  sdl2-build-env:1.0
