#!/usr/bin/env bash
# Rasterizes the piece SVGs of a piece set to PNG, because SFML cannot load SVG files.
#
# Usage: tools/render_pieces.sh [set-name] [size-in-pixels]
# Needs: rsvg-convert (Debian/Ubuntu: librsvg2-bin, Arch: librsvg)

set -euo pipefail

SET="${1:-cburnett}"
SIZE="${2:-160}"
DIR="$(cd "$(dirname "$0")/.." && pwd)/assets/pieces/${SET}"

for svg in "${DIR}"/svg/*.svg; do
    name="$(basename "${svg}" .svg)"
    rsvg-convert --width "${SIZE}" --height "${SIZE}" "${svg}" --output "${DIR}/${name}.png"
    echo "${name}.png"
done
