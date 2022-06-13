// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/********************************************************************************
 * Author    :  Damir Bakiev                                                    *
 * Version   :  na                                                              *
 * Date      :  01 February 2020                                                *
 * Website   :  na                                                              *
 * Copyright :  Damir Bakiev 2016-2022                                          *
 * License:                                                                     *
 * Use, modification & distribution is subject to Boost Software License Ver 1. *
 * http://www.boost.org/LICENSE_1_0.txt                                         *
 *******************************************************************************/
#include "gi_drill.h"

#include "scene.h"
#include <graphicsview.h>

#include <QPainter>
#include <QStyleOptionGraphicsItem>

using namespace ClipperLib;

GiDrill::GiDrill(const Path& path, double diameter, FileInterface* file, int toolId)
    : GraphicsItem { file }
    , diameter_ { diameter }
    , path_ { path }
    , toolId_ { toolId } {
    setAcceptHoverEvents(true);
    setFlag(ItemIsSelectable, true);
    setToolId(toolId_);
    create();
    changeColor();
}

void GiDrill::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) {

    if (App::scene()->drawPdf()) {
        painter->setBrush(Qt::black);
        painter->setPen(Qt::NoPen);
        painter->drawPath(shape_);
        return;
    }

    painter->setBrush(bodyColor_);
    painter->setPen(Qt::NoPen);
    painter->drawPolygon(shape_.toFillPolygon());

    pen_.setColor(pathColor_);
    painter->strokePath(shape_, pen_);
}

bool GiDrill::isSlot() { return path_.size() > 1; }

void GiDrill::setDiameter(double diameter) {
    if (diameter_ == diameter)
        return;
    diameter_ = diameter;

    create();
    update();
}

void GiDrill::update(const Path& path, double diameter) {
    diameter_ = diameter;
    path_ = path;
    create();
    update();
}

Paths GiDrill::paths(int alternate) const {
    return { transform().map(path_) };
}

void GiDrill::changeColor() {
    animation.setStartValue(bodyColor_);

    switch (colorState) {
    case Default:
        bodyColor_ = QColor(100, 100, 100);
        break;
    case Hovered:
        bodyColor_ = QColor(150, 0x0, 150);
        break;
    case Selected:
        bodyColor_ = QColor(255, 0x0, 255);
        break;
    case Hovered | Selected:
        bodyColor_ = QColor(127, 0x0, 255);
        break;
    }

    pathColor_ = bodyColor_;
    switch (colorState) {
    case Default:
        break;
    case Hovered:
        break;
    case Selected:
        pathColor_ = Qt::white;
        break;
    case Hovered | Selected:
        pathColor_ = Qt::white;
        break;
    }

    animation.setEndValue(bodyColor_);
    animation.start();
}

void GiDrill::create() {
    shape_ = QPainterPath();

    switch (path_.size()) {
    case 0:
        break;
    case 1: {
        path_ = CirclePath(diameter_ * uScale, path_.front());
        ReversePath(path_);
        path_.push_back(path_.front());
        shape_.addPolygon(path_);
    } break;
    case 2:
    default: {
        rect_ = shape_.boundingRect();
        Paths paths;
        ClipperOffset offset;
        offset.AddPath(path_, jtRound, etOpenRound);
        offset.Execute(paths, diameter_ * 0.5 * uScale);
        for (Path& path : paths) {
            path.push_back(path.front());
            shape_.addPolygon(path);
        }
    }
    }

    rect_ = shape_.boundingRect();
    fillPolygon = shape_.toFillPolygon();
}