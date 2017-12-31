#include <QMenuBar>
#include "mainwindow.hpp"
#include "globals.hpp"
#include "game.hpp"
#include "login.hpp"
#include "server.hpp"

MainWindow::MainWindow(Globals& globals_,QWidget* const parent) :
  QMainWindow(parent),
  globals(globals_),
  emptyGame(tr("Edit &new game")),
  logIn(tr("&Log in")),
  quit(tr("&Quit")),
  buildTime(tr("Built: ")+__DATE__+QString(" ")+__TIME__)
{
  setWindowTitle(tr("4steps - Arimaa client"));
  const auto menu=menuBar()->addMenu(tr("&Game"));

  emptyGame.setShortcut(QKeySequence::New);
  connect(&emptyGame,&QAction::triggered,this,[=]{new Game(globals,FIRST_SIDE,this);});
  menu->addAction(&emptyGame);

  logIn.setShortcut(QKeySequence(Qt::CTRL+Qt::Key_L));
  connect(&logIn,&QAction::triggered,this,&MainWindow::login);
  menu->addAction(&logIn);

  quit.setShortcuts(QKeySequence::Quit);
  connect(&quit,&QAction::triggered,this,&MainWindow::close);
  menu->addAction(&quit);

  setCentralWidget(&tabWidget);
  tabWidget.setTabsClosable(true);
  connect(&tabWidget,&QTabWidget::tabCloseRequested,this,&MainWindow::closeTab);
  menuBar()->setCornerWidget(&buildTime);
}

void MainWindow::login()
{
  const auto login=new Login(globals,*this);
  login->setAttribute(Qt::WA_DeleteOnClose);
  login->show();
}

void MainWindow::addServer(ASIP& asip)
{
  const auto serverPage=new Server(globals,asip,*this);
  tabWidget.setCurrentIndex(tabWidget.addTab(serverPage,asip.serverURL().host()));
  servers.emplace_back(serverPage);
}

void MainWindow::closeTab(const int& index)
{
  const auto tabPage=tabWidget.widget(index);
  tabWidget.removeTab(index);
  tabPage->deleteLater();
  servers.erase(servers.begin()+index);
}

void MainWindow::addGame(std::unique_ptr<ASIP> newGame,const Side viewpoint,const bool guaranteedUnique)
{
  for (auto game=games.begin();game!=games.end();) {
    if (game->expired())
      game=games.erase(game);
    else
      ++game;
  }

  ASIP& newGame_=*newGame.get();
  if (!guaranteedUnique)
    for (const auto existingGame:games)
      if (auto existingGame_=existingGame.lock())
        if (existingGame_->isEqualGame(newGame_)) {
          new Game(globals,viewpoint,this,existingGame_);
          return;
        }
  newGame_.sit();
  const auto addedGame=std::shared_ptr<ASIP>(newGame.release(),[this](ASIP* const session) {
    session->leave();
    session->deleteLater();
  });
  games.push_back(addedGame);
  new Game(globals,viewpoint,this,addedGame);
}
