// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
/*******************************************************************************
 * Author    :  Damir Bakiev                                                    *
 * Version   :  na                                                              *
 * Date      :  March 25, 2023                                                  *
 * Website   :  na                                                              *
 * Copyright :  Damir Bakiev 2016-2023                                          *
 * License   :                                                                  *
 * Use, modification & distribution is subject to Boost Software License Ver 1. *
 * http://www.boost.org/LICENSE_1_0.txt                                         *
 *******************************************************************************/

#include "pocketraster_form.h"
#include "ui_pocketrasterform.h"

#include "graphicsview.h"
#include "settings.h"
#include <QMessageBox>

namespace PocketRaster {

Form::Form(GCode::Plugin* plugin, QWidget* parent)
    : GCode::BaseForm(plugin, new Creator, parent)
    , ui(new Ui::PocketRasterForm)
    , names {tr("Raster On"), tr("Raster Outside"), tr("Raster Inside")} {
    ui->setupUi(content);

    setWindowTitle(tr("Pocket Raster Toolpath"));

    MySettings settings;
    settings.beginGroup("PocketRasterForm");
    settings.getValue(ui->cbxPass);
    settings.getValue(ui->dsbxAcc);
    settings.getValue(ui->dsbxAngle);
    settings.getValue(ui->rbClimb);
    settings.getValue(ui->rbConventional);
    settings.getValue(ui->rbFast);
    settings.getValue(ui->rbInside);
    settings.getValue(ui->rbNormal);
    settings.getValue(ui->rbOutside);
    settings.endGroup();

    rb_clicked();

    connect(ui->rbClimb, &QRadioButton::clicked, this, &Form::rb_clicked);
    connect(ui->rbConventional, &QRadioButton::clicked, this, &Form::rb_clicked);
    connect(ui->rbInside, &QRadioButton::clicked, this, &Form::rb_clicked);
    connect(ui->rbOutside, &QRadioButton::clicked, this, &Form::rb_clicked);

    connect(ui->toolHolder1, &ToolSelectorForm::updateName, this, &Form::updateName);

    connect(leName, &QLineEdit::textChanged, this, &Form::onNameTextChanged);

    //
}

Form::~Form() {

    MySettings settings;
    settings.beginGroup("PocketRasterForm");
    settings.setValue(ui->cbxPass);
    settings.setValue(ui->dsbxAcc);
    settings.setValue(ui->dsbxAngle);
    settings.setValue(ui->rbClimb);
    settings.setValue(ui->rbConventional);
    settings.setValue(ui->rbFast);
    settings.setValue(ui->rbInside);
    settings.setValue(ui->rbNormal);
    settings.setValue(ui->rbOutside);
    settings.endGroup();
    delete ui;
}

void Form::сomputePaths() {
    const auto tool {ui->toolHolder1->tool()};

    if (!tool.isValid()) {
        tool.errorMessageBox(this);
        return;
    }

    Paths wPaths;
    Paths wRawPaths;
    AbstractFile const* file = nullptr;
    bool skip {true};

    for (auto* item : App::graphicsView()->selectedItems()) {
        GraphicsItem* gi = dynamic_cast<GraphicsItem*>(item);
        switch (item->type()) {
        case GiType::DataSolid:
        case GiType::DataPath:
            if (!file) {
                file = gi->file();
                boardSide = file->side();
            } else if (file != gi->file()) {
                if (skip) {
                    if ((skip = (QMessageBox::question(this, tr("Warning"), tr("Work items from different files!\nWould you like to continue?"), QMessageBox::Yes, QMessageBox::No) == QMessageBox::No)))
                        return;
                }
            }
            if (item->type() == GiType::DataSolid)
                wPaths.append(gi->paths());
            else
                wRawPaths.append(gi->paths());
            break;
        case GiType::ShCircle:
        case GiType::ShRectangle:
        case GiType::ShPolyLine:
        case GiType::ShCirArc:
        case GiType::ShText:
            wRawPaths.append(gi->paths());
            break;
        case GiType::DrillGi:
            wPaths.append(gi->paths());
            break;
        default:
            break;
        }
        addUsedGi(gi);
    }

    if (wRawPaths.empty() && wPaths.empty()) {
        QMessageBox::warning(this, tr("Warning"), tr("No selected items for working..."));
        return;
    }

    GCode::Params gcp_;
    gcp_.setConvent(ui->rbConventional->isChecked());
    gcp_.setSide(side);
    gcp_.tools.push_back(tool);

    gcp_.params[Creator::UseAngle] = ui->dsbxAngle->value();
    gcp_.params[GCode::Params::Depth] = dsbxDepth->value();
    gcp_.params[Creator::Pass] = ui->cbxPass->currentIndex();
    if (ui->rbFast->isChecked()) {
        gcp_.params[Creator::Fast] = true;
        gcp_.params[Creator::AccDistance] = (tool.feedRate_mm_s() * tool.feedRate_mm_s()) / (2 * ui->dsbxAcc->value());
    }

    creator->setGcp(gcp_);
    creator->addPaths(std::move(wPaths));
    creator->addRawPaths(wRawPaths);
    fileCount = 1;
    createToolpath();
}

void Form::updateName() {
    const auto& tool {ui->toolHolder1->tool()};
    if (tool.type() != Tool::Laser)
        ui->rbNormal->setChecked(true);
    ui->rbFast->setEnabled(tool.type() == Tool::Laser);

    leName->setText(names[side]);
}

void Form::updatePixmap() {
    ui->lblPixmap->setPixmap(QIcon::fromTheme(pixmaps[direction]).pixmap(QSize(150, 150)));
}

void Form::rb_clicked() {

    if (ui->rbOutside->isChecked())
        side = GCode::Outer;
    else if (ui->rbInside->isChecked())
        side = GCode::Inner;

    if (ui->rbClimb->isChecked())
        direction = GCode::Climb;
    else if (ui->rbConventional->isChecked())
        direction = GCode::Conventional;

    updateName();
    updateButtonIconSize();

    updatePixmap();
}

void Form::resizeEvent(QResizeEvent* event) {
    updatePixmap();
    QWidget::resizeEvent(event);
}

void Form::showEvent(QShowEvent* event) {
    updatePixmap();
    QWidget::showEvent(event);
}

void Form::onNameTextChanged(const QString& arg1) { fileName_ = arg1; }

void Form::editFile(GCode::File* /*file*/) { }

} // namespace PocketRaster

#include "moc_pocketraster_form.cpp"
