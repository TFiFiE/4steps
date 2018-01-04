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

  Globals& globals;
  MainWindow& mainWindow;

  QVBoxLayout vBoxLayout;
    QFormLayout formLayout;
      QGridLayout protocol;
        QLabel asip;
        std::unique_ptr<QRadioButton> protocolButtons[2];
      QLineEdit gameroom,username,password;
    QDialogButtonBox dialogButtonBox;
};

#endif // LOGIN_HPP
