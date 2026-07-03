/*
 * WelcomeDialog.cpp
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "WelcomeDialog.h"

#include <QtWidgets>
#include <QDesktopServices>

#include "MM.h"

namespace mmp {

namespace {
const char* kShowKey = "showWelcomeOnStartup";
}

WelcomeDialog::WelcomeDialog(QWidget* parent) : QDialog(parent)
{
  setWindowTitle(tr("Welcome to %1").arg(MM::APPLICATION_NAME));

  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->setContentsMargins(24, 24, 24, 24);
  layout->setSpacing(12);

  QLabel* titleImage = new QLabel;
  titleImage->setPixmap(QPixmap(":/mapmap-title-light").scaledToWidth(240, Qt::SmoothTransformation));
  titleImage->setAlignment(Qt::AlignCenter);
  layout->addWidget(titleImage);

  QLabel* text = new QLabel(tr(
      "MapMap projects video and images onto real surfaces.\n\n"
      "To get started, import a media file to create a paint, then add a shape "
      "to project it onto."));
  text->setWordWrap(true);
  text->setAlignment(Qt::AlignCenter);
  layout->addWidget(text);

  QPushButton* importButton = new QPushButton(tr("Import a media file…"));
  importButton->setDefault(true);
  layout->addWidget(importButton);

  QPushButton* docButton = new QPushButton(tr("Read the documentation"));
  layout->addWidget(docButton);

  _showOnStartupBox = new QCheckBox(tr("Show this window at startup"));
  _showOnStartupBox->setChecked(showOnStartup());
  layout->addWidget(_showOnStartupBox);

  QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Close);
  layout->addWidget(buttons);

  connect(importButton, &QAbstractButton::clicked, this, [this]() {
    emit importMediaRequested();
    accept();
  });
  connect(docButton, &QAbstractButton::clicked, this, []() {
    QDesktopServices::openUrl(QUrl(MM::WEBSITE_URL));
  });
  connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::accept);
  connect(_showOnStartupBox, &QCheckBox::toggled, this, [](bool on) {
    QSettings().setValue(kShowKey, on);
  });
}

bool WelcomeDialog::showOnStartup()
{
  return QSettings().value(kShowKey, true).toBool();
}

}
