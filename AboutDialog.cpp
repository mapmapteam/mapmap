/*
 * AboutDialog.cpp
 *
 * (c) 2016 Dame Diongue -- baydamd(@)gmail(.)com
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

#include "AboutDialog.h"

#include <QtWidgets>

namespace mmp {

AboutDialog::AboutDialog(QWidget *parent) : QDialog(parent)
{
  // Defines fixed size
  resize(ABOUT_WINDOW_WIDTH, ABOUT_WINDOW_HEIGHT);
  setMinimumSize(ABOUT_WINDOW_WIDTH, ABOUT_WINDOW_HEIGHT);

  // Set title
  setWindowTitle(tr("About %1").arg(MM::APPLICATION_NAME));
  // Set Layout
  _mainLayout = new QGridLayout;
  setLayout(_mainLayout);

  // Set icon label
  _iconLabel = new QLabel;
  _iconLabel->setPixmap(QPixmap(":/mapmap-logo").scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));
  _iconLabel->setContentsMargins(0, 20, 0, 20);
  _mainLayout->addWidget(_iconLabel, 0, 0, 1, 1, Qt::AlignRight);
  // Set title label
  _textLabel = new QLabel;
  _textLabel->setPixmap(QPixmap(":/mapmap-title-light").scaled(200, 40, Qt::KeepAspectRatio, Qt::SmoothTransformation));
  _mainLayout->addWidget(_textLabel, 0, 2);
  // Set version label
  _versionLabel = new QLabel;
  _versionLabel->setText(QString("<h2> %1 </h2>").arg(MM::VERSION));
  _versionLabel->setContentsMargins(7, 20, 0, 0);
  _mainLayout->addWidget(_versionLabel, 0, 3);

  // Add tab widget
  _tabWidget = new QTabWidget;
  _mainLayout->addWidget(_tabWidget, 1, 0, 1, 4);

  // Close button
  _buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
  connect(_buttonBox, SIGNAL(rejected()), this, SLOT(close()));
  _mainLayout->addWidget(_buttonBox, 2, 0, 1, 4);

  // Create and fill different tabs
  createAboutTab();
  // Changelog  Tab
  createChangelogTab();
  // Libraries
  createLibrariesTab();
  // Contributors
  createContributorsTab();
  // License
  createLicenseTab();

  createOscTab();
}

void AboutDialog::createAboutTab()
{
  _aboutTextBrowser = new QTextBrowser;
  _aboutTextBrowser->setOpenExternalLinks(true);

  // Software description
  QString aboutSoftwareText = "<p>" + tr("MapMap is a free/open source video mapping software.") + "</p>";
  // Copyright and software owners
  QString copyrightText = "<p>" + tr("Copyright &copy; 2013 %1.").arg(MM::COPYRIGHT_OWNERS) + "</p>";
  // License short notice
  QFile licenseShortFile(":/license-short");
  licenseShortFile.open(QIODevice::ReadOnly | QIODevice::Text);
  QString licenseNoticeText = Qt::convertFromPlainText(QTextCodec::codecForName("UTF-8")->toUnicode(licenseShortFile.readAll()), Qt::WhiteSpaceNormal);
  // About projection mapping
  QFile aboutMappingFile(":/projection-mapping");
  aboutMappingFile.open(QIODevice::ReadOnly | QIODevice::Text);
  QString aboutMappingText = QTextCodec::codecForName("UTF-8")->toUnicode(aboutMappingFile.readAll());
  // Visit our website for more information
  QString projectWebsiteText = "<p>" + tr("See the ") + QString("<a href=\"%1\">").arg(MM::ORGANIZATION_DOMAIN) +
                                                 tr("%1 website").arg(MM::APPLICATION_NAME) + "</a> for more information on this software.</p>";
  // Append texts
  QString aboutText;
  aboutText.append(aboutSoftwareText);
  aboutText.append(copyrightText);
  aboutText.append(licenseNoticeText);
  aboutText.append(aboutMappingText);
  aboutText.append(projectWebsiteText);

  // Set about text
  _aboutTextBrowser->setText(aboutText);

  _tabWidget->addTab(_aboutTextBrowser, tr("About"));
}

void AboutDialog::createChangelogTab()
{
  _changelogTextBrowser = new QTextBrowser;
  _changelogTextBrowser->setOpenExternalLinks(true);

  QFile changelogFile(":/changelog");
  changelogFile.open(QIODevice::ReadOnly | QIODevice::Text);
  _changelogTextBrowser->setText(QTextCodec::codecForName("UTF-8")->toUnicode(changelogFile.readAll()));

  _tabWidget->addTab(_changelogTextBrowser, tr("Changelog"));
}

void AboutDialog::createLibrariesTab()
{
  _librariesTextBrowser = new QTextBrowser;
  _librariesTextBrowser->setOpenExternalLinks(true);

  QString qtVersionText = QString("<h4>Qt %1</h4>").arg(QT_VERSION_STR);
  QString gstreamerVersionText = QString("<h4>%1</h4>").arg(gst_version_string());
  //QString libloVersionText = QString("<h4>%1</h4>").arg(lo_version('str', 0, 0, 0, 0, 0, 0, 0, 0));

  QString librairiesText;
  librairiesText.append(qtVersionText);
  librairiesText.append(gstreamerVersionText);
  //librairiesText.append(libloVersionText);

  // Set librairies main text
  _librariesTextBrowser->setText(librairiesText);
  _tabWidget->addTab(_librariesTextBrowser, tr("Libraries"));
}

void AboutDialog::createContributorsTab()
{
  _contributorsTextBrowser = new QTextBrowser;
  _contributorsTextBrowser->setOpenExternalLinks(true);

  QFile contributorsFile(":/contributors");
  contributorsFile.open(QIODevice::ReadOnly | QIODevice::Text);
  _contributorsTextBrowser->setText(QTextCodec::codecForName("UTF-8")->toUnicode(contributorsFile.readAll()));

  _tabWidget->addTab(_contributorsTextBrowser, tr("Contributors"));
}

void AboutDialog::createLicenseTab()
{
  _licenseTextBrowser = new QTextBrowser;
  _licenseTextBrowser->setOpenExternalLinks(true);

  QFile licenseFile(":/license");
  licenseFile.open(QIODevice::ReadOnly | QIODevice::Text);
  _licenseTextBrowser->setText(QTextCodec::codecForName("UTF-8")->toUnicode(licenseFile.readAll()));

  _tabWidget->addTab(_licenseTextBrowser, tr("License"));
}

void AboutDialog::createOscTab()
{

  QTextBrowser *
  oscBrowser = new QTextBrowser;
  oscBrowser->setOpenExternalLinks(true);
  QFile oscFile(":/osc_html");
  oscFile.open(QIODevice::ReadOnly | QIODevice::Text);
  oscBrowser->setText(QTextCodec::codecForName("UTF-8")->toUnicode(oscFile.readAll()));
  _tabWidget->addTab(oscBrowser, tr("OSC"));
}

}
