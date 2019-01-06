#ifndef BOTS_HPP
#define BOTS_HPP

#include <memory>
#include <QDialog>
#include <QUrl>
#include <QNetworkAccessManager>
#include <QStandardItemModel>
#include <QLayout>
#include <QTableView>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QRadioButton>
class MainWindow;
#include "def.hpp"
#include "asip.hpp"

class Bots : public QDialog {
  Q_OBJECT
public:
  explicit Bots(ASIP& session_,const QUrl& url_,MainWindow& mainWindow_);
private:
  void update();
  void parse(QString page);
  void createGame(const QModelIndex& index);
  void enterGame(const ASIP::GameInfo& game,const Side role,const Side viewpoint);
  void enable();
  Side getSide() const;
  virtual QSize	sizeHint() const override;

  ASIP& session;
  QNetworkAccessManager& networkAccessManager;
  MainWindow& mainWindow;
  const QUrl url;
  QStandardItemModel standardItemModel;
  unsigned int nameColumn,linkColumn;

  QVBoxLayout vBoxLayout;
    QHBoxLayout hBoxLayout;
      QPushButton create,refresh,closeButton;
      QCheckBox join;
    QHBoxLayout sideLayout;
      QLabel botSide;
      std::unique_ptr<QRadioButton> sideButtons[NUM_SIDES+1];
    QTableView tableView;
    QLabel label,message;
};

#endif // BOTS_HPP
