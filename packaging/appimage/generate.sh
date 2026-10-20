#!/bin/bash

########################################################################
# Install build-time and run-time dependencies
########################################################################

export DEBIAN_FRONTEND=noninteractive
export APPIMAGE_EXTRACT_AND_RUN=1

########################################################################
# Install Inkscape to appdir/
########################################################################

cd build
DESTDIR="$PWD/appdir" cmake --install . --prefix /usr ; find appdir/
cp ./appdir/usr/share/icons/hicolor/256x256/apps/org.inkscape.Inkscape.png ./appdir/
sed -i -e 's|^Icon=.*|Icon=org.inkscape.Inkscape|g' ./appdir/usr/share/applications/org.inkscape.Inkscape.desktop # FIXME

########################################################################
# Bundle Python for extensions
########################################################################

apt-get update

make_ld_launcher() {
    cat > "$2" <<EOF
#!/bin/sh
HERE="\$(dirname "\$(readlink -f "\${0}")")"
exec "\${HERE}/../../lib64/ld-linux-x86-64.so.2" "\${HERE}/$1" "\$@"
EOF
    chmod a+x "$2"
}

apt_bundle() {
    apt-get download "$@"
    find *.deb -exec dpkg-deb -x {} appdir \;
    find *.deb -delete
}

PY_VER=3.14
apt_bundle \
    gir1.2-gdkpixbuf-2.0 \
    gir1.2-glib-2.0 \
    gir1.2-gtk-4.0 \
    gir1.2-pango-1.0 \
    glycin-loaders \
    libpython${PY_VER}-stdlib \
    libpython${PY_VER}-minimal \
    python${PY_VER} \
    python${PY_VER}-minimal \
    python3-bs4 \
    python3-cachecontrol \
    python3-cairo \
    python3-certifi \
    python3-chardet \
    python3-cssselect \
    python3-filelock \
    python3-gi \
    python3-gi-cairo \
    python3-idna \
    python3-lxml \
    python3-msgpack \
    python3-numpy \
    python3-packaging \
    python3-pil \
    python3-platformdirs \
    python3-pyparsing \
    python3-requests \
    python3-scour \
    python3-serial \
    python3-six \
    python3-tinycss2 \
    python3-urllib3 \
    python3-webencodings \
    python3-zstandard
(
    cd ./appdir/usr/bin
    make_ld_launcher "python${PY_VER}" python3
)

########################################################################
# Install our own gschema (to work even on older systems)
########################################################################

# Without this, we get crashes when some part of GTK expects a setting to exist.
# The AppRun script will set GSETTINGS_SCHEMA_DIR and/or XDG_DATA_DIRS for us.
apt_bundle gnome-settings-daemon-common
mkdir -p ./appdir/usr/share/glib-2.0/schemas/
cp /usr/share/glib-2.0/schemas/*.gschema.xml ./appdir/usr/share/glib-2.0/schemas/

########################################################################
# Generate AppImage
########################################################################

rm -rf ./appdir/usr/include
rm -rf ./appdir/usr/share/doc

goappimage_url="https://github.com/$(wget -q https://github.com/probonopd/go-appimage/releases/expanded_assets/continuous -O - | grep "appimagetool-.*-x86_64.AppImage" | head -n 1 | cut -d '"' -f 2)"
wget -c "$goappimage_url" -O goappimage
chmod +x goappimage

./goappimage -s --preserve_cwd deploy ./appdir/usr/share/applications/org.inkscape.Inkscape.desktop

# Make gdk-pixbuf loaders use relative paths, rather than absolute paths pointing at /usr/lib
sed -i -e 's|/usr/lib/x86_64-linux-gnu/gdk-pixbuf-.*/.*/loaders/||g' ./appdir/lib/x86_64-linux-gnu/gdk-pixbuf-*/*/loaders.cache

# If GTK_THEME is set, it overrides everything and changing the global theme GtkSetting on the fly will do nothing.
# goappimage's wrapper sets GTK_THEME=Default, thus preventing our theme preferences from ever taking effect.
# So just drop that bit of the wrapper here.
sed -i '/export GTK_THEME=/d' ./appdir/AppRun

# Inject a custom script for any dynamic setup we might need to do
sed -i '/^MAIN=/r../packaging/appimage/setup.sh' ./appdir/AppRun

ARCH=x86_64 VERSION=1.0 ./goappimage ./appdir

sha="$(git rev-parse --short HEAD)"
mv Inkscape*.AppImage* "../Inkscape-$sha.AppImage"
