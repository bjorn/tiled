.. raw:: html

   <div class="new">New in Tiled 1.3</div>

.. |ro| replace:: *[read‑only]*

Scripting
=========

Introduction
------------

Tiled can be extended with the use of JavaScript. Scripts can be used to
implement :ref:`custom map formats <script-registerMapFormat>`,
:ref:`custom actions <script-registerAction>` and :ref:`new tools <script-registerTool>`.
Scripts can also :ref:`automate actions based on signals <script-connecting-to-signals>`.

On startup, Tiled will execute any script files present in
:ref:`extensions <script-extensions>`. In addition it is possible to run
scripts directly from :ref:`the console <script-console>`. All scripts share
a single JavaScript context.

.. note::

    A few example scripts and links to existing Tiled extensions are provided
    at the Tiled Extensions repository: https://github.com/mapeditor/tiled-extensions

.. warning::

    Most builds of Tiled use Qt 5.12 or later, which support ECMAScript 7.
    However, the builds for Windows XP and the snap release of Tiled currently
    rely on Qt 5.6 and Qt 5.9 respectively. ECMAScript 7 features will not
    work there and some other functionality is missing as well, like
    interrupting scripts on API usage errors and the creation of new
    Tiled-specific objects.

.. _script-extensions:

Scripted Extensions
^^^^^^^^^^^^^^^^^^^

Extensions are placed in a system-specific location. This folder can be opened
from the Plugins tab in the :doc:`Preferences dialog </manual/preferences>`.

+-------------+-----------------------------------------------------------------+
| **Windows** | | :file:`C:/Users/<USER>/AppData/Local/Tiled/extensions/`       |
+-------------+-----------------------------------------------------------------+
| **macOS**   | | :file:`~/Library/Preferences/Tiled/extensions/`               |
+-------------+-----------------------------------------------------------------+
| **Linux**   | | :file:`~/.config/tiled/extensions/`                           |
+-------------+-----------------------------------------------------------------+

An extension can be placed directly in the extensions directory, or in a
sub-directory. All scripts files found in these directories are executed on
startup.

When any loaded script is changed or when any files are added/removed from the
extensions directory, the script engine is automatically reinstantiated and the
scripts are reloaded. This way there is no need to restart Tiled when
installing extensions. It also makes it quick to iterate on a script until it
works as intended.

Apart from scripts, extensions can include images that can be used as the icon
for scripted actions or tools.

.. _script-console:

Console View
^^^^^^^^^^^^

In the Console view (*View > Views and Toolbars > Console*) you will
find a text entry where you can write or paste scripts to evaluate them.

You can use the Up/Down keys to navigate through previously entered
script expressions.

.. _script-connecting-to-signals:

Connecting to Signals
^^^^^^^^^^^^^^^^^^^^^

The script API provides signals to which functions can be connected.
Currently, the tiled module has the most useful :ref:`set of signals <script-tiled-signals>`.

Properties usually will have related signals which can be used to detect
changes to that property, but most of those are currently not
implemented.

To connect to a signal, call its ``connect`` function and pass in a
function object. In the following example, newly created maps
automatically get their first tile layer removed:

.. code:: js

    tiled.assetCreated.connect(function(asset) {
        if (asset.layerCount > 0) {
            asset.removeLayerAt(0)
            tiled.log("assetCreated: Removed automatically added tile layer.")
        }
    })

In some cases it will be necessary to later disconnect the function from
the signal again. This can be done by defining the function separately
and passing it into the ``disconnect`` function:

.. code:: js

    function onAssetCreated(asset) {
        // Do something...
    }

    tiled.assetCreated.connect(onAssetCreated)
    // ...
    tiled.assetCreated.disconnect(onAssetCreated)

API Reference
-------------

tiled module
^^^^^^^^^^^^

The ``tiled`` module is the main entry point and provides properties,
functions and signals which are documented below.

Properties
~~~~~~~~~~

.. csv-table::
    :widths: 1, 2

    **version** : string |ro|, Currently used version of Tiled.
    **platform** : string |ro|, "Operating system. One of ``windows``, ``macos``, ``linux`` or ``unix``
    (for any other UNIX-like system)."
    **arch** : string |ro|, "Processor architecture. One of ``x64``, ``x86`` or ``unknown``."
    **actions** : [string] |ro|, "Available actions for :ref:`tiled.trigger() <script-trigger>`."
    **menus** : [string] |ro|, "Available menus for :ref:`tiled.extendMenu() <script-extendMenu>`."
    **activeAsset** : :ref:`script-asset`, "Currently selected asset, or ``null`` if no file is open. Can be assigned
    any open asset in order to change the active asset."
    **openAssets** : array |ro|, "List of currently opened :ref:`assets <script-asset>`."
    **mapEditor** : :ref:`script-mapeditor`, "Access the editor used when editing maps."
    **tilesetEditor** : :ref:`script-tileseteditor`, "Access the editor used when editing tilesets."
    **tilesetFormats** : [string] |ro|, "List of supported tileset format names. Use
    :ref:`tilesetFormat <script-tilesetFormat>` to get the corresponding format object
    to read and write files. (Since 1.4)"
    **mapFormats** : [string] |ro|, "List of supported map format names. Use
    :ref:`mapFormat <script-mapFormat>` to get the corresponding format object to
    read and write files. (Since 1.4)"

Functions
~~~~~~~~~

.. _script-trigger:

tiled.trigger(action : string) : void
    This function can be used to trigger any registered action. This
    includes most actions you would normally trigger through the menu or by
    using their shortcut.

    Use the ``tiled.actions`` property to get a list of all available actions.

    Actions that are checkable will toggle when triggered.

.. _script-execute:

tiled.executeCommand(name : string, inTerminal : bool) : void
    Executes the first custom command with the given name, as if it was
    triggered manually. Works also with commands that are not currently enabled.

    Raises a script error if the command is not found.

.. _script-open:

tiled.open(fileName : string) : :ref:`script-asset`
    Requests to open the asset with the given file name. Returns a reference to
    the opened asset, or ``null`` in case there was a problem.

.. _script-close:

tiled.close(asset : :ref:`script-asset`) : bool
    Closes the given asset without checking for unsaved changes (to confirm the
    loss of any unsaved changes, set ``activeAsset`` and trigger the "Close"
    action instead).

.. _script-reload:

tiled.reload(asset : :ref:`script-asset`) : :ref:`script-asset`
    Reloads the given asset from disk, without checking for unsaved changes.
    This invalidates the previous script reference to the asset, hence the new
    reference is returned for convenience. Returns ``null`` if reloading failed.

tiled.alert(text : string [, title : string]) : void
    Shows a modal warning dialog to the user with the given text and
    optional title.

tiled.confirm(text : string [, title : string]) : bool
    Shows a yes/no dialog to the user with the given text and optional
    title. Returns ``true`` or ``false``.

tiled.prompt(label : string [, text : string [, title : string]]) : string
    Shows a dialog that asks the user to enter some text, along with the
    given label and optional title. The optional ``text`` parameter provides
    the initial value of the text. Returns the entered text.

tiled.log(text : string) : void
    Outputs the given text in the Console window as regular text.

tiled.warn(text : string, activated : function) : void
    Outputs the given text in the Console window as warning message and creates
    an issue in the Issues window.

    When the issue is activated (with double-click or Enter key) the given
    callback function is invoked.

tiled.error(text : string, activated : function) : void
    Outputs the given text in the Console window as error message and creates
    an issue in the Issues window.

    When the issue is activated (with double-click or Enter key) the given
    callback function is invoked.

.. _script-registerAction:

tiled.registerAction(id : string, callback : function) : :ref:`script-action`
    Registers a new action with the given ``id`` and ``callback`` (which is
    called when the action is triggered). The returned action object can be
    used to set (and update) various properties of the action.

    Example:

    .. code:: javascript

        var action = tiled.registerAction("CustomAction", function(action) {
            tiled.log(action.text + " was " + (action.checked ? "checked" : "unchecked"))
        })

        action.text = "My Custom Action"
        action.checkable = true
        action.shortcut = "Ctrl+K"

    The shortcut will currently only work when the action is added to a menu
    using :ref:`tiled.extendMenu() <script-extendMenu>`.

