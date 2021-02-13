#include <QMenuBar>
#include <QFileDialog>
#include <QDesktopServices>
#include "mainwindow.hpp"
#include "globals.hpp"
#include "game.hpp"
#include "login.hpp"
#include "puzzles.hpp"
#include "server.hpp"

MainWindow::MainWindow(Globals& globals_,QWidget* const parent) :
  QMainWindow(parent),
  globals(globals_),
  emptyGame(tr("Edit &new game")),
  customGame(tr("Set up &custom position")),
  logIn(tr("&Log in")),
  quit(tr("&Quit")),
  chat(tr("Enter &chat (Discord server)")),
  buildTime(tr("Built: ")+__DATE__+QString(" ")+__TIME__)
{
  setWindowTitle(tr("4steps - Arimaa client"));
  const auto menu=menuBar()->addMenu(tr("&Game"));

  emptyGame.setShortcut(QKeySequence::New);
  connect(&emptyGame,&QAction::triggered,this,[this]{(new Game(globals,FIRST_SIDE,this))->show();});
  menu->addAction(&emptyGame);

  customGame.setShortcut(QKeySequence(Qt::CTRL+Qt::Key_C));
  using namespace std;
  connect(&customGame,&QAction::triggered,this,[this]{(new Game(globals,FIRST_SIDE,this,nullptr,make_unique<TurnState>()))->show();});
  menu->addAction(&customGame);

  logIn.setShortcut(QKeySequence(Qt::CTRL+Qt::Key_L));
  connect(&logIn,&QAction::triggered,this,[this]{openDialog(new Login(globals,*this));});
  menu->addAction(&logIn);

  quit.setShortcuts(QKeySequence::Quit);
  connect(&quit,&QAction::triggered,this,&MainWindow::close);
  menu->addAction(&quit);

  const auto puzzles=menuBar()->addAction(tr("&Puzzles"));
  connect(puzzles,&QAction::triggered,this,[this] {
    globals.settings.beginGroup("Puzzles");
    QString fileName=globals.settings.value("filename").toString();
    globals.settings.endGroup();
    fileName=QFileDialog::getOpenFileName(this,tr("Select puzzle file"),fileName);
    if (!fileName.isNull()) {
      try {
        new Puzzles(globals,fileName.toStdString(),this);
        globals.settings.beginGroup("Puzzles");
        globals.settings.setValue("filename",fileName);
        globals.settings.endGroup();
      } catch (...) {}
    }
  });

  const auto about=menuBar()->addMenu(tr("&About"));
  connect(&chat,&QAction::triggered,this,[]{QDesktopServices::openUrl(QUrl("https://discord.gg/YCH3FSp"));});
  about->addAction(&chat);

  setCentralWidget(&tabWidget);
  tabWidget.setTabsClosable(true);
  connect(&tabWidget,&QTabWidget::tabCloseRequested,this,&MainWindow::closeTab);
  menuBar()->setCornerWidget(&buildTime);
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

ASIP* MainWindow::addGame(std::unique_ptr<ASIP> newGame,const Side viewpoint,const bool guaranteedUnique)
{
  for (auto game=games.begin();game!=games.end();) {
    if (game->expired())
      game=games.erase(game);
    else
      ++game;
  }

  if (!guaranteedUnique)
    for (const auto& existingGame:games)
      if (auto existingGame_=existingGame.lock())
        if (existingGame_->isEqualGame(*newGame)) {
          (new Game(globals,viewpoint,this,existingGame_))->show();
          return nullptr;
        }
  newGame->sit();
  const auto addedGame=std::shared_ptr<ASIP>(newGame.release(),[](ASIP* const session) {
    session->leave();
    session->deleteLater();
  });
  games.push_back(addedGame);
  (new Game(globals,viewpoint,this,addedGame))->show();
  return addedGame.get();
}
