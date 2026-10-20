Android
=======

This directory contains build scripts and resources used for Android packaging.

Explanation of files:

* `package.sh`

  The main packaging script, run by CI to generate the Android package.

* `AndroidManifest.xml`, `res/`

  These are copied directly into the apk by `package.sh`.

* `copylibs.py`

  A generic library bundling script used by `package.sh`.

* `genicons.py`

  Not used by CI. This is a manually-run script whose purpose is to regenerate the app icons (in `res`) if they need updating. The invocation to do so is

      ./genicons.py ../../share/icons/application/scalable/org.inkscape.Inkscape.svg -o res