.. _script-registerMapFormat:

tiled.registerMapFormat(shortName : string, mapFormat : object) : void
    Registers a new map format that can then be used to open and/or save maps
    in that format.

    If a map format is already registered with the same ``shortName``,
    the existing format is replaced. The short name can also be used to
    specify the format when using ``--export-map`` on the command-line,
    in case the file extension is ambiguous or a different one should be
    used.

    The ``mapFormat`` object is expected to have the following properties:

    .. csv-table::
        :widths: 1, 2

        **name** : string, Name of the format as shown in the file dialog.
        **extension** : string, The file extension used by the format.
        "**read** : function(fileName : string) : :ref:`script-map`", "A function
        that reads a map from the given file. Can use :ref:`TextFile <script-textfile>` or
        :ref:`BinaryFile <script-binaryfile>` to read the file."
        "**write** : function(map : :ref:`script-map`, fileName : string) : string | undefined", "A function
        that writes a map to the given file. Can use :ref:`TextFile <script-textfile>` or
        :ref:`BinaryFile <script-binaryfile>` to write the file. When a non-empty string is returned, it is shown as error message."
        "**outputFiles** : function(map : :ref:`script-map`, fileName : string) : [string]", "A function
        that returns the list of files that will be written when exporting the given map (optional)."

    Example that produces a simple JSON representation of a map:

    .. code:: javascript

        var customMapFormat = {
            name: "Custom map format",
            extension: "custom",

            write: function(map, fileName) {
                var m = {
                    width: map.width,
                    height: map.height,
                    layers: []
                };

                for (var i = 0; i < map.layerCount; ++i) {
                    var layer = map.layerAt(i);
                    if (layer.isTileLayer) {
                        var rows = [];
                        for (y = 0; y < layer.height; ++y) {
                            var row = [];
                            for (x = 0; x < layer.width; ++x)
                                row.push(layer.cellAt(x, y).tileId);
                            rows.push(row);
                        }
                        m.layers.push(rows);
                    }
                }

                var file = new TextFile(fileName, TextFile.WriteOnly);
                file.write(JSON.stringify(m));
                file.commit();
            },
        }

        tiled.registerMapFormat("custom", customMapFormat)

.. _script-registerTilesetFormat:

tiled.registerTilesetFormat(shortName : string, tilesetFormat : object) : void
    Like :ref:`registerMapFormat <script-registerMapFormat>`, but registers a
    custom tileset format instead.

    The ``tilesetFormat`` object is expected to have the following properties:

    .. csv-table::
        :widths: 1, 2

        **name** : string, Name of the format as shown in the file dialog.
        **extension** : string, The file extension used by the format.
        "**read** : function(fileName : string) : :ref:`script-tileset`", "A function
        that reads a tileset from the given file. Can use :ref:`TextFile <script-textfile>` or
        :ref:`BinaryFile <script-binaryfile>` to read the file."
        "**write** : function(tileset : :ref:`script-tileset`, fileName : string) : string | undefined", "A function
        that writes a tileset to the given file. Can use :ref:`TextFile <script-textfile>` or
        :ref:`BinaryFile <script-binaryfile>` to write the file. When a non-empty string is returned, it is shown as error message."

.. _script-registerTool:

tiled.registerTool(shortName : string, tool : object) : object
    Registers a custom tool that will become available on the Tools tool bar
    of the Map Editor.

    If a tool is already registered with the same ``shortName`` the existing
    tool is replaced.

    The ``tool`` object has the following properties:

    .. csv-table::
        :widths: 1, 2

        **name** : string, Name of the tool as shown on the tool bar.
        **map** : :ref:`script-map`, Currently active tile map.
        **selectedTile** : :ref:`script-tile`, The last clicked tile for the active map. See also the ``currentBrush`` property of :ref:`script-mapeditor`.
        **preview** : :ref:`script-map`, Get or set the preview for tile layer edits.
        **tilePosition** : :ref:`script-point`, Mouse cursor position in tile coordinates.
        **statusInfo** : string, Text shown in the status bar while the tool is active.
        **enabled** : bool, Whether this tool is enabled.
        "**activated** : function() : void", Called when the tool was activated.
        "**deactivated** : function() : void", Called when the tool was deactivated.
        "**keyPressed** : function(key, modifiers) : void", Called when a key was pressed while the tool was active.
        "**mouseEntered** : function() : void", Called when the mouse entered the map view.
        "**mouseLeft** : function() : void", Called when the mouse left the map view.
        "**mouseMoved** : function(x, y, modifiers) : void", Called when the mouse position in the map scene changed.
        "**mousePressed** : function(button, x, y, modifiers) : void", Called when a mouse button was pressed.
        "**mouseReleased** : function(button, x, y, modifers) : void", Called when a mouse button was released.
        "**mouseDoubleClicked** : function(button, x, y, modifiers) : void", Called when a mouse button was double-clicked.
        "**modifiersChanged** : function(modifiers) : void", Called when the active modifier keys changed.
        "**languageChanged** : function() : void", Called when the language was changed.
        "**mapChanged** : function(oldMap : :ref:`script-map`, newMap : :ref:`script-map`) : void", Called when the active map was changed.
        "**tilePositionChanged** : function() : void", Called when the hovered tile position changed.
        "**updateStatusInfo** : function() : void", Called when the hovered tile position changed. Used to override the default updating of the status bar text.
        "**updateEnabledState** : function() : void", Called when the map or the current layer changed.

    Here is an example tool that places a rectangle each time the mouse has
    moved by 32 pixels:

    .. code:: javascript

        var tool = tiled.registerTool("PlaceRectangles", {
            name: "Place Rectangles",

            mouseMoved: function(x, y, modifiers) {
                if (!this.pressed)
                    return

                var dx = Math.abs(this.x - x)
                var dy = Math.abs(this.y - y)

                this.distance += Math.sqrt(dx*dx + dy*dy)
                this.x = x
                this.y = y

                if (this.distance > 32) {
                    var objectLayer = this.map.currentLayer

                    if (objectLayer && objectLayer.isObjectLayer) {
                        var object = new MapObject(++this.counter)
                        object.x = Math.min(this.lastX, x)
                        object.y = Math.min(this.lastY, y)
                        object.width = Math.abs(this.lastX - x)
                        object.height = Math.abs(this.lastY - y)
                        objectLayer.addObject(object)
                    }

                    this.distance = 0
                    this.lastX = x
                    this.lastY = y
                }
            },

            mousePressed: function(button, x, y, modifiers) {
                this.pressed = true
                this.x = x
                this.y = y
                this.distance = 0
                this.counter = 0
                this.lastX = x
                this.lastY = y
            },

            mouseReleased: function(button, x, y, modifiers) {
                this.pressed = false
            },
        })


.. _script-extendMenu:

tiled.extendMenu(id : string, items : array | object) : void
    Extends the menu with the given ID. Supports both a list of items or a
    single item. Available menu IDs can be obtained using the ``tiled.menus``
    property.

    A menu item is defined by an object with the following properties:

    .. csv-table::
        :widths: 1, 2

        **action** : string, ID of a registered action that the menu item will represent.
        **before** : string, ID of the action before which this menu item should be added (optional).
        **separator** : bool, Set to ``true`` if this item is a menu separator (optional).

    If a menu item does not include a ``before`` property, the value is
    inherited from the previous item. When this property is not set at all,
    the items are appended to the end of the menu.

    Example that adds a custom action to the "Edit" menu, before the "Select
    All" action and separated by a separator:

    .. code:: javascript

        tiled.extendMenu("Edit", [
            { action: "CustomAction", before: "SelectAll" },
            { separator: true }
        ]);

    The "CustomAction" will need to have been registered before using
    :ref:`tiled.registerAction() <script-registerAction>`.


