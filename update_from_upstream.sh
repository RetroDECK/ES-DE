#!/bin/bash

git fetch https://gitlab.com/es-de/emulationstation-de/ a59b8016be3ccaab0a678a552128d06b32e7dc01  # tag stable-3.1 on 4/1/25 - Fetch the latest changes from the remote master branch
git merge FETCH_HEAD  # Merge the fetched changes into your current branch
echo -e "PLEASE CHECK IF ANYTHING IS CHANGED IN:\n-resources/systems/linux/es_find_rules.xml\n-resources/systems/linux/es_systems.xml"