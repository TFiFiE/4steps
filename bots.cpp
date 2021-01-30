#include <QHeaderView>
#include <QTextEdit>
#include <QNetworkReply>
#include <QXmlStreamReader>
#include <QUrlQuery>
#include <QScreen>
#include "bots.hpp"
#include "mainwindow.hpp"
#include "io.hpp"
#include "messagebox.hpp"
#include "globals.hpp"

using namespace std;
Bots::Bots(ASIP& session_,const QUrl& url_,MainWindow& mainWindow_) :
  QDialog(&mainWindow_,Qt::Window),
  session(session_),
  networkAccessManager(session.networkAccessManager),
  mainWindow(mainWindow_),
  url(url_),
  nameColumn(-1),
  linkColumn(-1),
  vBoxLayout(this),
  create(tr("Create &bot game")),
  refresh(tr("&Refresh list")),
  closeButton(tr("&Close")),
  join(tr("&Join game on creation")),
  botSide(tr("Bot's side:")),
  sideButtons{make_unique<QRadioButton>(tr("white (Gold)")),
              make_unique<QRadioButton>(tr("black (Silver)")),
              make_unique<QRadioButton>(tr("random"))},
  label(tr("Server response:"))
{
  setWindowTitle(tr("Server bots"));

  connect(&create,&QPushButton::clicked,this,[this] {
    createGame(tableView.selectionModel()->selectedRows().first());
  });
  hBoxLayout.addWidget(&create);

  join.setChecked(true);
  connect(&session,&ASIP::destroyed,this,[this] {
    join.setChecked(false);
    join.setEnabled(false);
  });
  hBoxLayout.addWidget(&join,0,Qt::AlignCenter);

  connect(&refresh,&QPushButton::clicked,this,&Bots::update);
  hBoxLayout.addWidget(&refresh);

  connect(&closeButton,&QPushButton::clicked,this,&Bots::close);
  hBoxLayout.addWidget(&closeButton);

  vBoxLayout.addLayout(&hBoxLayout);

  sideLayout.addWidget(&botSide,0,Qt::AlignCenter);
  sideLayout.addWidget(sideButtons[0].get());
  sideLayout.addWidget(sideButtons[1].get());
  sideLayout.addWidget(sideButtons[2].get());
  sideButtons[2]->setChecked(true);

  vBoxLayout.addLayout(&sideLayout);

  tableView.setModel(&standardItemModel);
  tableView.setAlternatingRowColors(true);
  tableView.verticalHeader()->setVisible(false);
  tableView.horizontalHeader()->setHighlightSections(false);
  tableView.setEditTriggers(QAbstractItemView::NoEditTriggers);
  tableView.setSelectionBehavior(QAbstractItemView::SelectRows);
  tableView.setSelectionMode(QAbstractItemView::SingleSelection);
  connect(&tableView,&QTableView::doubleClicked,this,&Bots::createGame);

  vBoxLayout.addWidget(&tableView);

  label.setEnabled(false);
  vBoxLayout.addWidget(&label);

  message.setStyleSheet("background-color:"+QTextEdit().palette().base().color().name()+';');
  message.setTextInteractionFlags(Qt::TextSelectableByMouse);
  message.setVisible(false);
  vBoxLayout.addWidget(&message);

  create.setEnabled(tableView.selectionModel()->hasSelection());
  connect(tableView.selectionModel(),&QItemSelectionModel::selectionChanged,&create,[this](const QItemSelection& selected) {
    create.setEnabled(!selected.isEmpty());
  });

  setAttribute(Qt::WA_DeleteOnClose);
  show();

  update();
}

void Bots::update()
{
  const auto networkReply=networkAccessManager.get(QNetworkRequest(url));
  networkReply->setParent(this);
  connect(networkReply,&QNetworkReply::finished,this,[=] {
    networkReply->deleteLater();
    parse(networkReply->readAll());
  });
}

