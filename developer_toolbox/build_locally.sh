#!/bin/bash

# WARNING: run this script from the project root folder, not from here!!

inject(){

    flatpak_user_installation="$HOME/.local/share/flatpak/app/net.retrodeck.retrodeck/current/active/files"
    flatpak_system_installation="/var/lib/flatpak/app/net.retrodeck.retrodeck/current/active/files"

    force_user=false
    force_system=false

    # Parse arguments
    while [[ "$#" -gt 0 ]]; do
        case $1 in
            --force-user) force_user=true ;;
            --force-system) force_system=true ;;
            *) echo "Unknown parameter: $1"; exit 1 ;;
        esac
        shift
    done

    # Determine installation path
    if [ "$force_user" = true ]; then
        echo "Forcing user mode installation."
        app="$flatpak_user_installation"
    elif [ "$force_system" = true ]; then
        echo "Forcing system mode installation."
        app="$flatpak_system_installation"
    elif [ -d "$flatpak_user_installation" ]; then
        echo "RetroDECK is installed in user mode, proceeding."
        app="$flatpak_user_installation"
    elif [ -d "$flatpak_system_installation" ]; then
        echo "RetroDECK is installed in system mode, proceeding."
        app="$flatpak_system_installation"
    else
        echo "RetroDECK installation not found, are you inside a flatpak? Quitting"
        exit 1
    fi

    # Copying files to the installation
    sudo make install DESTDIR="$app"

}

cmake -DRETRODECK=on -DCMAKE_INSTALL_PREFIX=/app . && make
read -p "Do you want to inject the files in this RetroDECK installation? (y/n): " choice
case "$choice" in 
    y|Y )
        echo "Injecting files..."
        inject
    ;;
    n|N )
        echo "Skipping file injection."
        sudo make install
    ;;
    * )
        echo "Invalid choice. Skipping file injection."
    ;;
esac