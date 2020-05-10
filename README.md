![Demo](data/preview.png)

## material-decoration

Material-ish window decoration theme for KWin.

### Locally Integrated Menus

This hides the AppMenu icon button and draws the menu in the titlebar.

![](https://i.imgur.com/oFOVWjV.png)

Make sure you add the AppMenu button in System Settings > Application Style > Window Decorations > Buttons Tab.

TODO/Bugs ([Issue #1](https://github.com/Zren/material-decoration/issues/1)):

* Open Submenu on Shortcut (eg: `Alt+F`)
* Display mnemonics when holding `Alt`

Upstream LIM discussion in the KDE Bug report: https://bugs.kde.org/show_bug.cgi?id=375951#c27

### Installation

#### Binary package

- openSUSE:
```
sudo zypper ar obs://home:trmdi trmdi
sudo zypper in -r trmdi material-decoration
```

#### Building from source
Build dependencies:

- Ubuntu:
```
sudo apt build-dep breeze
```


Download the source:

```
cd ~/Downloads
git clone https://github.com/Zren/material-decoration.git
cd material-decoration
```

Then compile the decoration, and install it:

```
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=/usr ..
make
sudo make install
```

Select Material in System Settings > Application Style > Window Decorations.

To test changes, restart `kwin_x11` with:

```
QT_LOGGING_RULES="*=false;kdecoration.material=true" kstart5 -- kwin_x11 --replace
```