void Bots::parse(QString page)
{
  const auto begin=page.indexOf("<tr",0,Qt::CaseInsensitive);
  const auto end=page.lastIndexOf("</table",-1,Qt::CaseInsensitive);
  page="<table>"+page.mid(begin,end-begin)+"</table>";
  page.replace(QRegularExpression("<td align=(.*?)>"),"<td align='\\1'>");
  page.replace(QRegularExpression("(?!&[a-z]+;)&"),"&amp;");
  QXmlStreamReader xmlStreamReader(page);

  std::vector<std::vector<std::pair<QString,QString> > > rows;
  QString fullText,linkText,link;
  for (auto token=xmlStreamReader.readNext();token!=QXmlStreamReader::EndDocument;token=xmlStreamReader.readNext()) {
    if (token==QXmlStreamReader::Characters) {
      const auto text=xmlStreamReader.text();
      fullText+=text;
      linkText+=text;
    }
    else if (xmlStreamReader.name()=="tr") {
      if (token==QXmlStreamReader::StartElement)
        rows.emplace_back();
    }
    else if (xmlStreamReader.name()=="td") {
      if (token==QXmlStreamReader::StartElement) {
        fullText.clear();
        link.clear();
        rows.back().emplace_back();
      }
      else if (token==QXmlStreamReader::EndElement)
        rows.back().back().first=(link.isEmpty() ? fullText : linkText).trimmed();
    }
    else if (xmlStreamReader.name()=="a") {
      if (token==QXmlStreamReader::StartElement) {
        linkText.clear();
        link=xmlStreamReader.attributes().value("href").toString();
        rows.back().back().second=link;
      }
    }
  }

  standardItemModel.setRowCount(static_cast<int>(rows.size()-1));
  for (unsigned int rowIndex=0;rowIndex<rows.size();++rowIndex) {
    const auto& row=rows[rowIndex];
    if (rowIndex==0) {
      standardItemModel.setColumnCount(static_cast<int>(row.size()));
      for (unsigned int cellIndex=0;cellIndex<row.size();++cellIndex) {
        const auto& cell=row[cellIndex];
        standardItemModel.setHeaderData(cellIndex,Qt::Horizontal,QString(cell.first).replace('_',' '));
        const bool hasLink=(cell.first.toLower()=="bot_link");
        tableView.setColumnHidden(cellIndex,hasLink);
        if (hasLink)
          linkColumn=cellIndex;
        if (!cell.second.isEmpty())
          standardItemModel.setHeaderData(cellIndex,Qt::Horizontal,cell.second,Qt::UserRole);
        if (cell.first.toLower()=="bot_name")
          nameColumn=cellIndex;
      }
    }
    else {
      const unsigned int dataRowIndex=rowIndex-1;
      for (unsigned int cellIndex=0;cellIndex<row.size();++cellIndex) {
        const auto& cell=row[cellIndex];
        const auto& modelIndex=standardItemModel.index(dataRowIndex,cellIndex);
        bool isNumber;
        const QVariant number=cell.first.toDouble(&isNumber);
        standardItemModel.setData(modelIndex,isNumber ? number : QVariant(cell.first));
        const auto alignment=(Qt::AlignVCenter|(isNumber ? Qt::AlignRight : (cellIndex==nameColumn ? Qt::AlignLeft : Qt::AlignHCenter)));
        standardItemModel.itemFromIndex(modelIndex)->setTextAlignment(alignment);
        if (!cell.second.isEmpty())
          standardItemModel.setData(modelIndex,cell.second,Qt::UserRole);
      }
    }
  }
  tableView.horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
  tableView.horizontalHeader()->setStretchLastSection(true);
  tableView.horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
  tableView.horizontalHeader()->setStretchLastSection(false);
  tableView.setSortingEnabled(true);
}

void Bots::createGame(const QModelIndex& index)
{
  const auto botName=index.siblingAtColumn(nameColumn).data();
  const Side side=getSide();
  const auto botPage=url.resolved(index.siblingAtColumn(linkColumn).data(Qt::UserRole).toUrl());
  QUrlQuery urlQuery;
  urlQuery.addQueryItem("action","player");
  urlQuery.addQueryItem("side",QString(toLetter(side)));
  urlQuery.addQueryItem("newgame","Start Bot");
  tableView.setEnabled(false);
  create.setEnabled(false);
  label.setEnabled(false);
  message.setVisible(false);

  const auto botStartReply=networkAccessManager.post(getNetworkRequest(botPage),urlQuery.toString(QUrl::FullyEncoded).toUtf8());
  botStartReply->setParent(this);
  connect(botStartReply,&QNetworkReply::finished,[=] {
    botStartReply->deleteLater();
    label.setEnabled(true);
    message.setVisible(true);
    message.setText(botStartReply->readAll());
    if (join.isEnabled()) { // Is server session still alive?
      const auto openGamesReply=session.openGames();
      if (join.isChecked()) {
        const QObject* const pending=new QObject(this);
        connect(&session,&ASIP::sendGameList,pending,[=](const typename ASIP::GameListCategory gameListCategory,const std::vector<typename ASIP::GameInfo>& games) {
          if (gameListCategory==ASIP::OPEN_GAMES) {
            for (int gameIndex=static_cast<int>(games.size()-1);gameIndex>=0;--gameIndex) {
              const auto& game=games[gameIndex];
              if (game.players[side]==botName) {
                delete pending;
                enable();
                enterGame(game,otherSide(side),otherSide(side));
                return;
              }
            }
          }
        });
        connect(openGamesReply,&QNetworkReply::finished,pending,[=] {
          delete pending;
          enable();
          MessageBox(QMessageBox::Critical,tr("Error joining game"),"No matching game found.",QMessageBox::NoButton,this).exec();
        });
        return;
      }
    }
    enable();
  });
}

void Bots::enterGame(const ASIP::GameInfo& game,const Side role,const Side viewpoint)
{
  session.enterGame(this,game.id,role,[=](QNetworkReply* const networkReply) {
    connect(networkReply,&QNetworkReply::finished,this,[=] {
      try {
        auto game=session.getGame(*networkReply);
        connect(game.get(),&ASIP::statusChanged,this,[this](const ASIP::Status oldStatus,const ASIP::Status newStatus) {
          if (oldStatus==ASIP::LIVE && newStatus==ASIP::FINISHED)
            update();
        });
        mainWindow.addGame(std::move(game),viewpoint,true);
      }
      catch (const std::exception& exception) {
        MessageBox(QMessageBox::Critical,tr("Error opening game"),exception.what(),QMessageBox::NoButton,this).exec();
      }
    });
  });
}

void Bots::enable()
{
  tableView.setEnabled(true);
  create.setEnabled(true);
}

Side Bots::getSide() const
{
  if (sideButtons[0]->isChecked())
    return FIRST_SIDE;
  else if (sideButtons[1]->isChecked())
    return SECOND_SIDE;
  else {
    assert(sideButtons[2]->isChecked());
    return Side(mainWindow.globals.rand(NUM_SIDES));
  }
}

QSize Bots::sizeHint() const
{
  return qApp->primaryScreen()->availableGeometry().size();
}
