#!/usr/bin/env bash
# Collects the texts of the program into assets/i18n/sharpblunder.pot and brings every assets/i18n/<code>.po up to date.
#
# The texts are the English strings in tr("..."), tr_c("context", "..."), tr_id("...", "###id"), tr_format("...") and
# tr_noop("..."). Run the script after a text has been added or changed. New texts show up in the .po files with an
# empty translation, texts that no longer exist are kept as obsolete entries (#~), and texts that changed a little are
# marked "fuzzy": the program ignores them until a person has checked them and removed the mark.
#
# To add a language: msginit --no-translator -i assets/i18n/sharpblunder.pot -l <code> -o assets/i18n/<code>.po
# and set the header X-Native-Name to the name of the language in that language (e.g. "Deutsch").
#
# Usage: tools/update_translations.sh
# Needs: GNU gettext (xgettext, msgmerge)

set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
I18N="${ROOT}/assets/i18n"
POT="${I18N}/sharpblunder.pot"

cd "${ROOT}"

# --no-location would hide where a text is used; the file name alone keeps the diffs small.
find src -name '*.cpp' -o -name '*.h' | sort | xargs xgettext \
    --language=C++ \
    --from-code=UTF-8 \
    --keyword=tr:1 \
    --keyword=tr_noop:1 \
    --keyword=tr_c:1c,2 \
    --keyword=tr_id:1 \
    --keyword=tr_format:1 \
    --add-comments=TRANSLATORS \
    --add-location=file \
    --sort-output \
    --package-name=SharpBlunder \
    --msgid-bugs-address=https://github.com/stwe/SharpBlunder/issues \
    --output="${POT}"

for po in "${I18N}"/*.po; do
    [ -e "${po}" ] || continue
    msgmerge --quiet --update --backup=none --add-location=file "${po}" "${POT}"
    echo "Updated: assets/i18n/$(basename "${po}") ($(msgfmt --statistics -o /dev/null "${po}" 2>&1))"
done

echo "Written: assets/i18n/sharpblunder.pot ($(grep -c '^msgid ' "${POT}") entries)"
