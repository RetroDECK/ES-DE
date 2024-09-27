#!/bin/bash

# WARNING: run this script from the project root folder, not from here!!

# Check if script is running with elevated privileges
if [ "$EUID" -ne 0 ]; then
    echo "The build might fail without some superuser permissions, please run me with sudo. Continue without sudo? [y/N]"
    read -r continue_without_sudo
    if [[ "$continue_without_sudo" != "y" ]]; then
        exit 1
    fi
fi

git submodule update --init --recursive

export GITHUB_WORKSPACE="."
export FOLDER="es-de-build"

chmod a+rwx -R "$FOLDER"

# Initialize the Flatpak repo
ostree init --mode=archive-z2 --repo=${GITHUB_WORKSPACE}/retrodeck-repo

automation_tools/update_es-de_manifest.sh

cp net.retrodeck.es-de.appdata.xml net.retrodeck.es-de.appdata.xml.bak
cp net.retrodeck.es-de.yml net.retrodeck.es-de.yml.bak

automation_tools/install_dependencies.sh
#automation_tools/cooker_build_id.sh
automation_tools/pre_build_automation.sh
#automation_tools/cooker_flatpak_portal_add.sh
# THIS SCRIPT IS BROKEN HENCE DISABLED FTM
# automation_tools/appdata_management.sh
#automation_tools/flatpak_build_download_only.sh
#automation_tools/flatpak_build_only.sh
#automation_tools/flatpak_build_bundle.sh

flatpak-builder --user --force-clean \
    --install-deps-from=flathub \
    --install-deps-from=flathub-beta \
    --repo="${GITHUB_WORKSPACE}/retrodeck-repo" \
    "${GITHUB_WORKSPACE}/${FOLDER}" \
    net.retrodeck.es-de.yml

flatpak build-bundle "${GITHUB_WORKSPACE}/retrodeck-repo" "$GITHUB_WORKSPACE/RetroDECK-ES-DE.flatpak" net.retrodeck.es-de

rm -f net.retrodeck.es-de.appdata.xml
rm -f net.retrodeck.es-de.yml
cp net.retrodeck.es-de.appdata.xml.bak net.retrodeck.es-de.appdata.xml
cp net.retrodeck.es-de.yml.bak net.retrodeck.es-de.yml