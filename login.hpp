#ifndef LOGIN_HPP
#define LOGIN_HPP

#include <memory>
class QNetworkReply;
#include <QDialog>
#include <QFormLayout>
#include <QLabel>
#include <QRadioButton>
#include <QLineEdit>
#include <QDialogButtonBox>
struct Globals;
class MainWindow;
class ASIP;

class Login : public QDialog {
  Q_OBJECT
public:
  explicit Login(Globals& globals_,MainWindow& mainWindow_);
private:
  void loginAttempt(QNetworkReply& networkReply,ASIP& asip);

  static const char* defaultGamerooms[2];

  Globals& globals;
  MainWindow& mainWindow;

  QVBoxLayout vBoxLayout;
    QFormLayout formLayout;
      QGridLayout protocol;
        QLabel asip;
        std::array<std::unique_ptr<QRadioButton>,2> protocolButtons;
        int selectedProtocolButton;
      QLineEdit gameroom,username,password;
    QDialogButtonBox dialogButtonBox;
};

#endif // LOGIN_HPP
