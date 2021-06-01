.. raw:: html

   <div class="new new-prev">Since Tiled 1.3</div>

.. |ro| replace:: *[read‑only]*

Scripting
=========

Introduction
------------

Tiled can be extended with the use of JavaScript. See the `Tiled Scripting
API`_ for a reference of all available functionality.

TypeScript definitions of the API are available as the `@mapeditor/tiled-api`_
NPM package, which can provide auto-completion in your editor. The API
reference is generated based on these definitions.

On startup, Tiled will execute any script files present in :ref:`extension
folders <script-extensions>`. In addition it is possible to run scripts
directly from :ref:`the console <script-console>`. All scripts share a single
JavaScript context.

.. note::

    A few example scripts and links to existing Tiled extensions are provided
    at the Tiled Extensions repository: https://github.com/mapeditor/tiled-extensions

.. note::

    The full scripting API and support for ECMAScript 7 features are only
    available for Tiled builds based on Qt 5.12 or later. This currently
    excludes the Windows XP and snap releases.

.. _script-extensions:

Scripted Extensions
^^^^^^^^^^^^^^^^^^^

Extensions can be placed in a system-specific or :doc:`project-specific
</manual/projects>` location.

The system-specific folder can be opened from the Plugins tab in the
:doc:`Preferences dialog </manual/preferences>`. The usual location on each
supported platform is as follows:

+-------------+-----------------------------------------------------------------+
| **Windows** | | :file:`C:/Users/<USER>/AppData/Local/Tiled/extensions/`       |
+-------------+-----------------------------------------------------------------+
| **macOS**   | | :file:`~/Library/Preferences/Tiled/extensions/`               |
+-------------+-----------------------------------------------------------------+
| **Linux**   | | :file:`~/.config/tiled/extensions/`                           |
+-------------+-----------------------------------------------------------------+

The project-specific folder defaults to "extensions", relative to the
directory of the ``.tiled-project`` file, but this can be changed in the
*Project Properties*.

.. warning::

    Since Tiled 1.7, project-specific extensions are only enabled by default
    for projects you created. When opening any other project, a popup will
    notify you when the project has a scripted extensions directory, allowing
    you to enable extensions for that project.

    Always be careful when enabling extensions on projects you haven't
    created, since extensions have access to your files and can execute
    processes.

An extension can be placed either directly in an extensions directory, or in a
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

API Reference
-------------

See the `Tiled Scripting API`_.

The following global variable is currently not documented in the generated
documentation, since it conflicts with nodejs types:

__filename
    The file path of the current file being evaluated. Only available during
    initial evaluation of the file and not when later functions in that file
    get called. If you need it there, copy the value to local scope.

.. _Tiled Scripting API: https://www.mapeditor.org/docs/scripting/
.. _@mapeditor/tiled-api: https://www.npmjs.com/package/@mapeditor/tiled-api
