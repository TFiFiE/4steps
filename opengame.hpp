#ifndef OPENGAME_HPP
#define OPENGAME_HPP

#include <memory>
#include <QDialog>
#include <QFormLayout>
#include <QLineEdit>
#include <QLabel>
#include <QRadioButton>
#include <QDialogButtonBox>
class Server;
#include "def.hpp"

class OpenGame : public QDialog {
  Q_OBJECT
public:
  OpenGame(Server& server);
private:
  QVBoxLayout vBoxLayout;
    QFormLayout formLayout;
      QLineEdit id;
    QHBoxLayout hBoxLayout;
      QLabel role;
      std::unique_ptr<QRadioButton> roleButtons[NUM_SIDES+1];
    QDialogButtonBox dialogButtonBox;
};

#endif // OPENGAME_HPP
