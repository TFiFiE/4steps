#include <QHeaderView>
#include <QNetworkReply>
#include <QMenu>
#include "gamelist.hpp"
#include "server.hpp"
#include "io.hpp"
#include "messagebox.hpp"

GameList::GameList(Server* const server,const QString& labelText) :
  description(labelText)
{
  addWidget(&description,0,1,Qt::AlignCenter);
  addWidget(&lastUpdated,0,2,Qt::AlignRight);

  QStringList m_TableHeader;
  m_TableHeader<<sideWord(FIRST_SIDE)<<sideWord(SECOND_SIDE)<<tr("Time control");
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
    const auto& self=server->session.username();
    const auto& players=game.players;

    Side role=NO_SIDE;
    const Side selectedSide=(column<NUM_SIDES ? static_cast<Side>(column) : NO_SIDE);
    if (selectedSide==NO_SIDE) {
      for (Side side=FIRST_SIDE;side<NUM_SIDES;increment(side))
        if (self==players[side]) {
          role=side;
          break;
        }
    }
    else {
      const auto& playerName=players[selectedSide];
      if (playerName.isEmpty() || self==playerName)
        role=selectedSide;
      else {
        const Side nonselectedSide=otherSide(selectedSide);
        if (self==players[nonselectedSide])
          role=nonselectedSide;
      }
    }

    Side viewpoint;
    if (selectedSide!=NO_SIDE)
      viewpoint=selectedSide;
    else if (role==NO_SIDE)
      viewpoint=FIRST_SIDE;
    else
      viewpoint=role;

    server->enterGame(game,role,viewpoint);
  });
  tableWidget.setContextMenuPolicy(Qt::CustomContextMenu);
  connect(&tableWidget,&QTableWidget::customContextMenuRequested,this,[=](const QPoint pos){
    QModelIndex index=tableWidget.indexAt(pos);
    const auto& game=games[index.row()];

    bool emptySeat=false;
    for (const auto& player:game.players) {
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
          MessageBox(QMessageBox::Critical,tr("Error cancelling game"),exception.what(),QMessageBox::NoButton,server).exec();
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

  tableWidget.setRowCount(static_cast<int>(games.size()));
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
