#include <QNetworkReply>
#include "opengame.hpp"
#include "server.hpp"
#include "messagebox.hpp"

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
    server.session.enterGame(this,id.text(),role,[=,&server](QNetworkReply* const networkReply) {
      connect(networkReply,&QNetworkReply::finished,&server,[=,&server] {
        try {
          server.addGame(*networkReply,role==NO_SIDE ? FIRST_SIDE : role);
          close();
        }
        catch (const std::exception& exception) {
          MessageBox(QMessageBox::Critical,tr("Error opening game"),exception.what(),QMessageBox::NoButton,this).exec();
        }
      });
    });
  });
  connect(&dialogButtonBox,&QDialogButtonBox::rejected,this,&OpenGame::close);
  vBoxLayout.addWidget(&dialogButtonBox);
}