.. _script-tilesetFormat:

tiled.tilesetFormat(shortName : string) : :ref:`script-tilesetformatwrapper`
    Returns the tileset format object with the given name, or `undefined` if
    no object was found. See the `tilesetFormats` property for more info.

.. _script-tilesetFormatForFile:

tiled.tilesetFormatForFile(fileName : string) : :ref:`script-tilesetformatwrapper`
    Returns the tileset format object that can read the given file, or `undefined`
    if no object was found.

.. _script-mapFormat:

tiled.mapFormat(shortName : string) : :ref:`script-mapformatwrapper`
    Returns the map format object with the given name, or `undefined` if no object
    was found. See the `mapFormats` property for more info.

.. _script-mapFormatForFile:

tiled.mapFormatForFile(fileName : string) : :ref:`script-mapformatwrapper`
    Returns the map format object that can read the given file, or `undefined` if
    no object was found.

.. _script-tiled-filePath:

tiled.filePath(path : url) : :ref:`script-filepath`
    Creates a :ref:`script-filepath` object with the given URL.

.. _script-tiled-objectRef:

tiled.objectRef(id : int) : :ref:`script-objectref`
    Creates an :ref:`script-objectref` object with the given ID.

.. _script-tiled-signals:

Signals
~~~~~~~

tiled.assetCreated(asset : :ref:`script-asset`)
    A new asset has been created.

tiled.assetOpened(asset : :ref:`script-asset`)
    An asset has been opened.

tiled.assetAboutToBeSaved(asset : :ref:`script-asset`)
    An asset is about to be saved. Can be used to make last-minute changes.

tiled.assetSaved(asset : :ref:`script-asset`)
    An asset has been saved.

tiled.assetAboutToBeClosed(asset : :ref:`script-asset`)
    An asset is about to be closed.

tiled.activeAssetChanged(asset : :ref:`script-asset`)
    The currently active asset has changed.


.. _script-action:

Action
^^^^^^

An action that was registered with :ref:`tiled.registerAction() <script-registerAction>`.
This class is used to change the properties of the action. It can be added to a menu using
:ref:`tiled.extendMenu() <script-extendMenu>`.

Properties
~~~~~~~~~~

.. csv-table::
    :widths: 1, 2

    **checkable** : bool, Whether the action can be checked.
    **checked** : bool, Whether the action is checked.
    **enabled** : bool, Whether the action is enabled.
    **icon** : string, File name of an icon.
    **iconVisibleInMenu** : bool, Whether the action should show an icon in a menu.
    **id** : string |ro|, The ID this action was registered with.
    **shortcut** : QKeySequence, The shortcut (can be assigned a string like "Ctrl+K").
    **text** : string, The text used when the action is part of a menu.
    **visible** : bool, Whether the action is visible.

Functions
~~~~~~~~~

Action.trigger() : void
    Triggers the action.

Action.toggle() : void
    Changes the checked state to its opposite state.

.. _script-asset:

Asset
^^^^^

Inherits :ref:`script-object`.

Represents any top-level data type that can be saved to a file. Currently
either a :ref:`script-map` or a :ref:`script-tileset`.

For assets that are loaded in the editor, all modifications and modifications
to their contained parts create undo commands. This includes both modifying
functions that are called as well as simply assigning to a writable property.

Properties
~~~~~~~~~~

.. csv-table::
    :widths: 1, 2

    **fileName** : string |ro|, File name of the asset.
    **modified** : bool |ro|, Whether the asset was modified after it was saved or loaded.
    **isTileMap** : bool |ro|, Whether the asset is a :ref:`script-map`.
    **isTileset** : bool |ro|, Whether the asset is a :ref:`script-tileset`.

Functions
~~~~~~~~~

.. _script-asset-macro:

Asset.macro(text : string, callback : function) : value
    Creates a single undo command that wraps all changes applied to this asset
    by the given callback. Recommended to avoid spamming the undo stack with
    small steps that the user does not care about.

    Example function that changes visibility of multiple layers in one step:

    .. code:: javascript

        tileMap.macro((visible ? "Show" : "Hide") + " Selected Layers", function() {
            tileMap.selectedLayers.forEach(function(layer) {
                layer.visible = visible
            })
        })

    The returned value is whatever the callback function returned.

.. _script-asset-undo:

Asset.undo() : void
    Undoes the last applied change. Note that the undo system is only enabled
    for assets loaded in the editor!

.. _script-asset-redo:

Asset.redo() : void
    Redoes the last change that was undone. Note that the undo system is only
    enabled for assets loaded in the editor!

.. _script-fileformat:

FileFormat
^^^^^^^^^^

Common functionality for file format readers and writers. (Since 1.4)

Properties
~~~~~~~~~~

.. csv-table::
    :widths: 1, 2

    **canRead** : bool |ro|, Whether this format supports reading files.
    **canWrite** : bool |ro|, Whether this format supports writing files.

Functions
~~~~~~~~~

FileFormat.supportsFile(fileName : string) : bool
    Returns whether the file is readable by this format.

.. _script-grouplayer:

GroupLayer
^^^^^^^^^^

Inherits :ref:`script-layer`.

Properties
~~~~~~~~~~

.. csv-table::
    :widths: 1, 2

    **layerCount** : int |ro|, Number of child layers the group layer has.

Functions
~~~~~~~~~

new GroupLayer([name : string])
    Constructs a new group layer.

GroupLayer.layerAt(index : int) : :ref:`script-layer`
    Returns a reference to the child layer at the given index.

GroupLayer.removeLayerAt(index : int) : void
    Removes the child layer at the given index. When a reference to the layer
    still exists and this group layer isn't already standalone, that reference
    becomes a standalone copy of the layer.

GroupLayer.removeLayer(layer : :ref:`script-layer`) : void
    Removes the given layer from the group. If this group wasn't standalone,
    the reference to the layer becomes a standalone copy.

GroupLayer.insertLayerAt(index : int, layer : :ref:`script-layer`) : void
    Inserts the layer at the given index. The layer can't already be part of
    a map.

GroupLayer.addLayer(layer : :ref:`script-layer`) : void
    Adds the layer to the group, above all existing layers. The layer can't
    already be part of a map.

.. _script-imagelayer:

ImageLayer
^^^^^^^^^^

Inherits :ref:`script-layer`.

Properties
~~~~~~~~~~

.. csv-table::
    :widths: 1, 2

    **transparentColor** : color, Color used as transparent color when rendering the image.
    **imageSource** : url, Reference to the image rendered by this layer.

.. _script-layer:

Layer
^^^^^

Inherits :ref:`script-object`.

Properties
~~~~~~~~~~

.. csv-table::
    :widths: 1, 2

    **name** : string, Name of the layer.
    **opacity** : number, "Opacity of the layer, from 0 (fully transparent) to 1 (fully opaque)."
    **visible** : bool, Whether the layer is visible (affects child layer visibility for group layers).
    **locked** : bool, Whether the layer is locked (affects whether child layers are locked for group layers).
    **offset** : :ref:`script-point`, Offset in pixels that is applied when this layer is rendered.
    **map** : :ref:`script-map`, Map that this layer is part of (or ``null`` in case of a standalone layer).
    **selected** : bool, Whether the layer is selected.
    **isTileLayer** : bool |ro|, Whether this layer is a :ref:`script-tilelayer`.
    **isObjectLayer** : bool |ro|, Whether this layer is an :ref:`script-objectgroup`.
    **isGroupLayer** : bool |ro|, Whether this layer is a :ref:`script-grouplayer`.
    **isImageLayer** : bool |ro|, Whether this layer is an :ref:`script-imagelayer`.

.. _script-mapobject:

MapObject
^^^^^^^^^

Inherits :ref:`script-object`.

Properties
~~~~~~~~~~

