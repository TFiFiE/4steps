#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include <memory>
#include <QMainWindow>
#include <QAction>
#include <QLabel>
class QSvgRenderer;
class QNetworkAccessManager;
class ASIP;
class Server;
#include "def.hpp"

class MainWindow : public QMainWindow {
  Q_OBJECT
public:
  explicit MainWindow(QSvgRenderer pieceIcons_[NUM_PIECE_SIDE_COMBINATIONS],QNetworkAccessManager& networkAccessManager_,QWidget* const parent=nullptr);
  void login();
  void addServer(ASIP& asip);
  void closeTab(const int& index);
  void addGame(std::unique_ptr<ASIP> game,const Side viewpoint,const bool guaranteedUnique=false);

  QSvgRenderer* const pieceIcons;
  QTabWidget tabWidget;
  std::vector<Server*> servers;
private:
  QNetworkAccessManager& networkAccessManager;
  QAction emptyGame,logIn,quit;
  QLabel buildTime;
  std::vector<std::weak_ptr<ASIP> > games;
};

#endif // MAINWINDOW_HPP
