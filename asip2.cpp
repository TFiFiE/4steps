#include <QNetworkReply>
#include <QJsonObject>
#include <QJsonDocument>
#include "asip2.hpp"

ASIP2::ASIP2(QNetworkAccessManager& networkAccessManager,const QString& serverURL,QObject* const parent,Data startingData) :
  ASIP(networkAccessManager,serverURL,parent,startingData)
{
}

std::unique_ptr<ASIP> ASIP2::create(QNetworkAccessManager& networkAccessManager,const QString& serverURL,QObject* const parent,Data startingData) const
{
  using namespace std;
  return make_unique<ASIP2>(networkAccessManager,serverURL,parent,startingData);
}

std::vector<ASIP::GameListCategory> ASIP2::availableGameListCategories() const
{
  return {MY_GAMES,INVITED_GAMES,OPEN_GAMES,LIVE_GAMES/*,RECENT_GAMES*/};
}

std::vector<QNetworkReply*> ASIP2::state()
{
  const auto stateReply=post(this,{{"action","state"},dataPair("sid")});
  connect(stateReply,&QNetworkReply::finished,this,[=]{
    try {
      const auto stateData=processReply(*stateReply);
      const std::pair<QString,GameListCategory> gameListActions[]={
        {"mygames",MY_GAMES},
        {"invitedmegames",INVITED_GAMES},
        {"opengames",OPEN_GAMES},
        {"livegames",LIVE_GAMES},
        {"recentgames",RECENT_GAMES}};
      for (const auto& gameListAction:gameListActions)
        emit sendGameList(std::get<1>(gameListAction),getGameList(stateData[std::get<0>(gameListAction)].toList()));
    }
    catch (const std::exception& exception) {
      emit error(exception);
    }
  });
  return std::vector<QNetworkReply*>(1,stateReply);
}

std::vector<ASIP::GameInfo> ASIP2::getGameList(const QVariantList& games)
{
  std::vector<ASIP::GameInfo> result;
  result.reserve(games.size());
  for (const auto& game:games) {
    result.emplace_back(ASIP::GameInfo());
    GameInfo& gameInfo=result.back();

    const auto gameMap=game.toMap();
    for (auto pair=gameMap.begin();pair!=gameMap.end();++pair) {
      const auto key=pair.key();
      const auto value=pair.value();
      if (key=="id")
        gameInfo.id=value.toString();
      else if (key=="timecontrol")
        gameInfo.timeControl=value.toString();
      else if (key=="rated")
        gameInfo.rated=(value=="1");
      else if (key=="postal")
        gameInfo.postal=(value=="1");
      else if (key=="createdts")
        gameInfo.createdts=value.toULongLong();
      else if (key=="wusername")
        gameInfo.players[FIRST_SIDE]=value.toString();
      else if (key=="busername")
        gameInfo.players[SECOND_SIDE]=value.toString();
    }
  }
  return result;
}

QByteArray ASIP2::getRequestData(const std::vector<std::pair<QString,QString> >& items)
{
  QJsonObject jsonObject;
  for (const auto& item:items)
    if (!item.second.isEmpty())
      jsonObject.insert(item.first,item.second);
  return QJsonDocument(jsonObject).toJson(QJsonDocument::Compact);
}

ASIP::Data ASIP2::getReplyData(const QByteArray& data)
{
  const auto jsonDocument=QJsonDocument::fromJson(data);
  runtime_assert(data.isEmpty() || !jsonDocument.isNull(),"Not a valid JSON document.");
  return jsonDocument.object().toVariantHash();
}
