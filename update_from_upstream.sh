#!/bin/bash

git fetch https://gitlab.com/es-de/emulationstation-de/ "3.1.1"  # Fetch the latest changes from the remote master branch
git merge FETCH_HEAD  # Merge the fetched changes into your current branch
echo -e "PLEASE CHECK IF ANYTHING IS CHANGED IN:\n-resources/systems/linux/es_find_rules.xml\n-resources/systems/linux/es_systems.xml"