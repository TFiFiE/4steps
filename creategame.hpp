#ifndef CREATEGAME_HPP
#define CREATEGAME_HPP

#include <memory>
class QNetworkReply;
#include <QDialog>
#include <QCheckBox>
#include <QLabel>
#include <QDialogButtonBox>
struct Globals;
class ASIP;
class Server;
#include "timecontrol.hpp"
#include "def.hpp"

class CreateGame : public QDialog {
  Q_OBJECT
public:
  explicit CreateGame(Globals& globals_,ASIP& session_,Server& server_);
  void creationAttempt(QNetworkReply& networkReply);
private:
  Globals& globals;
  const Server& server;
  ASIP& session;

  QVBoxLayout vBoxLayout;
    QHBoxLayout hBoxLayout;
      QCheckBox timed,rated;
      QLabel yourSide;
      std::unique_ptr<QRadioButton> sideButtons[NUM_SIDES+1];
      TimeControl timeControl;
    QDialogButtonBox dialogButtonBox;
};

#endif // CREATEGAME_HPP
