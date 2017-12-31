#include <QNetworkReply>
#include <QMessageBox>
#include "login.hpp"
#include "globals.hpp"
#include "mainwindow.hpp"
#include "server.hpp"
#include "asip1.hpp"
#include "asip2.hpp"

using namespace std;
Login::Login(Globals& globals_,MainWindow& mainWindow_) :
  QDialog(&mainWindow_),
  globals(globals_),
  mainWindow(mainWindow_),
  vBoxLayout(this),
  asip(tr("Arimaa Server Interface Protocol")),
  protocolButtons{make_unique<QRadioButton>(tr("1.0")),
                  make_unique<QRadioButton>(tr("2.0"))},
  gameroom("http://arimaa.com/arimaa/gameroom/client2gr.cgi"),
  dialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel)
{
  setWindowTitle(tr("Log in"));

  protocol.addWidget(&asip,0,0,1,2);
  protocol.addWidget(protocolButtons[0].get());
  protocol.addWidget(protocolButtons[1].get());
  protocolButtons[1]->setChecked(true);
  formLayout.addRow(tr("Protocol:"),&protocol);

  connect(&gameroom,&QLineEdit::textChanged,[=]{
    const auto margins=gameroom.textMargins();
    gameroom.setMinimumWidth(textWidth(gameroom)+margins.left()+margins.right());
  });
  gameroom.textChanged(QString());
  formLayout.addRow(tr("Game room:"),&gameroom);

  globals.settings.beginGroup("Login");
  username.setText(globals.settings.value("username").toString());
  globals.settings.endGroup();
  formLayout.addRow(tr("Username:"),&username);

  password.setEchoMode(QLineEdit::Password);
  formLayout.addRow(tr("Password:"),&password);

  vBoxLayout.addLayout(&formLayout);
  if (username.text().isEmpty())
    username.setFocus();
  else
    password.setFocus();
  connect(&dialogButtonBox,&QDialogButtonBox::accepted,[&]{
    const auto gameroomURL=gameroom.text();
    const auto usernameString=username.text();
    for (const auto server:mainWindow.servers) {
      const ASIP& asip=server->session;
      if (QUrl(gameroomURL).host()==asip.serverURL().host() && usernameString==asip.username()) {
        mainWindow.tabWidget.setCurrentWidget(server);
        close();
        return;
      }
    }
    setEnabled(false);
    ASIP* asip;
    if (protocolButtons[0]->isChecked())
      asip=new ASIP1(globals.networkAccessManager,gameroomURL,this);
    else {
      assert(protocolButtons[1]->isChecked());
      asip=new ASIP2(globals.networkAccessManager,gameroomURL,this);
    }
    const auto networkReply=asip->login(this,usernameString,password.text());
    connect(networkReply,&QNetworkReply::finished,this,[=]{loginAttempt(*networkReply,*asip);});
  });
  connect(&dialogButtonBox,&QDialogButtonBox::rejected,this,&Login::close);
  vBoxLayout.addWidget(&dialogButtonBox);
}

void Login::loginAttempt(QNetworkReply& networkReply,ASIP& asip)
{
  try {
    asip.processReply(networkReply);
    mainWindow.addServer(asip);
    close();

    globals.settings.beginGroup("Login");
    globals.settings.setValue("username",username.text());
    globals.settings.endGroup();
  }
  catch (const std::exception& exception) {
    asip.deleteLater();
    QMessageBox::critical(this,tr("Login error"),exception.what());
    setEnabled(true);
    const QString message(exception.what());
    if (message.contains("Password",Qt::CaseInsensitive)) {
      password.setFocus();
      password.selectAll();
    }
    else if (message.contains("Username",Qt::CaseInsensitive))
      username.setFocus();
    else
      gameroom.setFocus();
  }
}
