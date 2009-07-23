#! /bin/sh -e
test -n "$srcdir" || srcdir=`dirname "$0"`
test -n "$srcdir" || srcdir=.
(
  cd "$srcdir" &&
  gnome-doc-prepare --copy --force &&
  AUTOPOINT='intltoolize --automake --copy' autoreconf --force --install
) || exit
test -n "$NOCONFIGURE" || "$srcdir/configure" "$@"
