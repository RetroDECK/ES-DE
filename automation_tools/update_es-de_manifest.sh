#!/bin/bash

git clone https://github.com/XargonWan/RetroDECK --depth=1 RetroDECK

# Creating ES-DE manifest
manifest_header="manifest-header.yml"
esde_module="es-de-module.yml"
esde_manifest="net.retrodeck.es-de.yml"
command="/app/bin/es-de" 

# sed -n '/command/q;p' RetroDECK/net.retrodeck.retrodeck.yml > "$manifest_header"  TEMPORARY DISABLED TO TRY A BUILD WITH ANOTHER RUNTIME
echo -e "command: $command\n" >> "$manifest_header"
sed -i '/^[[:space:]]*#/d' "$manifest_header"
sed -i 's/[[:space:]]*#.*$//' "$manifest_header"
sed -n '/finish-args:/,${/cleanup:/q;p;}' RetroDECK/net.retrodeck.es-de.yml >> "$manifest_header"

sed -i 's/net.retrodeck.retrodeck/net.retrodeck.es-de/' "$manifest_header"

cat "$manifest_header" > "$esde_manifest"
cat "$esde_module" >> "$esde_manifest"

rm -rf RetroDECK