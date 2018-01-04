#include <QNetworkReply>
#include <QMessageBox>
#include "opengame.hpp"
#include "server.hpp"
#include "mainwindow.hpp"

using namespace std;
OpenGame::OpenGame(Server& server) :
  QDialog(&server),
  vBoxLayout(this),
  role("Your role:"),
  roleButtons{make_unique<QRadioButton>(tr("white (Gold)")),
              make_unique<QRadioButton>(tr("black (Silver)")),
              make_unique<QRadioButton>(tr("spectator"))},
  dialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel)
{
  setWindowTitle(tr("Open existing game"));

  formLayout.addRow(tr("Game id:"),&id);
  vBoxLayout.addLayout(&formLayout);

  hBoxLayout.addWidget(&role);
  hBoxLayout.addWidget(roleButtons[0].get());
  hBoxLayout.addWidget(roleButtons[1].get());
  hBoxLayout.addWidget(roleButtons[2].get());
  roleButtons[2]->setChecked(true);
  vBoxLayout.addLayout(&hBoxLayout);

  connect(&dialogButtonBox,&QDialogButtonBox::accepted,[&]{
    Side role;
    if (roleButtons[0]->isChecked())
      role=FIRST_SIDE;
    else if (roleButtons[1]->isChecked())
      role=SECOND_SIDE;
    else {
      assert(roleButtons[2]->isChecked());
      role=NO_SIDE;
    }
    const auto networkReply=server.session.enterGame(this,id.text(),role);
    connect(networkReply,&QNetworkReply::finished,[&,networkReply] {
      try {
        server.mainWindow.addGame(server.session.getGame(*networkReply),role==NO_SIDE ? FIRST_SIDE : role);
        close();
      }
      catch (const std::exception& exception) {
        QMessageBox::critical(this,tr("Error opening game"),exception.what());
      }
    });
  });
  connect(&dialogButtonBox,&QDialogButtonBox::rejected,this,&OpenGame::close);
  vBoxLayout.addWidget(&dialogButtonBox);
}
