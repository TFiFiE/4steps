#include <QNetworkReply>
#include "creategame.hpp"
#include "globals.hpp"
#include "server.hpp"
#include "messagebox.hpp"

using namespace std;
CreateGame::CreateGame(Globals& globals_,ASIP& session_,Server& server_) :
  QDialog(&server_),
  globals(globals_),
  server(server_),
  session(session_),
  vBoxLayout(this),
  timed(tr("timed")),
  rated(tr("rated")),
  yourSide(tr("Your side:")),
  sideButtons{make_unique<QRadioButton>(tr("white (Gold)")),
              make_unique<QRadioButton>(tr("black (Silver)")),
              make_unique<QRadioButton>(tr("random"))},
  timeControl(tr("Time control")),
  dialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel)
{
  setWindowTitle(tr("Create new game"));

  timed.setChecked(true);
  hBoxLayout.addWidget(&timed);
  rated.setChecked(true);
  hBoxLayout.addWidget(&rated);
  connect(&timed,&QCheckBox::toggled,[=](bool checked) {
    timeControl.setEnabled(checked);
    rated.setEnabled(checked);
  });
  hBoxLayout.addWidget(&yourSide);
  hBoxLayout.addWidget(sideButtons[0].get());
  hBoxLayout.addWidget(sideButtons[1].get());
  hBoxLayout.addWidget(sideButtons[2].get());
  sideButtons[2]->setChecked(true);
  vBoxLayout.addLayout(&hBoxLayout);

  vBoxLayout.addWidget(&timeControl);

  connect(&dialogButtonBox,&QDialogButtonBox::accepted,[&]{
    setEnabled(false);
    Side side;
    if (sideButtons[0]->isChecked())
      side=FIRST_SIDE;
    else if (sideButtons[1]->isChecked())
      side=SECOND_SIDE;
    else {
      assert(sideButtons[2]->isChecked());
      side=(rand()%2==0 ? FIRST_SIDE : SECOND_SIDE);
    }
    const auto networkReply=session.createGame(this,timeControl.toString(timed.isChecked()),timed.isChecked() && rated.isChecked(),side);
    connect(networkReply,&QNetworkReply::finished,this,[=]{creationAttempt(*networkReply);});
  });
  connect(&dialogButtonBox,&QDialogButtonBox::rejected,this,&CreateGame::close);
  vBoxLayout.addWidget(&dialogButtonBox);
  timeControl.readSettings(globals.settings);
  timeControl.moveTime.seconds.setFocus();
}

void CreateGame::creationAttempt(QNetworkReply& networkReply)
{
  try {
    server.addGame(networkReply,session.role(),true);
    server.refreshPage();
    close();
    timeControl.writeSettings(globals.settings);
  }
  catch (const std::exception& exception) {
    MessageBox(QMessageBox::Critical,tr("Error creating game"),exception.what(),QMessageBox::NoButton,this).exec();
    setEnabled(true);
  }
}