.. csv-table::
    :widths: 1, 2

    **id** : int |ro|, Unique (map-wide) ID of the object.
    **shape** : int, :ref:`Shape <script-mapobject-shape>` of the object.
    **name** : string, Name of the object.
    **type** : string, Type of the object.
    **x** : number, X coordinate of the object in pixels.
    **y** : number, Y coordinate of the object in pixels.
    **pos** : :ref:`script-point`, Position of the object in pixels.
    **width** : number, Width of the object in pixels.
    **height** : number, Height of the object in pixels.
    **size** : :ref:`script-size`, Size of the object in pixels.
    **rotation** : number, Rotation of the object in degrees clockwise.
    **visible** : bool, Whether the object is visible.
    **polygon** : :ref:`Polygon <script-polygon>`, Polygon of the object.
    **text** : string, The text of a text object.
    **font** : :ref:`script-font`, The font of a text object.
    **textAlignment** : :ref:`script-alignment`, The alignment of a text object.
    **wordWrap** : bool, Whether the text of a text object wraps based on the width of the object.
    **textColor** : color, Color of a text object.
    **tile** : :ref:`script-tile`, Tile of the object.
    **tileFlippedHorizontally** : bool, Whether the tile is flipped horizontally.
    **tileFlippedVertically** : bool, Whether the tile is flipped vertically.
    **selected** : bool, Whether the object is selected.
    **layer** : :ref:`script-objectgroup` |ro|, Layer this object is part of (or ``null`` in case of a standalone object).
    **map** : :ref:`script-map` |ro|, Map this object is part of (or ``null`` in case of a standalone object).

.. _script-mapobject-shape:

.. csv-table::
    :header: "MapObject.Shape"

    MapObject.Rectangle
    MapObject.Polygon
    MapObject.Polyline
    MapObject.Ellipse
    MapObject.Text
    MapObject.Point

Functions
~~~~~~~~~

new MapObject([name : string])
    Constructs a new map object, which can be added to an :ref:`script-objectgroup`.

.. _script-mapeditor:

MapEditor
^^^^^^^^^

Properties
~~~~~~~~~~

.. csv-table::
    :widths: 1, 2

    **currentBrush** : :ref:`script-map`, "Get or set the currently used tile brush."
    **currentMapView** : :ref:`script-mapview` |ro|, "Access the current map view."
    **tilesetsView** : :ref:`script-tilesetsview` |ro|, "Access the Tilesets view."

.. _script-mapformatwrapper:

MapFormat
^^^^^^^^^

This is an object that can read or write map files. (Since 1.4)

Inherits :ref:`script-fileformat`.

Functions
~~~~~~~~~

MapFormat.read(fileName : string) : :ref:`script-map`
    Read the given file as a map. This function will throw an error if reading
    is not supported.

MapFormat.write(map : :ref:`script-map`, fileName : string) : string
    Write the given map to a file. This function will throw an error if writing
    is not supported. If there is an error writing the file, it will return a
    description of the error; otherwise, it will return "".

.. _script-mapview:

MapView
^^^^^^^

The view displaying the map.

Properties
~~~~~~~~~~

.. csv-table::
    :widths: 1, 2

    **scale** : number, "Get or set the scale of the view."

Functions
~~~~~~~~~

MapView.centerOn(x : number, y : number) : void
    Centers the view at the given location in screen coordinates.

.. _script-object:

Object
^^^^^^

The base of most data types in Tiled. Provides the ability to associate custom
properties with the data.

Properties
~~~~~~~~~~

.. csv-table::
    :widths: 1, 2

    **asset** : :ref:`script-asset` |ro|, "The asset this object is part of, or ``null``."
    **readOnly** : bool |ro|, Whether the object is read-only.

Functions
~~~~~~~~~

.. _script-object-property:

Object.property(name : string) : variant
    Returns the value of the custom property with the given name, or
    ``undefined`` if no such property is set on the object.

    *Note:* Currently it is not possible to inspect the value of ``file`` properties.

.. _script-object-setProperty:

Object.setProperty(name : string, value : variant) : void
    Sets the value of the custom property with the given name. Supported types
    are ``bool``, ``number`` and ``string``. When setting a ``number``, the
    property type will be set to either ``int`` or ``float``, depending on
    whether it is a whole number.

    *Note:* Support for ``color`` and ``file`` properties is currently missing.

.. _script-object-properties:

Object.properties() : object
    Returns all custom properties set on this object. Modifications to the
    properties will not affect the original object.

.. _script-object-setProperties:

Object.setProperties(properties : object) : void
    Replaces all currently set custom properties with a new set of properties.

.. _script-object-removeProperty:

Object.removeProperty(name : string) : void
    Removes the custom property with the given name.

.. _script-objectgroup:

ObjectGroup
^^^^^^^^^^^

Inherits :ref:`script-layer`.

The "ObjectGroup" is a type of layer that can contain objects. It will
henceforth be referred to as a layer.

Properties
~~~~~~~~~~

.. csv-table::
    :widths: 1, 2

    **objects** : [:ref:`script-mapobject`] |ro|, Array of all objects on this layer.
    **objectCount** : int |ro|, Number of objects on this layer.
    **color** : color, Color of shape and point objects on this layer (when not set by object type).

Functions
~~~~~~~~~

new ObjectGroup([name : string])
    Constructs a new object layer, which can be added to a :ref:`script-map`.

ObjectGroup.objectAt(index : int) : :ref:`script-mapobject`
    Returns a reference to the object at the given index. When the object is
    removed, the reference turns into a standalone copy of the object.

ObjectGroup.removeObjectAt(index : int) : void
    Removes the object at the given index.

ObjectGroup.removeObject(object : :ref:`script-mapobject`) : void
    Removes the given object from this layer. The object reference turns into
    a standalone copy of the object.

ObjectGroup.insertObjectAt(index : int, object : :ref:`script-mapobject`) : void
    Inserts the object at the given index. The object can't already be part
    of a layer.

ObjectGroup.addObject(object : :ref:`script-mapobject`) : void
    Adds the given object to the layer. The object can't already be part of
    a layer.

.. _script-selectedarea:

SelectedArea
^^^^^^^^^^^^

Properties
~~~~~~~~~~

.. csv-table::
    :widths: 1, 2

    **boundingRect** : :ref:`script-rect` |ro|, Bounding rectangle of the selected area.

Functions
~~~~~~~~~

SelectedArea.get() : :ref:`script-region`
    Returns the selected region.

SelectedArea.set(rect : :ref:`script-rect`) : void
    Sets the selected area to the given rectangle.

SelectedArea.set(region : :ref:`script-region`) : void
    Sets the selected area to the given region.

SelectedArea.add(rect : :ref:`script-rect`) : void
    Adds the given rectangle to the selected area.

SelectedArea.add(region : :ref:`script-region`) : void
    Adds the given region to the selected area.

SelectedArea.subtract(rect : :ref:`script-rect`) : void
    Subtracts the given rectangle from the selected area.

SelectedArea.subtract(region : :ref:`script-region`) : void
    Subtracts the given region from the selected area.

SelectedArea.intersect(rect : :ref:`script-rect`) : void
    Sets the selected area to the intersection of the current selected area and the given rectangle.

SelectedArea.intersect(region : :ref:`script-region`) : void
    Sets the selected area to the intersection of the current selected area and the given region.

.. _script-terrain:

Terrain
^^^^^^^

Inherits :ref:`script-object`.

Properties
~~~~~~~~~~

.. csv-table::
    :widths: 1, 2

    **id** : int |ro|, ID of this terrain.
    **name** : string, Name of the terrain.
    **imageTile** : :ref:`script-tile`, The tile representing the terrain (needs to be from the same tileset).
    **tileset** : :ref:`script-tileset` |ro|, The tileset of the terrain.

.. _script-tile:

Tile
^^^^

Inherits :ref:`script-object`.

Properties
~~~~~~~~~~

