#include <QHeaderView>
#include <QNetworkReply>
#include <QMessageBox>
#include <QMenu>
#include "gamelist.hpp"
#include "server.hpp"

GameList::GameList(Server* const server,const QString& labelText) :
  description(labelText)
{
  addWidget(&description,0,1,Qt::AlignCenter);
  addWidget(&lastUpdated,0,2,Qt::AlignRight);

  QStringList m_TableHeader;
  m_TableHeader<<tr("Gold")<<tr("Silver")<<tr("Time control");
  tableWidget.setColumnCount(3);
  tableWidget.setHorizontalHeaderLabels(m_TableHeader);
  tableWidget.horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
  tableWidget.horizontalHeader()->setStretchLastSection(true);
  tableWidget.horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
  tableWidget.verticalHeader()->setVisible(false);
  tableWidget.setShowGrid(false);
  tableWidget.setEditTriggers(QAbstractItemView::NoEditTriggers);
  tableWidget.setFocusPolicy(Qt::NoFocus);
  tableWidget.setSelectionMode(QAbstractItemView::NoSelection);
  const int height=tableWidget.horizontalHeader()->height()+2;
  tableWidget.setMaximumHeight(height);
  tableWidget.setCursor(Qt::PointingHandCursor);
  addWidget(&tableWidget,1,0,3,0,Qt::AlignTop);
  connect(&tableWidget,&QTableWidget::itemClicked,this,[=](const QTableWidgetItem* item){
    const auto& game=games[item->row()];
    const auto column=item->column();
    Side role,viewpoint;
    if (column>1) {
      role=NO_SIDE;
      viewpoint=FIRST_SIDE;
    }
    else {
      viewpoint=(column==1 ? SECOND_SIDE : FIRST_SIDE);
      const auto playerName=game.players[viewpoint];
      if (playerName.isEmpty() || playerName==server->session.username())
        role=viewpoint;
      else
        role=NO_SIDE;
    }
    const auto networkReply=server->session.enterGame(this,game.id,role);
    connect(networkReply,&QNetworkReply::finished,this,[=] {
      try {
        server->addGame(*networkReply,viewpoint);
        if (role!=NO_SIDE && game.players[role].isEmpty())
          server->refreshPage();
      }
      catch (const std::exception& exception) {
        QMessageBox::critical(server,tr("Error opening game"),exception.what());
      }
    });
  });
  tableWidget.setContextMenuPolicy(Qt::CustomContextMenu);
  connect(&tableWidget,&QTableWidget::customContextMenuRequested,this,[=](const QPoint pos){
    QModelIndex index=tableWidget.indexAt(pos);
    const auto& game=games[index.row()];

    bool emptySeat=false;
    for (const auto player:game.players) {
      if (player.isEmpty())
        emptySeat=true;
      else if (player!=server->session.username())
        return;
    }
    if (!emptySeat)
      return;

    auto menu=new QMenu(server);
    menu->setAttribute(Qt::WA_DeleteOnClose);
    const auto cancelGame=new QAction("Cancel",menu);
    connect(cancelGame,&QAction::triggered,this,[=]{
      const auto networkReply=server->session.cancelGame(this,game.id);
      connect(networkReply,&QNetworkReply::finished,this,[=] {
        try {
          server->session.processReply(*networkReply);
        }
        catch (const std::exception& exception) {
          #if 0 // SERVER BUG: Cancelling with ASIP 2.0 triggers error.
          QMessageBox::critical(server,tr("Error cancelling game"),exception.what());
          #endif
        }
        server->refreshPage();
      });
    });
    menu->addAction(cancelGame);
    menu->popup(tableWidget.viewport()->mapToGlobal(pos));
  });
}

void GameList::receiveGames(std::vector<ASIP::GameInfo> games_)
{
  games.swap(games_);

  tableWidget.setRowCount(games.size());
  unsigned int gameIndex=0;
  for (const auto& game:games) {
    for (Side side=FIRST_SIDE;side<NUM_SIDES;increment(side)) {
      const auto& playerName=game.players[side];
      auto playerItem=new QTableWidgetItem(playerName);
      playerItem->setTextAlignment(Qt::AlignCenter);
      if (playerName.isEmpty()) {
        playerItem->setText("join");
        auto font=playerItem->font();
        font.setItalic(true);
        font.setBold(true);
        playerItem->setFont(font);
      }
      tableWidget.setItem(gameIndex,side==FIRST_SIDE ? 0 : 1,playerItem);
    }
    auto timeControl=new QTableWidgetItem(game.timeControl);
    timeControl->setTextAlignment(Qt::AlignCenter);
    timeControl->setFlags(timeControl->flags()&~Qt::ItemIsSelectable);
    tableWidget.setItem(gameIndex,2,timeControl);
    ++gameIndex;
  }

  int height=tableWidget.horizontalHeader()->height()+2;
  for (int i=0;i<tableWidget.rowCount();++i)
     height+=tableWidget.rowHeight(i);
  tableWidget.setMaximumHeight(height);
  lastUpdated.setText(tr("Last updated: ")+QDateTime::currentDateTime().toString("HH:mm:ss"));
}
