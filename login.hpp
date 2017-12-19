#ifndef LOGIN_HPP
#define LOGIN_HPP

#include <memory>
#include <QDialog>
#include <QFormLayout>
#include <QLabel>
#include <QRadioButton>
#include <QLineEdit>
#include <QDialogButtonBox>
class QNetworkAccessManager;
class QNetworkReply;
class MainWindow;
class ASIP;

class Login : public QDialog {
  Q_OBJECT
public:
  explicit Login(QNetworkAccessManager& networkAccessManager_,MainWindow& mainWindow_);
private:
  void loginAttempt(QNetworkReply& networkReply,ASIP& asip);

  QNetworkAccessManager& networkAccessManager;
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
