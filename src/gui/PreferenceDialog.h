/*
 * PreferenceDialog.h
 *
 * (c) 2014 Sofian Audry -- info(@)sofianaudry(.)com
 * (c) 2014 Alexandre Quessy -- alexandre(@)quessy(.)net
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

#ifndef PREFERENCEDIALOG_H_
#define PREFERENCEDIALOG_H_

#include <QDialog>
#include <QNetworkInterface>

#include "MainWindow.h"

namespace mmp {

class PreferenceDialog : public QDialog
{
  Q_OBJECT

public:
  PreferenceDialog(QWidget* parent = 0);

  ~PreferenceDialog();

  void showDialog();
  void saveSettings() { applySettings(); }

protected:
  void closeEvent(QCloseEvent* event);
  bool loadSettings();

signals:
  void closed();
  void settingSaved();

private slots:
  void accept();
  void reject();
  int exec();

  void applySettings();
  void refreshCurrentIP();

private:
  // Reset port number
  void updatePortNumber();
  // Create categories pages
  void createInterfacePage();
  void createMappingPage();
  void createOutputPage();
  void createControlsPage();
  void createAdvancedPage();

  // Create List
  void createPreferencesList();

  // Create pages widgets
  QWidget *_interfacePage;
  QWidget *_mappingPage;
  QWidget *_outputPage;
  QTabWidget *_controlsPage;
  QTabWidget *_advancedPage;

  // Interface widgets
  QComboBox *_languageBox;
  void createLanguageList();
  QComboBox *_toolbarIconSizeBox;

  // Shape widgets
  // Sticky vertices
  QCheckBox *_stickyVerticesBox;
  QComboBox *_stickyRadiusBox;

  // Output widgets
  QCheckBox *_showResolutionBox;
  QRadioButton *_classicRadio;
  QRadioButton *_palTestRadio;
  QRadioButton *_ntscTestRadio;
  QList<QRadioButton *> _radioGroup;
  QLabel *_classicTestImg;
  QLabel *_palTestImg;
  QLabel *_ntscTestImg;
  QCheckBox *_showControlOnOverBox;

  // Controls widgets
  // OSC
  QWidget *_oscWidget;
//  QCheckBox *_sendMessageBox;
  QCheckBox *_listenMessageBox;
  QLineEdit *_sendAddressEdit;
  QSpinBox *_sendPortNumber;
  QLabel *_machineAddressLabel;
  QSpinBox *_listenPortNumber;
  QPushButton *_ipRefreshButton;
  QCheckBox *_oscSameMediaSourceBox;

  // Advanced widgets
  // Playback
  QWidget *_playbackWidget;
  QCheckBox *_playInLoopBox;


  // Common widgets
  QListWidget *_listWidget;
  QStackedLayout *_stackedLayout;
  QDialogButtonBox* _buttonBox;

  // Constantes
  static const int PREFERENCES_WINDOW_WIDTH = 720;
  static const int PREFERENCES_WINDOW_HEIGHT = 540;
  static const int LIST_ITEM_HEIGHT = 36;
};

}

#endif /* PREFERENCESDIALOG_H_ */
