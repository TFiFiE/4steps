#include <QNetworkReply>
#include <QHeaderView>
#include "server.hpp"
#include "globals.hpp"
#include "gamelist.hpp"
#include "creategame.hpp"
#include "opengame.hpp"
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

void Server::addGame(QNetworkReply& networkReply,const Side viewpoint,const bool guaranteedUnique) const
{
  const auto game=mainWindow.addGame(session.getGame(networkReply),viewpoint,guaranteedUnique);
  if (game!=nullptr) // game not already added
    connect(game,&ASIP::statusChanged,this,[this](const ASIP::Status oldStatus,const ASIP::Status newStatus) {
      if (oldStatus==ASIP::UNSTARTED && newStatus==ASIP::LIVE)
        return;
      refreshPage();
    });
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
