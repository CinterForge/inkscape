[Developer documentation](../readme.md) / [Compiling Inkscape](./readme.md) /


# Advanced build options valid for all operating systems

This page lists detailed options for building Inkscape. They are valid for all operating systems.

Before this you should already have read the basic instructions for building on your operating system - see [Compiling Inkscape](./readme.md).


Build Options
-------------

A number of configuration settings can be overridden through CMake. To
see a list of the options available for Inkscape, run:

```sh
cmake -L
```
or, for more advanced cmake settings:

```sh
cmake --help
```

For example, to build Inkscape with only SVG 1 support, and no SVG 2, do:

```sh
cmake .. -DWITH_SVG2=OFF
```

# Ninja Build vs. Ninja Install

There are two variants of building:

1. Ninja Build (`ninja`): Only compile Inkscape. The resulting binary `bin/inkscape` (Windows: `bin/inkscape.exe`) does not yet work because it is missing the necessary files like UI layout, icons etc. (`share`) and translations (`po`).
2. Ninja Install (`ninja install`): Compile Inkscape (as in 1.) and copy the binary and all needed files into one folder `build/install_dir`. The location of that `install_dir` is set in the option`CMAKE_INSTALL_PREFIX` when calling `cmake`.

Here in the documentation we recommend `ninja install` to simplify the explanations.

If you want to keep multiple compiled versions of Inkscape, you can copy the `install_dir` of each version to another folder and run them from there.


## Faster build without "Ninja install"

This section describes a possibility to slightly improve build time that is, however, only recommended for advanced developers.

You can save a few seconds if you only run `ninja` instead of `ninja install`. This needs a workaround to set up and some caution when using.


In the following we will set up a directory named `dev_share` used as`CMAKE_INSTALL_PREFIX`.
```
cd build

mkdir -p dev_share
ln -s ../../share dev_share/inkscape
ln -s ../po/locale dev_share/locale
```

Run CMake with the new `CMAKE_INSTALL_PREFIX`: `cmake -DCMAKE_INSTALL_PREFIX=./dev_share .....`, where for "....." you insert all other options as in the normal (e.g., [Linux](./linux.md)) build instructions.

Build Inkscape using `ninja` instead of `ninja install`.

Run Inkscape using `./bin/inkscape` instead of `./install_dir/bin/inkscape`.

When using these workarounds, it is recommended that you **do not run `ninja install`** or any commands that depend on it (e.g., packaging for Linux DEB or `ninja dist-win-msi` for Windows EXE format). Else, some files may unexpectedly appear in the `share` folder of your repository and your Git status will show these unwanted changes.


# CMake Build Type (Debug or Release)

The standard for development is to create a debug build (`-DCMAKE_BUILD_TYPE=Debug`) that includes debugging symbols and enables stricter compiler settings and assertion checks at runtime.

For a Release build without debug information, use `-DCMAKE_BUILD_TYPE=Release`.

# Internal lib2geom

The standard for development is to use the latest development version of lib2geom (`-DWITH_INTERNAL_2GEOM=ON`). This version is included in the Inkscape source tree as a submodule (`src/3rdparty/2geom`).

If you want to use the system version instead, use `-DWITH_INTERNAL_2GEOM=OFF`.

# Make vs Ninja

Inkscape can also be built with Make instead of Ninja. This has no real benefit except if you are used to calling `make`.

If you use the suggested CMake commands but without `-G Ninja` then you will need to run `make` instead of `ninja`.