.. csv-table::
    :widths: 1, 2

    **id** : int |ro|, ID of this tile within its tileset.
    **width** : int |ro|, Width of the tile in pixels.
    **height** : int |ro|, Height of the tile in pixels.
    **size** : :ref:`script-size` |ro|, Size of the tile in pixels.
    **type** : string, Type of the tile.
    **imageFileName** : string, File name of the tile image (when the tile is part of an image collection tileset).
    **terrain** : :ref:`script-tileterrains`, An object specifying the terrain at each corner of the tile.
    **probability** : number, Probability that the tile gets chosen relative to other tiles.
    **objectGroup** : :ref:`script-objectgroup`, The :ref:`script-objectgroup` associated with the tile in case collision shapes were defined. Returns ``null`` if no collision shapes were defined for this tile.
    **frames** : :ref:`[frame] <script-frames>`, This tile's animation as an array of frames.
    **animated** : bool |ro|, Indicates whether this tile is animated.
    **tileset** : :ref:`script-tileset` |ro|, The tileset of the tile.

.. _script-tile-flags:

.. csv-table::
    :header: "Tile.Flags"

    Tile.FlippedHorizontally
    Tile.FlippedVertically
    Tile.FlippedAntiDiagonally
    Tile.RotatedHexagonal120

.. _script-tile-corner:

.. csv-table::
    :header: "Tile.Corner"

    Tile.TopLeft
    Tile.TopRight
    Tile.BottomLeft
    Tile.BottomRight

Functions
~~~~~~~~~

Tile.terrainAtCorner(corner : :ref:`Corner <script-tile-corner>`) : :ref:`script-terrain`
    Returns the terrain used at the given corner.

Tile.setTerrainAtCorner(corner : :ref:`Corner <script-tile-corner>`, :ref:`script-terrain`) : void
    Sets the terrain used at the given corner.

.. _script-tilecollisioneditor:

TileCollisionEditor
^^^^^^^^^^^^^^^^^^^

Properties
~~~~~~~~~~

.. csv-table::
    :widths: 1, 2

    **selectedObjects** : [:ref:`script-mapobject`], Selected objects.
    **view** : [:ref:`script-mapview`], The map view used by the Collision Editor.

Functions
~~~~~~~~~

TileCollisionEditor.focusObject(object : :ref:`script-mapobject`) : void
    Focuses the given object in the collision editor view and makes sure its
    visible in its objects list. Does not automatically select the object.

.. _script-tilelayer:

TileLayer
^^^^^^^^^

Inherits :ref:`script-layer`.

Note that while tile layers have a size, the size is generally ignored on
infinite maps. Even for fixed size maps, nothing in the scripting API stops you
from changing the layer outside of its boundaries and changing the size of the
layer has no effect on its contents. If you want to change the size while
affecting the contents, use the ``resize`` function.

Properties
~~~~~~~~~~

.. csv-table::
    :widths: 1, 2

    **width** : int, Width of the layer in tiles (only relevant for non-infinite maps).
    **height** : int, Height of the layer in tiles (only relevant for non-infinite maps).
    **size** : :ref:`script-size`, Size of the layer in tiles (has ``width`` and ``height`` members) (only relevant for non-infinite maps).

Functions
~~~~~~~~~

new TileLayer([name : string])
    Constructs a new tile layer, which can be added to a :ref:`script-map`.

TileLayer.region() : :ref:`script-region`
    Returns the region of the layer that is covered with tiles.

TileLayer.resize(size : :ref:`script-size`, offset : :ref:`script-point`) : void
    Resizes the layer, erasing the part of the contents that falls outside of
    the layer's new size. The offset parameter can be used to shift the contents
    by a certain distance in tiles before applying the resize.

TileLayer.cellAt(x : int, y : int) : :ref:`script-cell`
    Returns the value of the cell at the given position. Can be used to query
    the flags and the tile ID, but does not currently allow getting a tile
    reference.

TileLayer.flagsAt(x : int, y : int) : int
    Returns the :ref:`flags <script-tile-flags>` used for the tile at the given
    position.

TileLayer.tileAt(x : int, y : int) : :ref:`script-tile`
    Returns the tile used at the given position, or ``null`` for empty spaces.

.. _script-tilelayer-edit:

TileLayer.edit() : :ref:`script-tilelayeredit`
    Returns an object that enables making modifications to the tile layer.

.. _script-tilelayeredit:

TileLayerEdit
^^^^^^^^^^^^^

This object enables modifying the tiles on a tile layer. Tile layers can't be
modified directly for reasons of efficiency. The :ref:`apply() <script-tilelayeredit-apply>`
function needs to be called when you're done making changes.

An instance of this object is created by calling :ref:`TileLayer.edit() <script-tilelayer-edit>`.

Properties
~~~~~~~~~~

.. csv-table::
    :widths: 1, 2

    **target** : :ref:`script-tilelayer` |ro|, The target layer of this edit object.
    **mergeable** : bool, "Whether applied edits are mergeable with previous edits. Starts out as ``false`` and is automatically set to ``true`` by :ref:`apply() <script-tilelayeredit-apply>`."

Functions
~~~~~~~~~

TileLayerEdit.setTile(x : int, y : int, tile : :ref:`script-tile` [, flags : int = 0]) : void
    Sets the tile at the given location, optionally specifying :ref:`tile flags <script-tile-flags>`.

.. _script-tilelayeredit-apply:

TileLayerEdit.apply() : void
    Applies all changes made through this object. This object can be reused to
    make further changes.

.. _script-map:

TileMap
^^^^^^^

Inherits :ref:`script-asset`.

Properties
~~~~~~~~~~

.. csv-table::
    :widths: 1, 2

    **width** : int, Width of the map in tiles (only relevant for non-infinite maps).
    **height** : int, Height of the map in tiles (only relevant for non-infinite maps).
    **size** : :ref:`script-size` |ro|, Size of the map in tiles (only relevant for non-infinite maps).
    **tileWidth** : int, Tile width (used by tile layers).
    **tileHeight**: int, Tile height (used by tile layers).
    **infinite** : bool, Whether this map is infinite.
    **hexSideLength** : int, Length of the side of a hexagonal tile (used by tile layers on hexagonal maps).
    **staggerAxis** : :ref:`StaggerAxis <script-map-staggeraxis>`, "For staggered and hexagonal maps, determines which axis (X or Y) is staggered."
    **orientation** : :ref:`Orientation <script-map-orientation>`, "General map orientation"
    **renderOrder** : :ref:`RenderOrder <script-map-renderorder>`, "Tile rendering order (only implemented for orthogonal maps)"
    **staggerIndex** : :ref:`StaggerIndex <script-map-staggerindex>`, "For staggered and hexagonal maps, determines whether the even or odd indexes along the staggered axis are shifted."
    **backgroundColor** : color, Background color of the map.
    **layerDataFormat** : :ref:`LayerDataFormat <script-map-layerdataformat>`, "The format in which the layer data is stored, taken into account by TMX, JSON and Lua map formats."
    **layerCount** : int |ro|, Number of top-level layers the map has.
    **tilesets** : [:ref:`script-tileset`], "The list of tilesets referenced by this map. To determine which tilesets are actually used, call :ref:`usedTilesets() <script-map-usedTilesets>`."
    **selectedArea** : :ref:`SelectionArea <script-selectedarea>`, The selected area of tiles.
    **currentLayer** : :ref:`script-layer`, The current layer.
    **selectedLayers** : [:ref:`script-layer`], Selected layers.
    **selectedObjects** : [:ref:`script-mapobject`], Selected objects.

.. _script-map-orientation:

.. csv-table::
    :header: "TileMap.Orientation"

    TileMap.Unknown
    TileMap.Orthogonal
    TileMap.Isometric
    TileMap.Staggered
    TileMap.Hexagonal

.. _script-map-layerdataformat:

.. csv-table::
    :header: "TileMap.LayerDataFormat"

    TileMap.XML
    TileMap.Base64
    TileMap.Base64Gzip
    TileMap.Base64Zlib
    TileMap.Base64Zstandard
    TileMap.CSV

