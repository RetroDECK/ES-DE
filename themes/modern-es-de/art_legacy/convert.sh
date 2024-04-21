#!/bin/bash

EXTENSION=jpg

for file in $(ls *.${EXTENSION}); do
  echo convert $file $(basename -s $EXTENSION $file)webp
  convert $file $(basename -s $EXTENSION $file)webp
done
