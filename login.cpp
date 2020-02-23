#include <QNetworkReply>
#include "login.hpp"
#include "globals.hpp"
#include "mainwindow.hpp"
#include "server.hpp"
#include "asip1.hpp"
#include "asip2.hpp"
#include "arimaa_com.hpp"
#include "messagebox.hpp"

const char* Login::defaultGamerooms[]={"http://arimaa.com/arimaa/gameroom/client1gr.cgi",
                                       "http://arimaa.com/arimaa/gameroom/client2gr.cgi"};

using namespace std;
Login::Login(Globals& globals_,MainWindow& mainWindow_) :
  QDialog(&mainWindow_),
  globals(globals_),
  mainWindow(mainWindow_),
  vBoxLayout(this),
  asip(tr("Arimaa Server Interface Protocol")),
  protocolButtons{make_unique<QRadioButton>(tr("1.0")),
                  make_unique<QRadioButton>(tr("2.0"))},
  selectedProtocolButton(-1),
  dialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel)
{
  setWindowTitle(tr("Log in"));

  protocol.addWidget(&asip,0,0,1,2);

  for (unsigned int protocolButtonIndex=0;protocolButtonIndex<protocolButtons.size();++protocolButtonIndex) {
    const auto protocolButton=protocolButtons[protocolButtonIndex].get();
    protocol.addWidget(protocolButton);
    connect(protocolButton,&QRadioButton::clicked,[this,protocolButtonIndex]{
      if (gameroom.text().isEmpty() || (selectedProtocolButton>=0 && gameroom.text()==defaultGamerooms[selectedProtocolButton]))
        gameroom.setText(defaultGamerooms[protocolButtonIndex]);
      selectedProtocolButton=protocolButtonIndex;
    });
  }
  protocolButtons[1]->click();

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
    const auto gameroomHost=QUrl(gameroomURL).host();
    const auto usernameString=username.text();
    for (const auto server:mainWindow.servers) {
      const ASIP& asip=server->session;
      if (gameroomHost==asip.serverURL().host() && usernameString==asip.username()) {
        mainWindow.tabWidget.setCurrentWidget(server);
        close();
        return;
      }
    }
    setEnabled(false);
    ASIP* asip;
    const bool arimaa_com=(gameroomHost=="arimaa.com");
    if (protocolButtons[0]->isChecked()) {
      if (arimaa_com)
        asip=new Arimaa_com<ASIP1>(globals.networkAccessManager,gameroomURL,this);
      else
        asip=new ASIP1(globals.networkAccessManager,gameroomURL,this);
    }
    else {
      assert(protocolButtons[1]->isChecked());
      if (arimaa_com)
        asip=new Arimaa_com<ASIP2>(globals.networkAccessManager,gameroomURL,this);
      else
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
    MessageBox(QMessageBox::Critical,tr("Login error"),exception.what(),QMessageBox::NoButton,this).exec();
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