.. _script-map-renderorder:

.. csv-table::
    :header: "TileMap.RenderOrder"

    TileMap.RightDown
    TileMap.RightUp
    TileMap.LeftDown
    TileMap.LeftUp

.. _script-map-staggeraxis:

.. csv-table::
    :header: "TileMap.StaggerAxis"

    TileMap.StaggerX
    TileMap.StaggerY

.. _script-map-staggerindex:

.. csv-table::
    :header: "TileMap.StaggerIndex"

    TileMap.StaggerOdd
    TileMap.StaggerEven

Functions
~~~~~~~~~

new TileMap()
    Constructs a new map.

.. _script-map-autoMap:

TileMap.autoMap([rulesFile : string]) : void
    Applies :doc:`/manual/automapping` using the given rules file, or using the
    default rules file is none is given.

    *This operation can only be applied to maps loaded from a file.*

TileMap.autoMap(region : :ref:`script-region` | :ref:`script-rect` [, rulesFile : string]) : void
    Applies :doc:`/manual/automapping` in the given region using the given
    rules file, or using the default rules file is none is given.

    *This operation can only be applied to maps loaded from a file.*

.. _script-map-setSize:

TileMap.setSize(width : int, height : int) : void
    Sets the size of the map in tiles. This does not affect the contents of the map.

    See also :ref:`resize <script-map-resize>`.

.. _script-map-setTileSize:

TileMap.setTileSize(width : int, height : int) : void
    Sets the tile size of the map in pixels. This affects the rendering of all tile layers.

.. _script-map-layerAt:

TileMap.layerAt(index : int) : :ref:`script-layer`
    Returns a reference to the top-level layer at the given index. When the
    layer gets removed from the map, the reference changes to a standalone
    copy of the layer.

.. _script-map-removeLayerAt:

TileMap.removeLayerAt(index : int) : void
    Removes the top-level layer at the given index. When a reference to the
    layer still exists, that reference becomes a standalone copy of the layer.

.. _script-map-removeLayer:

TileMap.removeLayer(layer : :ref:`script-layer`) : void
    Removes the given layer from the map. The reference to the layer becomes
    a standalone copy.

.. _script-map-insertLayerAt:

TileMap.insertLayerAt(index : int, layer : :ref:`script-layer`) : void
    Inserts the layer at the given index. The layer can't already be part of
    a map.

.. _script-map-addLayer:

TileMap.addLayer(layer : :ref:`script-layer`) : void
    Adds the layer to the map, above all existing layers. The layer can't
    already be part of a map.

.. _script-map-addTileset:

TileMap.addTileset(tileset : :ref:`script-tileset`) : bool
    Adds the given tileset to the list of tilesets referenced by this map.
    Returns ``true`` if the tileset was added, or ``false`` if the tileset was
    already referenced by this map.

.. _script-map-replaceTileset:

TileMap.replaceTileset(oldTileset : :ref:`script-tileset`, newTileset : :ref:`script-tileset`) : bool
    Replaces all occurrences of ``oldTileset`` with ``newTileset``. Returns
    ``true`` on success, or ``false`` when either the old tileset was not
    referenced by the map, or when the new tileset was already referenced by
    the map.

.. _script-map-removeTileset:

TileMap.removeTileset(tileset : :ref:`script-tileset`) : bool
    Removes the given tileset from the list of tilesets referenced by this
    map. Returns ``true`` on success, or ``false`` when the given tileset was
    not referenced by this map or when the tileset was still in use by a tile
    layer or tile object.

.. _script-map-usedTilesets:

TileMap.usedTilesets() : [:ref:`script-tileset`]
    Returns the list of tilesets actually used by this map. This is generally
    a subset of the tilesets referenced by the map (the ``TileMap.tilesets``
    property).

.. _script-map-merge:

TileMap.merge(map : :ref:`script-map` [, canJoin : bool = false]) : void
    Merges the tile layers in the given map with this one. If only a single
    tile layer exists in the given map, it will be merged with the
    ``currentLayer``.

    If ``canJoin`` is ``true``, the operation joins with the previous one on
    the undo stack when possible. Useful for reducing the amount of undo
    commands.

    *This operation can currently only be applied to maps loaded from a file.*

.. _script-map-resize:

TileMap.resize(size : :ref:`script-size` [, offset : :ref:`script-point` [, removeObjects : bool = false]]) : void
    Resizes the map to the given size, optionally applying an offset (in tiles).

    *This operation can currently only be applied to maps loaded from a file.*

    See also :ref:`setSize <script-map-setSize>`.

.. _script-map-screenToTile:

TileMap.screenToTile(x : number, y : number) : :ref:`script-point`
        Converts the given position from screen to tile coordinates.

.. _script-map-screenToTile2:

TileMap.screenToTile(position : :ref:`script-point`) : :ref:`script-point`
        Converts the given position from screen to tile coordinates.

.. _script-map-tileToScreen:

TileMap.tileToScreen(x : number, y : number) : :ref:`script-point`
        Converts the given position from tile to screen coordinates.

.. _script-map-tileToScreen2:

TileMap.tileToScreen(position : :ref:`script-point`) : :ref:`script-point`
        Converts the given position from tile to screen coordinates.

.. _script-map-screenToPixel:

TileMap.screenToPixel(x : number, y : number) : :ref:`script-point`
        Converts the given position from screen to pixel coordinates.

.. _script-map-screenToPixel2:

TileMap.screenToPixel(position : :ref:`script-point`) : :ref:`script-point`
        Converts the given position from screen to pixel coordinates.

.. _script-map-pixelToScreen:

TileMap.pixelToScreen(x : number, y : number) : :ref:`script-point`
        Converts the given position from pixel to screen coordinates.

.. _script-map-pixelToScreen2:

TileMap.pixelToScreen(position : :ref:`script-point`) : :ref:`script-point`
        Converts the given position from pixel to screen coordinates.

.. _script-map-pixelToTile:

TileMap.pixelToTile(x : number, y : number) : :ref:`script-point`
        Converts the given position from pixel to tile coordinates.

.. _script-map-pixelToTile2:

TileMap.pixelToTile(position : :ref:`script-point`) : :ref:`script-point`
        Converts the given position from pixel to tile coordinates.

.. _script-map-tileToPixel:

TileMap.tileToPixel(x : number, y : number) : :ref:`script-point`
        Converts the given position from tile to pixel coordinates.

.. _script-map-tileToPixel2:

TileMap.tileToPixel(position : :ref:`script-point`) : :ref:`script-point`
        Converts the given position from tile to pixel coordinates.

.. _script-tileset:

Tileset
^^^^^^^

Inherits :ref:`script-asset`.

Properties
~~~~~~~~~~

.. csv-table::
    :widths: 1, 2

    **name** : string, Name of the tileset.
    **image** : string, The file name of the image used by this tileset. Empty in case of image collection tilesets.
    **tiles**: [:ref:`script-tile`] |ro|, Array of all tiles in this tileset. Note that the index of a tile in this array does not always match with its ID.
    **terrains**: [:ref:`script-terrain`] |ro|, Array of all terrains in this tileset.
    **tileCount** : int, The number of tiles in this tileset.
    **nextTileId** : int, The ID of the next tile that would be added to this tileset. All existing tiles have IDs that are lower than this ID.
    **tileWidth** : int, Tile width for tiles in this tileset in pixels.
    **tileHeight** : int, Tile Height for tiles in this tileset in pixels.
    **tileSize** : :ref:`script-size`, Tile size for tiles in this tileset in pixels.
    **imageWidth** : int |ro|, Width of the tileset image in pixels.
    **imageHeight** : int |ro|, Height of the tileset image in pixels.
    **imageSize** : :ref:`script-size` |ro|, Size of the tileset image in pixels.
    **tileSpacing** : int |ro|, Spacing between tiles in this tileset in pixels.
    **margin** : int |ro|, Margin around the tileset in pixels (only used at the top and left sides of the tileset image).
    **objectAlignment** : :ref:`Alignment <script-tileset-alignment>`, "The alignment to use for tile objects (when ``Unspecified``, uses ``Bottom`` alignment on isometric maps and ``BottomLeft`` alignment for all other maps)."
    **tileOffset** : :ref:`script-point`, Offset in pixels that is applied when tiles from this tileset are rendered.
    **orientation** : :ref:`Orientation <script-tileset-orientation>`, The orientation of this tileset (used when rendering overlays and in the tile collision editor).
    **backgroundColor** : color, Background color for this tileset in the *Tilesets* view.
    **isCollection** : bool |ro|, Whether this tileset is a collection of images (same as checking whether ``image`` is an empty string).
    **selectedTiles** : [:ref:`script-tile`], Selected tiles (in the tileset editor).

