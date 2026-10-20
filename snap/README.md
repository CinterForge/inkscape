[Inkscape Developer Documentation](../doc/readme.md) /

Snap package
============

The `snap/` directory is used for building the snap (https://snapcraft.io/) package of Inkscape. Snap is a package format for Ubuntu and some related Linux distributions.

## Automatic build

The Inkscape snap package is automatically built and uploaded to the Snap store. There are three channels:

- "stable": Stable release - updated manually
- "candidate": Latest version of stable branch - automatic build - https://launchpad.net/~inkscape.dev/inkscape/+snap/inkscape-1.4.x
- "edge": Master development branch - automatic build - https://launchpad.net/~inkscape.dev/+snap/inkscape-master

The account on the snap store is owned by Ted Gould <ted@gould.cx> and some others.

If the snap does no longer build or run, the most probable reason is that we added a new dependency. Have a look at the recent changes in https://gitlab.com/inkscape/inkscape-ci-docker, and try to make a similar change to `build-packages` (build dependency) or `stage-packages` (runtime dependency) in `snapcraft.yaml`.

## Building locally

The following instructions assume that you already have the Inkscape repository cloned (recursively, that means including submodules).
Open a terminal in the top folder of the Inkscape repository.

First setup:
```
# Set up snapcraft
sudo snap install --classic snapcraft
```

Build:
```
# Make sure the build starts fresh, without any leftovers from the previous build.
# (Makes it much slower, but can be helpful to avoid weird errors.)
# You can skip this step for quick experiments.
snapcraft clean
# Build
snapcraft pack
```

```
# Make sure that no apt version of Inkscape is installed
sudo apt remove inkscape
# Install the build result locally (Adjust filename accordingly)
sudo snap install --dangerous inkscape_XXX.snap
# Fix access to .config/inkscape directory (TODO why is this needed)
sudo snap connect inkscape:dot-config-inkscape
```

### Troubleshooting
#### General
If the snap fails to build but it worked before, first run `snapcraft clean` and build again and see if the errors disappear. Too often, snapcraft remembers some parts of the previous build, which causes trouble.

To debug the build process, you can get a shell inside the build environment: `snapcraft package --debug` opens a shell after building fails. `snapcraft package --shell-after` opens a shell after building succeeded. Inside the build environment, `/root/parts/inkscape/src` is the inkscape source dir and `/root/parts/inkscape/build/` is the CMake build dir. The whole git repo is in `/root/project`.

First copy-and-paste this line.
```
cd /root/parts/inkscape/build/
```
Strangely, it is important that you do not copy-and-paste the above command together with further commands. This is because the shell inside the build environment has some incomplete magic that sets the build environment variables.

You should now see a message `build environment set for part...`. Then you can continue:
```
rm CMakeCache.txt
cmake /root/parts/inkscape/src -G Ninja -DCMAKE_INSTALL_PREFIX=
cmake --build .
```

#### Snap directory
```
The 'snap' directory is meant specifically for snapcraft, but it contains
the following non-snapcraft-related paths:
- README.md

This is unsupported and may cause unexpected behavior. If you must store
these files within the 'snap' directory, move them to 'snap/local'
```
Just ignore the message.

#### LXD
```
LXD is required but not installed. Do you wish to install LXD and configure it with the defaults? [y/N]:
```
Answer `y` <Enter>.


```
craft-providers error: Failed to install LXD: user must be manually added to 'lxd' group before using LXD.
```
Run `sudo adduser $USERNAME lxd` , then log out and log in again.


#### Store upload failed
```
(on launchpad.net)
Store upload failed ... Error found while validating
```

Install review-tools: `sudo snap install review-tools`

Check the produced snap: `review-tools.snap-review inkscape_XXX.snap`

You can ignore all errors stating `Human review required`. Everything else must be fixed.

