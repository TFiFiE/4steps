#include <QNetworkReply>
#include <QHeaderView>
#include "server.hpp"
#include "globals.hpp"
#include "gamelist.hpp"
#include "creategame.hpp"
#include "opengame.hpp"
#include "messagebox.hpp"
#include "mainwindow.hpp"

Server::Server(Globals& globals_,ASIP& session_,MainWindow& mainWindow_) :
  session(session_),
  mainWindow(mainWindow_),
  globals(globals_),
  vBoxLayout(this),
  newGame(tr("&New game")),
  openGame(tr("&Open game")),
  refresh(tr("&Refresh page"))
{
  session.setParent(this);

  const QString gameListCategoryTitles[]={tr("My games"),tr("Invited games"),tr("Open games"),tr("Live games"),tr("Recent games")};
  for (const auto gameListCategory:session.availableGameListCategories()) {
    using namespace std;
    gameLists.insert(make_pair(gameListCategory,make_unique<GameList>(this,gameListCategoryTitles[gameListCategory])));
  }
  connect(&newGame,&QPushButton::clicked,this,[&]{openDialog(new CreateGame(globals,session,*this));});
  connect(&openGame,&QPushButton::clicked,this,[&]{openDialog(new OpenGame(*this));});
  connect(&refresh,&QPushButton::clicked,this,&Server::refreshPage);
  hBoxLayout.addWidget(&newGame);
  hBoxLayout.addWidget(&openGame);
  hBoxLayout.addWidget(&refresh);
  vBoxLayout.addLayout(&hBoxLayout);
  connect(&session,&ASIP::sendGameList,this,[&](const ASIP::GameListCategory gameListCategory,const std::vector<ASIP::GameInfo>& games) {
    const auto gameList=gameLists.find(gameListCategory);
    if (gameList!=gameLists.end())
      gameList->second->receiveGames(games);
  });
  connect(&session,&ASIP::childStatusChanged,this,[this](const ASIP::Status oldStatus,const ASIP::Status newStatus) {
    if (oldStatus==ASIP::UNSTARTED && newStatus==ASIP::LIVE)
      return;
    refreshPage();
  });
  refreshPage();
  for (auto& gameList:gameLists)
    vBoxLayout.addLayout(gameList.second.get());
}

Server::~Server()
{
  QNetworkReply* const networkReply=session.logout(nullptr);
  if (networkReply!=nullptr)
    connect(networkReply,&QNetworkReply::finished,networkReply,&QNetworkReply::deleteLater);
}

void Server::refreshPage() const
{
  session.state();
}

void Server::enterGame(const ASIP::GameInfo& game,const Side role,const Side viewpoint)
{
  session.enterGame(this,game.id,role,[=](QNetworkReply* const networkReply) {
    connect(networkReply,&QNetworkReply::finished,this,[=] {
      try {
        addGame(*networkReply,viewpoint);
        if (role!=NO_SIDE && game.players[role].isEmpty())
          refreshPage();
      }
      catch (const std::exception& exception) {
        MessageBox(QMessageBox::Critical,tr("Error opening game"),exception.what(),QMessageBox::NoButton,this).exec();
        refreshPage();
      }
    });
  });
}

void Server::addGame(QNetworkReply& networkReply,const Side viewpoint,const bool guaranteedUnique) const
{
  mainWindow.addGame(session.getGame(networkReply),viewpoint,guaranteedUnique);
}

void Server::resizeEvent(QResizeEvent*)
{
  for (const auto& gameList:gameLists) {
    gameList.second.get()->tableWidget.horizontalHeader()->setStretchLastSection(false);
    gameList.second.get()->tableWidget.horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    gameList.second.get()->tableWidget.horizontalHeader()->setStretchLastSection(true);
    gameList.second.get()->tableWidget.horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
  }
}
