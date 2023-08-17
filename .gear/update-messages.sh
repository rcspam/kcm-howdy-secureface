#!/bin/sh
# script to update po files
# run from source root

# kde localization bot
# https://invent.kde.org/sysadmin/l10n-scripty.git
SCRIPTY=$HOME/src/l10n-scripty

# estract messages from Messages.sh
PATH=$SCRIPTY:$PATH bash $SCRIPTY/extract-messages.sh
echo "Done extracting messages"

merge() {
    echo "Merging translations for $1"
    catalogs=`find . -name "$1.po"`
    for cat in $catalogs; do
        echo $cat
        msgmerge --no-fuzzy-matching -o $cat.new $cat po/$1.pot
        mv $cat.new $cat
    done
    echo "Done merging translations"
}

merge kcm_howdy

echo "Cleaning up"
rm -f po/kcm_howdy.pot
echo "Done"