.. _script-tileset-alignment:

.. csv-table::
    :header: "Tileset.Alignment"

    Tileset.Unspecified
    Tileset.TopLeft
    Tileset.Top
    Tileset.TopRight
    Tileset.Left
    Tileset.Center
    Tileset.Right
    Tileset.BottomLeft
    Tileset.Bottom
    Tileset.BottomRight

.. _script-tileset-orientation:

.. csv-table::
    :header: "Tileset.Orientation"

    Tileset.Orthogonal
    Tileset.Isometric

Functions
~~~~~~~~~

new Tileset([name : string])
    Constructs a new tileset.

Tileset.tile(id : int) : :ref:`script-tile`
    Returns a reference to the tile with the given ID. Raises an error if no
    such tile exists. When the tile gets removed from the tileset, the
    reference changes to a standalone copy of the tile.

    Note that the tiles in a tileset are only guaranteed to have consecutive
    IDs for tileset-image based tilesets. For image collection tilesets there
    will be gaps when tiles have been removed from the tileset.

Tileset.setTileSize(width : int, height : int) : void
    Sets the tile size for this tileset. If an image has been specified as well,
    the tileset will be (re)loaded. Can't be used on image collection tilesets.

Tileset.addTile() : :ref:`script-tile`
    Adds a new tile to this tileset and returns it. Only works for image collection tilesets.

Tileset.removeTiles(tiles : [:ref:`script-tile`]) : void
    Removes the given tiles from this tileset. Only works for image collection tilesets.

.. _script-tileseteditor:

TilesetEditor
^^^^^^^^^^^^^

Properties
~~~~~~~~~~

.. csv-table::
    :widths: 1, 2

    **collisionEditor** : :ref:`script-tilecollisioneditor`, "Access the collision editor within the tileset editor."

.. _script-tilesetformatwrapper:

TilesetFormat
^^^^^^^^^^^^^

This is an object that can read or write tileset files. (Since 1.4)

Inherits :ref:`script-fileformat`.

Functions
~~~~~~~~~

TilesetFormat.read(fileName : string) : :ref:`script-tileset`
    Read the given file as a tileset. This function will throw an error if reading
    is not supported.

TilesetFormat.write(tileset : :ref:`script-tileset`, fileName : string) : string
    Write the given tileset to a file. This function will throw an error if writing
    is not supported. If there is an error writing the file, it will return a
    description of the error; otherwise, it will return "".

.. _script-tilesetsview:

TilesetsView
^^^^^^^^^^^^

Properties
~~~~~~~~~~

.. csv-table::
    :widths: 1, 2

    **currentTileset** : :ref:`script-tileset`, "Access or change the currently displayed tileset."
    **selectedTiles** : [:ref:`script-tile`], "A list of the tiles that are selected in the current tileset."

.. _script-basic-types:

Basic Types
^^^^^^^^^^^

Some types are provided by the Qt Scripting Engine and others are added based
on the needs of the data types above. In the following the most important
ones are documented.

.. _script-alignment:

Alignment
~~~~~~~~~

.. csv-table::
    :header: "Qt.Alignment"
    :widths: 1, 2

    Qt.AlignLeft, 0x0001
    Qt.AlignRight, 0x0002
    Qt.AlignHCenter, 0x0004
    Qt.AlignJustify, 0x0008
    Qt.AlignTop, 0x0020
    Qt.AlignBottom, 0x0040
    Qt.AlignVCenter, 0x0080
    Qt.AlignCenter, Qt.AlignVCenter | Qt.AlignHCenter

.. _script-cell:

cell
~~~~

A cell on a :ref:`script-tilelayer`.

**Properties**

.. csv-table::
    :widths: 1, 2

    **tileId** : int, "The local tile ID of the tile, or -1 if the cell is empty."
    **empty** : bool, Whether the cell is empty.
    **flippedHorizontally** : bool, Whether the tile is flipped horizontally.
    **flippedVertically** : bool, Whether the tile is flipped vertically.
    **flippedAntiDiagonally** : bool, Whether the tile is flipped anti-diagonally.
    **rotatedHexagonal120** : bool, "Whether the tile is rotated by 120 degrees (for hexagonal maps, the anti-diagonal flip is interpreted as a 60-degree rotation)."

.. _script-filepath:

FilePath
~~~~~~~~

Used as the value for custom 'file' properties. Can be created with
:ref:`tiled.filePath <script-tiled-filePath>`.

.. csv-table::
    :widths: 1, 3

    **url** : url, The URL of the file.

.. _script-font:

Font
~~~~

.. csv-table::
    :widths: 1, 2

    **family** : string, The font family.
    **pixelSize** : int, Font size in pixels.
    **bold** : bool, Whether the font is bold.
    **italic** : bool, Whether the font is italic.
    **underline** : bool, Whether the text is underlined.
    **strikeOut** : bool, Whether the text is striked through.
    **kerning** : bool, Whether to use kerning when rendering the text.

.. _script-frames:

Frames
~~~~~~

An array of frames, which are objects with the following properties:

.. csv-table::
    :widths: 1, 2

    **tileId** : int, The local tile ID used to represent the frame.
    **duration** : int, Duration of the frame in milliseconds.

.. _script-objectref:

ObjectRef
~~~~~~~~~

The value of a property of type 'object', which refers to a
:ref:`script-mapobject` by its ID. Generally only used as a fallback when an
object property cannot be resolved to an actual object. Can be created with
:ref:`tiled.objectRef <script-tiled-objectRef>`.

.. csv-table::
    :widths: 1, 3

    **id** : int, The ID of the referenced object.

.. _script-point:

point
~~~~~

``Qt.point(x, y)`` can be used to create a point object.

**Properties**

.. csv-table::
    :widths: 1, 2

    **x** : number, X coordinate of the point.
    **y** : number, Y coordinate of the point.

.. _script-polygon:

Polygon
~~~~~~~

A polygon is not strictly a custom type. It is an array of objects that each
have an ``x`` and ``y`` property, representing the points of the polygon.

To modify the polygon of a :ref:`script-mapobject`, change or set up the
polygon array and then assign it to the object.

.. _script-rect:

rect
~~~~

``Qt.rect(x, y, width, height)`` can be used to create a rectangle.

**Properties**

.. csv-table::
    :widths: 1, 2

    **x** : int, X coordinate of the rectangle.
    **y** : int, Y coordinate of the rectangle.
    **width** : int, Width of the rectangle.
    **height** : int, Height of the rectangle.

.. _script-region:

region
~~~~~~

**Properties**

.. csv-table::
    :widths: 1, 2

    **boundingRect** : :ref:`script-rect` |ro|, Bounding rectangle of the region.

.. _script-size:

size
~~~~

``Qt.size(width, height)`` can be used to create a size object.

**Properties**

.. csv-table::
    :widths: 1, 2

    **width** : number, Width.
    **height** : number, Height.

.. _script-tileterrains:

Terrains
~~~~~~~~

An object specifying the terrain for each corner of a tile:

