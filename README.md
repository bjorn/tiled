Tiled Map Editor - http://www.mapeditor.org/

About Tiled
-------------------------------------------------------------------------------

Tiled is a general purpose tile map editor for all tile-based games, such as
RPGs, platformers or Breakout clones.

Tiled is highly flexible. It can be used to create maps of any size, with no
restrictions on tile size, or the number of layers or tiles that can be used.
Maps, layers, tiles, and objects can all be assigned arbitrary properties.
Tiled's map format (TMX) is easy to understand and allows multiple tilesets to
be used in any map. Tilesets can be modified at any time.

[![Backers on Open Collective](https://opencollective.com/tiled/backers/badge.svg)](#backers) [![Sponsors on Open Collective](https://opencollective.com/tiled/sponsors/badge.svg)](#sponsors) [![Build Status](https://travis-ci.org/bjorn/tiled.svg?branch=master)](https://travis-ci.org/bjorn/tiled)
[![Build status](https://ci.appveyor.com/api/projects/status/ceb79jn5cf99y3qd/branch/master?svg=true)](https://ci.appveyor.com/project/bjorn/tiled/branch/master)
[![Bountysource](https://www.bountysource.com/badge/tracker?tracker_id=52019)](https://www.bountysource.com/trackers/52019-tiled?utm_source=52019&utm_medium=shield&utm_campaign=TRACKER_BADGE)
[![Translation status](https://hosted.weblate.org/widgets/tiled/-/shields-badge.svg)](https://hosted.weblate.org/engage/tiled/?utm_source=widget)

About the Qt Version
-------------------------------------------------------------------------------

Tiled was originally written in Java. In 2008, work began to develop a faster,
better looking, and easier-to-use version of Tiled based on the Qt framework.
This decision was made as the Qt framework has a greater feature set than is
offered by the standard Java libraries.


Compiling
-------------------------------------------------------------------------------

Before you can compile Tiled, you must ensure the Qt (>= 5.6) development
libraries have been installed:

* On Ubuntu/Debian: `sudo apt install qt5-default qttools5-dev-tools zlib1g-dev qtdeclarative5-dev`
* On Fedora:        `sudo dnf builddep tiled`
* On Arch Linux:    `sudo pacman -S qt`
* On macOS with [Homebrew](http://brew.sh/):
  + `brew install qt5`
  + `brew link qt5 --force`

Alternatively, you can [download Qt here](https://www.qt.io/download-qt-installer).
You will still need to install a development environment alongside and some
libraries depending on your system, for example:

* On Ubuntu/Debian: `sudo apt install build-essential zlib1g-dev libgl1-mesa-dev`
* On Windows:       Choose "MinGW" Qt version, or install Visual Studio
* On macOS:         Install Xcode

Next, compile by running:

    $ qmake (or qmake-qt5 on some systems)
    $ make

To perform a shadow build, run qmake from a different directory and refer
it to tiled.pro. For example:

    $ mkdir build
    $ cd build
    $ qmake ../tiled.pro
    $ make

You can now run Tiled using the executable in `bin/tiled`.

Installing
-------------------------------------------------------------------------------

To install Tiled, run `make install` from the terminal. By default, Tiled will
install itself to `/usr/local`.

The installation prefix can be changed when running qmake, or by changing the
install root when running `make install`. For example, to use an installation
prefix of  `/usr` instead of `/usr/local`:

    $ qmake -r PREFIX=/usr

Note: The -r recursive flag is required if you've run qmake before, as this
command will affect nested pro files.

To install Tiled to a packaging directory:

    $ make install INSTALL_ROOT=/tmp/tiled-pkg

By default, Tiled and its plugins are compiled with an Rpath that allows them
to find the shared *libtiled* library immediately after being compiled. When
packaging a Tiled map for distribution, the Rpath should be disabled by
appending `RPATH=no` to the qmake command.

## Contributors

This project exists thanks to all the people who contribute. [[Contribute](CONTRIBUTING.md)].
<a href="graphs/contributors"><img src="https://opencollective.com/tiled/contributors.svg?width=890" /></a>


## Backers

Thank you to all our backers! 🙏 [[Become a backer](https://opencollective.com/tiled#backer)]

<a href="https://opencollective.com/tiled#backers" target="_blank"><img src="https://opencollective.com/tiled/backers.svg?width=890"></a>


## Sponsors

Support this project by becoming a sponsor. Your logo will show up here with a link to your website. [[Become a sponsor](https://opencollective.com/tiled#sponsor)]

<a href="https://opencollective.com/tiled/sponsor/0/website" target="_blank"><img src="https://opencollective.com/tiled/sponsor/0/avatar.svg"></a>
<a href="https://opencollective.com/tiled/sponsor/1/website" target="_blank"><img src="https://opencollective.com/tiled/sponsor/1/avatar.svg"></a>
<a href="https://opencollective.com/tiled/sponsor/2/website" target="_blank"><img src="https://opencollective.com/tiled/sponsor/2/avatar.svg"></a>
<a href="https://opencollective.com/tiled/sponsor/3/website" target="_blank"><img src="https://opencollective.com/tiled/sponsor/3/avatar.svg"></a>
<a href="https://opencollective.com/tiled/sponsor/4/website" target="_blank"><img src="https://opencollective.com/tiled/sponsor/4/avatar.svg"></a>
<a href="https://opencollective.com/tiled/sponsor/5/website" target="_blank"><img src="https://opencollective.com/tiled/sponsor/5/avatar.svg"></a>
<a href="https://opencollective.com/tiled/sponsor/6/website" target="_blank"><img src="https://opencollective.com/tiled/sponsor/6/avatar.svg"></a>
<a href="https://opencollective.com/tiled/sponsor/7/website" target="_blank"><img src="https://opencollective.com/tiled/sponsor/7/avatar.svg"></a>
<a href="https://opencollective.com/tiled/sponsor/8/website" target="_blank"><img src="https://opencollective.com/tiled/sponsor/8/avatar.svg"></a>
<a href="https://opencollective.com/tiled/sponsor/9/website" target="_blank"><img src="https://opencollective.com/tiled/sponsor/9/avatar.svg"></a>


