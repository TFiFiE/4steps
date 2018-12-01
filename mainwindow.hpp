#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include <memory>
#include <QMainWindow>
#include <QAction>
#include <QLabel>
struct Globals;
class ASIP;
class Server;
#include "def.hpp"

class MainWindow : public QMainWindow {
  Q_OBJECT
public:
  explicit MainWindow(Globals& globals_,QWidget* const parent=nullptr);
  void addServer(ASIP& asip);
  void closeTab(const int& index);
  ASIP* addGame(std::unique_ptr<ASIP> game,const Side viewpoint,const bool guaranteedUnique=false);

  QTabWidget tabWidget;
  std::vector<Server*> servers;
private:
  Globals& globals;
  QAction emptyGame,logIn,quit;
  QLabel buildTime;
  std::vector<std::weak_ptr<ASIP> > games;
};

#endif // MAINWINDOW_HPP
