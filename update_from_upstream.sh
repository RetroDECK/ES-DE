#!/bin/bash

git fetch https://gitlab.com/es-de/emulationstation-de/ master  # Fetch the latest changes from the remote master branch
git merge FETCH_HEAD  # Merge the fetched changes into your current branch