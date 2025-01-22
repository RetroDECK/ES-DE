#!/bin/bash

# WARNING: run this script from the project root folder, not from here!!

flatpak-builder --force-clean --user --install-deps-from=flathub --repo=esde-repo --install builddir net.retrodeck.es-de.yml