.. csv-table::

    **topLeft** : :ref:`script-terrain`
    **topRight** : :ref:`script-terrain`
    **bottomLeft** : :ref:`script-terrain`
    **bottomRight** : :ref:`script-terrain`

.. _script-textfile:

TextFile
~~~~~~~~

The TextFile object is used to read and write files in text mode.

When using ``TextFile.WriteOnly``, you need to call ``commit()`` when you're
done writing otherwise the operation will be aborted without effect.

**Properties**

.. csv-table::
    :widths: 1, 2

    **filePath** : string |ro|, "The path of the file."
    **atEof** : bool |ro|, "True if no more data can be read."
    **codec** : string, "The text codec."

.. csv-table::
    :header: "TextFile.OpenMode"
    :widths: 1, 2

    TextFile.ReadOnly, 0x0001
    TextFile.WriteOnly, 0x0002
    TextFile.ReadWrite, TextFile.ReadOnly | TextFile.WriteOnly
    TextFile.Append

**Functions**

new TextFile(fileName : string [, mode : OpenMode = ReadOnly])
    Opens a text file in the given mode.

TextFile.readLine() : string
    Reads one line of text from the file and returns it. The returned string
    does not contain the newline characters.

TextFile.readAll() : string
    Reads all data from the file and returns it.

TextFile.truncate() : void
    Truncates the file, that is, gives it the size of zero, removing all
    content.

TextFile.write(text : string) : void
    Writes a string to the file.

TextFile.writeLine(text : string) : void
    Writes a string to the file and appends a newline character.

TextFile.commit() : void
    Commits all written text to disk and closes the file. Should be called when
    writing to files in WriteOnly mode. Failing to call this function will
    result in cancelling the operation, unless safe writing to files is disabled.

TextFile.close() : void
    Closes the file. It is recommended to always call this function as soon as
    you are finished with the file.

.. _script-binaryfile:

BinaryFile
~~~~~~~~~~

The BinaryFile object is used to read and write files in binary mode.

When using ``BinaryFile.WriteOnly``, you need to call ``commit()`` when you're
done writing otherwise the operation will be aborted without effect.

**Properties**

.. csv-table::
    :widths: 1, 2

    **filePath** : string |ro|, "The path of the file."
    **atEof** : bool |ro|, "True if no more data can be read."
    **size** : number, "The size of the file (in bytes)."
    **pos** : number, "The position that data is written to or read from."

.. csv-table::
    :header: "BinaryFile.OpenMode"
    :widths: 1, 2

    BinaryFile.ReadOnly, 0x0001
    BinaryFile.WriteOnly, 0x0002
    BinaryFile.ReadWrite, BinaryFile.ReadOnly | BinaryFile.WriteOnly

**Functions**

new BinaryFile(filePath : string [, mode : OpenMode = ReadOnly])
    Opens a binary file in the given mode.

BinaryFile.resize(size : int) : void
    Sets the file size (in bytes). If size is larger than the file currently is,
    the new bytes will be set to 0; if size is smaller, the file is truncated.

BinaryFile.seek(pos : int) : void
    Sets the current position to *pos*.

BinaryFile.read(size : int) : ArrayBuffer
    Reads at most *size* bytes of data from the file and returns it as an
    ArrayBuffer.

BinaryFile.readAll() : ArrayBuffer
    Reads all data from the file and returns it as an ArrayBuffer.

BinaryFile.write(data : ArrayBuffer) : void
    Writes *data* into the file at the current position.

BinaryFile.commit() : void
    Commits all written data to disk and closes the file. Should be called when
    writing to files in WriteOnly mode. Failing to call this function will
    result in cancelling the operation, unless safe writing to files is
    disabled.

BinaryFile.close() : void
    Closes the file. It is recommended to always call this function as soon as
    you are finished with the file.

XmlFile
~~~~~~~

Inherits :ref:`script-xml-node`

**Functions**

new XmlFile(text : string)
    Creates a new Xml document by parsing `text`.

XmlFile.createNode(type : NodeType [, data : string = ""]) : XmlNode
    Creates a new node attached to this document. The new node is not in the element
    tree until it's been added somewhere. `data` is the initial value of XmlNode.text.
    `type` cannot be NodeType.Root or NodeType.Other.

XmlFile.writeToString(indent : int = 1) : string
    Returns this XML document represented as a string.

XmlNode
~~~~~~~~~~

**Properties**

.. csv-table::
    :widths: 1, 2

    **type** : NodeType |ro|, The type of this node
    **attributes** : object |ro|, A copy of the attributes on this Element
    **children** : [XmlNode] |ro|, A copy of the list of all the direct children of this Element or Root.
    **text** : string, "The meaning of this depends on the *type*. For an element, this is equivalent to the *tag*. For a text node, this is the plain text. For a CDATA node, it's the data as a string."
    **tag** : string, The tag name of this Element
    **parent** : XmlNode |ro|, The direct parent of this node
    **hasAttributes** : bool |ro|, True if this Element has at least one attribute
    **hasChildren** : bool |ro|, True if this Element or Root has at least one direct child node
    **root** : XmlFile |ro|, The XML file this node belongs to

.. csv-table::
    :header: XmlNode.NodeType
    :widths: 1, 2

    XmlNode.Element, A normal element in xml
    XmlNode.Text, Text inside an element or between elements
    XmlNode.CDATA, A `<![CDATA[ ... ]]>` section
    XmlNode.Root, The document itself
    XmlNode.Other, "Other xml constructs, like comments and processing commands"

**Functions**

XmlNode.clear() : void
    Deletes this node's list of children, removes itself from its parent, and it sets
    its type to NodeType.Unknown. The node cannot be reused after this operation.

XmlNode.attribute(name : string [, defaultValue : string = ""]) : string
    Gets the attribute with the given name. If the named attribute does not exist,
    it returns defaultValue instead. Throws an error if this node is not an Element.

XmlNode.setAttribute(name : string, newValue : string) : void
    Set the attribute with the given name to a new value, creating it if it
    didn't already exist. Throws an error if this node is not an Element.

XmlNode.hasAttribute(name : string) : bool
    Returns whether this node has an attribute with the given name. Throws an
    error if this node is not an Element.

XmlNode.removeAttribute(name : string) : void
    Removes the attribute with the given name from this element. Throws an error
    if this node is not an Element.

XmlNode.firstChild([tag : string]) : XmlNode
    Returns the first child with the given tag name. Returns `undefined` if the
    child can't be found or this node has no children.

XmlNode.lastChild([tag : string]) : XmlNode
    Returns the last child with the given tag name. Returns `undefined` if the
    child can't be found or this node has no children.

XmlNode.previousSibling([tag : string]) : XmlNode
    Returns the previous sibling of this node. Returns `undefined` if this is the
    first sibling or if there is no parent node.

XmlNode.nextSibling([tag : string]) : XmlNode
    Returns the next sibling of this node. Returns `undefined` if this is the last
    sibling or if there is no parent node.

XmlNode.appendChild(XmlNode newChild) : XmlNode
    Appends the child node to this one and returns it. Returns `undefined` if the
    operation failed. Note that the returned node might be different from newChild
    in some cases.

XmlNode.insertBefore(XmlNode newChild, XmlNode referenceChild) : XmlNode
    Puts the child node before the referenceChild node. Returns `undefined` if the
    operation failed. Note that the returned node might be different from newChild
    in some cases.

XmlNode.insertAfter(XmlNode newChild, XmlNode referenceChild) : XmlNode
    Puts the child node after the referenceChild node. Returns `undefined` if the
    operation failed. Note that the returned node might be different from newChild
    in some cases.

XmlNode.replaceChild(XmlNode newChild, XmlNode oldChild) : XmlNode
    Puts the child in the same spot as the old one, and returns the new child.
    Returns `undefined` if the operation failed. Note that the returned node might
    be different from newChild in some cases.

XmlNode.removeChild(XmlNode oldChild) : XmlNode
    Removes the child from this node, and returns it. Returns `undefined` if the
    operation failed.
