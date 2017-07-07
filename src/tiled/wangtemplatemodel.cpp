/*
 * wangtemplatemodel.cpp
 * Copyright 2017, Benjamin Trotter <bdtrotte@ucsc.edu>
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

#include "wangtemplatemodel.h"

#include <QtMath>

using namespace Tiled;
using namespace Internal;

WangTemplateModel::WangTemplateModel(WangSet *wangSet, QObject *parent)
    : QAbstractListModel(parent)
    , mWangSet(wangSet)
{
}

int WangTemplateModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    if (mWangSet) {
        int rows = mWangSet->edgeColors() * mWangSet->cornerColors();
        rows *= rows;
        rows *= rows;

        return rows;
    } else {
        return 0;
    }
}

int WangTemplateModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return 1;
}

QVariant WangTemplateModel::data(const QModelIndex &index, int role) const
{
    if (role == WangIdRole)
        return QVariant::fromValue(wangIdAt(index));

    return QVariant();
}

WangId WangTemplateModel::wangIdAt(const QModelIndex &index) const
{
    if (!index.isValid())
        return 0;

    const int idIndex = index.row();

    if (WangSet *set = wangSet())
        if (idIndex < rowCount())
            return set->templateWangIdAt(idIndex);

    return 0;
}

QModelIndex WangTemplateModel::wangIdIndex(WangId wangId) const
{
    if (!mWangSet)
        return QModelIndex();

    Q_ASSERT(mWangSet->wangIdIsValid(wangId));

    int edges = mWangSet->edgeColors();
    int corners = mWangSet->cornerColors();

    //Only wangIds with all edges/corners assigned are valid here
    if (edges > 1)
        Q_ASSERT(!wangId.hasEdgeWildCards());
    if (edges > 1)
        Q_ASSERT(!wangId.hasCornerWildCards());

    int row = 0;
    int cornerEdgePermutations = edges * corners;

    for (int i = 0; i < 8; ++i) {
        int bellowPermutations = qPow(cornerEdgePermutations, i/2) * ((i&1)? edges : 1);
        if (i&1)
            row += wangId.cornerColor(i/2) * bellowPermutations;
        else
            row += wangId.edgeColor(i/2) * bellowPermutations;
    }

    return index(row, 0);
}

void WangTemplateModel::setWangSet(WangSet *wangSet)
{
    mWangSet = wangSet;

    resetModel();
}

void WangTemplateModel::resetModel()
{
    beginResetModel();
    endResetModel();
}

void WangTemplateModel::wangSetChanged()
{
    resetModel();
}
