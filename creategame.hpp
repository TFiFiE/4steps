#ifndef CREATEGAME_HPP
#define CREATEGAME_HPP

#include <memory>
#include <QDialog>
#include <QCheckBox>
#include <QLabel>
#include <QDialogButtonBox>
class QNetworkReply;
class ASIP;
class Server;
#include "timecontrol.hpp"

class CreateGame : public QDialog {
  Q_OBJECT
public:
  explicit CreateGame(ASIP& session_,Server& server_);
  void creationAttempt(QNetworkReply& networkReply);
private:
  const Server& server;
  ASIP& session;

  QVBoxLayout vBoxLayout;
    QHBoxLayout hBoxLayout;
      QCheckBox timed,rated;
      QLabel yourSide;
      std::unique_ptr<QRadioButton> sideButtons[3];
      TimeControl timeControl;
    QDialogButtonBox dialogButtonBox;
};

#endif // CREATEGAME_HPP
