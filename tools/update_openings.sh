#!/usr/bin/env bash
# Rebuilds assets/openings/openings.tsv from the Lichess opening names (https://github.com/lichess-org/chess-openings).
#
# The upstream data lists each opening as a sequence of moves. The program needs the position behind it, because
# that finds an opening after a transposition as well. The upstream script bin/gen.py plays the moves and writes the
# position (EPD) with it. We keep only the columns eco, name and epd.
#
# Usage: tools/update_openings.sh
# Needs: git, python3 with venv

set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
WORK="$(mktemp -d)"
trap 'rm -rf "${WORK}"' EXIT

git clone --depth 1 https://github.com/lichess-org/chess-openings "${WORK}/src"

python3 -m venv "${WORK}/venv"
"${WORK}/venv/bin/pip" install --quiet -r "${WORK}/src/bin/requirements.txt"

(cd "${WORK}/src" && "${WORK}/venv/bin/python" bin/gen.py a.tsv b.tsv c.tsv d.tsv e.tsv) \
    | cut -f1,2,5 > "${ROOT}/assets/openings/openings.tsv"

cp "${WORK}/src/COPYING.txt" "${ROOT}/assets/openings/COPYING.txt"

echo "Written: assets/openings/openings.tsv ($(wc -l < "${ROOT}/assets/openings/openings.tsv") lines)"
echo "Source revision: $(git -C "${WORK}/src" rev-parse --short HEAD)"
