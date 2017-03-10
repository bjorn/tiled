/*
 * commandmanager.h
 * Copyright 2017, Ketan Gupta <ketan19972010@gmail.com>
 *
 * This file is part of Tiled.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <QObject>

class QAction;
class QMenu;

namespace Tiled {
namespace Internal {

class CommandDataModel;

class CommandManager : public QObject
{
    Q_OBJECT

public:

    static CommandManager *instance();

    static void deleteInstance();

    /**
     * Returns the CommandDataModel instance stored
     */
    CommandDataModel *commandDataModel();

    /**
     * Registers a new QMenu with the CommandManager
     */
    void registerMenu(QMenu* menu);

public slots:

    /**
     * Populates the menu pointed by menu
     */
    void populateMenu();

    /**
     * Displays the dialog to edit the commands
     */
    void showDialog();

private:
    Q_DISABLE_COPY(CommandManager);

    CommandManager();

    /**
     * Updates mActions QList
     */
    void updateActions();

    static CommandManager *mInstance;
    CommandDataModel *mModel;
    QList<QMenu*> mMenus;
    QList<QAction*> mActions;
};

} // namespace Internal
} // namespace Tiled
