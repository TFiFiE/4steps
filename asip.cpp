#include <QNetworkAccessManager>
#include <QNetworkReply>
#include "asip.hpp"
#include "io.hpp"
#include "asip1.hpp"

ASIP::ASIP(QNetworkAccessManager& networkAccessManager_,const QString& serverURL,QObject* const parent,Data startingData) :
  QObject(parent),
  networkAccessManager(networkAccessManager_),
  mostRecentData(std::move(startingData)),
  server(getNetworkRequest(serverURL)),
  gameStateReply(nullptr)
{
}

ASIP::Data ASIP::processReply(QNetworkReply& networkReply)
{
  lastReplyTime=QDateTime::currentDateTimeUtc();
  networkReply.deleteLater();
  runtime_assert(networkReply.error()==QNetworkReply::NoError,networkReply.errorString());
  const Status oldStatus=getStatus();
  const auto rawData=networkReply.readAll();
  const auto replyData=getReplyData(rawData);
  updateCache(replyData);
  timeEstimator.add(networkReply.property("post_time").toDateTime(),lastReplyTime,instantResponse(networkReply),replyData.value("timeonserver"));
  const auto error=replyData.find("error");
  if (error!=replyData.end())
    throw std::runtime_error(error.value().toString().toStdString());
  const Status newStatus=getStatus();
  if (oldStatus!=newStatus)
    emit statusChanged(oldStatus,newStatus);
  return replyData;
}

QUrl ASIP::serverURL() const
{
  return server.url();
}

qint64 ASIP::timeSinceLastReply() const
{
  return lastReplyTime.msecsTo(QDateTime::currentDateTimeUtc());
}

QString ASIP::username() const
{
  const QReadLocker readLocker(&mostRecentData_mutex);
  return get<QString>("username");
}

QNetworkReply* ASIP::login(QObject* const requester,const QString& username,const QString& password)
{
  return post(requester,{{"action","login"},{"username",username},{"password",password}});
}

QNetworkReply* ASIP::createGame(QObject* const requester,const QString& timeControl,const bool rated,const Side side)
{
  return post(requester,{{"action","newgame"},dataPair("sid"),{"role",QString(toLetter(side))},{"timecontrol",timeControl},{"rated",rated ? "1" : "0"}});
}

QNetworkReply* ASIP::myGames()
{
  return gameListAction("mygames",MY_GAMES);
}

QNetworkReply* ASIP::invitedGames()
{
  return gameListAction("invitedmegames",INVITED_GAMES);
}

QNetworkReply* ASIP::openGames()
{
  return gameListAction("opengames",OPEN_GAMES);
}

void ASIP::enterGame(QObject* const requester,const QString& gameID,const Side side,const std::function<void(QNetworkReply*)> networkReplyAction)
{
  const auto role=(side==NO_SIDE ? "v" : QString(toLetter(side)));
  const auto networkReply=post(requester,{{"action","reserveseat"},dataPair("sid"),{"gid",gameID},{"role",role}});
  networkReplyAction(networkReply);
}

QNetworkReply* ASIP::cancelGame(QObject* const requester,const QString& gameID)
{
  return post(requester,{{"action","cancelopengame"},dataPair("sid"),{"gid",gameID}});
}

QNetworkReply* ASIP::logout(QObject* const requester)
{
  QReadLocker readLocker(&mostRecentData_mutex);
  const auto sid=mostRecentData.find("sid");
  if (sid!=mostRecentData.end()) {
    const auto sid_value=sid.value();
    readLocker.unlock();
    return post(requester,{{"action","logout"},{"sid",sid_value.toString()}});
  }
  return nullptr;
}

std::vector<ASIP::GameListCategory> ASIP::availableGameListCategories() const
{
  return {MY_GAMES,INVITED_GAMES,OPEN_GAMES};
}

std::vector<QNetworkReply*> ASIP::state()
{
  return {myGames(),invitedGames(),openGames()};
}

std::vector<ASIP::GameInfo> ASIP::getGameList(QNetworkReply& networkReply,const GameListCategory gameListCategory)
{
  const auto replyData=processReply(networkReply);
  unsigned int numGames=-1;
  std::vector<ASIP::GameInfo> result;
  const auto self=username();
  for (auto entry=replyData.begin();entry!=replyData.end();++entry) {
    if (entry.key()=="num")
      numGames=entry.value().toUInt();
    else {
      const size_t index=entry.key().toUInt();
      result.resize(std::max(result.size(),index));
      GameInfo& gameInfo=result[index-1];
      std::stringstream ss;
      ss<<entry.value().toString().toStdString();
      std::string line;
      QString opponent;
      Side role=NO_SIDE;
      while (getline(ss,line)) {
        const auto divider=line.find('=');
        const std::string key=line.substr(0,divider);
        const std::string value=line.substr(divider+1);
        if (key=="gid")
          gameInfo.id=QString::fromStdString(value);
        else if (key=="role")
          role=toSide(value[0]);
        else if (key=="timecontrol")
          gameInfo.timeControl=QString::fromStdString(value);
        else if (key=="rated")
          gameInfo.rated=(value=="1");
        else if (key=="postal")
          gameInfo.postal=(value=="1");
        else if (key=="createdts")
          gameInfo.createdts=stoull(value);
        else if (key=="opponent")
          opponent=QString::fromStdString(value);
      }
      runtime_assert(role!=NO_SIDE,"No role.");
      gameInfo.players[otherSide(role)]=opponent;
      if (gameListCategory==MY_GAMES)
        gameInfo.players[role]=self;
    }
  }
  runtime_assert(numGames==result.size(),"Mismatching number of games.");
  return result;
}

std::unique_ptr<ASIP> ASIP::getGame(QNetworkReply& networkReply)
{
  processReply(networkReply);
  return getGame();
}

std::unique_ptr<ASIP> ASIP::getGame() const
{
  QReadLocker readLocker(&mostRecentData_mutex);
  auto startingData=mostRecentData;
  readLocker.unlock();
  startingData.remove("sid");

  auto game=create(networkAccessManager,mostRecentData.value("gsurl").toString(),nullptr,startingData);
  connect(game.get(),&ASIP::statusChanged,this,&ASIP::childStatusChanged);
  return game;
}

QNetworkReply* ASIP::gameListAction(const QString& action,const ASIP::GameListCategory gameListCategory)
{
  const auto networkReply=post(this,{{"action",action},dataPair("sid")});
  connect(networkReply,&QNetworkReply::finished,this,[this,networkReply,gameListCategory] {
    try {
      emit sendGameList(gameListCategory,getGameList(*networkReply,gameListCategory));
    }
    catch (const std::exception& exception) {
      emit error(exception);
    }
  });
  return networkReply;
}

bool ASIP::isEqualGame(const ASIP& otherGame) const
{
  if (serverURL()!=otherGame.serverURL())
    return false;
  const QReadLocker readLocker(&mostRecentData_mutex);
  const QReadLocker otherReadLocker(&otherGame.mostRecentData_mutex);
  return mostRecentData.value("grid")==otherGame.mostRecentData.value("grid") &&
         mostRecentData.value("tid")==otherGame.mostRecentData.value("tid");
}

Side ASIP::role() const
{
  const QReadLocker readLocker(&mostRecentData_mutex);
  return toSide(get<QString>("role")[0].toLatin1());
}

bool ASIP::gameStateAvailable() const
{
  const QReadLocker readLocker(&mostRecentData_mutex);
  return mostRecentData.contains("auth");
}

ASIP::Status ASIP::getStatus() const
{
  const QReadLocker readLocker(&mostRecentData_mutex);
  if (mostRecentData.contains("result"))
    return FINISHED;
  else if (mostRecentData.contains("starttime"))
    return LIVE;
  else if (mostRecentData.value("canstart")==1)
    return UNSTARTED;
  else
    return OPEN;
}

Side ASIP::sideToMove() const
{
  const QReadLocker readLocker(&mostRecentData_mutex);
  return toSide(get<QString>("turn")[0].toLatin1());
}

std::pair<GameTreeNode,unsigned int> ASIP::getMoves() const
{
  const QReadLocker readLocker(&mostRecentData_mutex);
  return toTree(get<QString>("moves").toStdString());
}

std::array<QString,NUM_SIDES> ASIP::getPlayers() const
{
  const QReadLocker readLocker(&mostRecentData_mutex);
  return {mostRecentData.value("wplayer").toString(),mostRecentData.value("bplayer").toString()};
}

Result ASIP::getResult() const
{
  const QReadLocker readLocker(&mostRecentData_mutex);
  const auto result=mostRecentData.value("result").toString().toStdString();
  if (result.size()>=2)
    return {toSide(result[0]),toEndCondition(result[1])};
  else
    return {NO_SIDE,NO_END};
}

std::array<std::array<qint64,3>,NUM_SIDES> ASIP::getTimes() const
{
  QReadLocker readLocker(&mostRecentData_mutex);
  const Side sideToMove_=sideToMove();
  runtime_assert(sideToMove_!=NO_SIDE,"No side to move.");
  const auto status=getStatus();
  const auto moveTime=mostRecentData.value("tcmove").toLongLong();
  auto maxTurnTime=mostRecentData.value("tcturntime").toLongLong()*1000;
  if (maxTurnTime==0)
    maxTurnTime=std::numeric_limits<qint64>::max();
  std::array<std::array<qint64,3>,NUM_SIDES> result;
  for (Side side=FIRST_SIDE;side<NUM_SIDES;increment(side)) {
    auto&      hard=std::get<0>(result[side]);
    auto& potential=std::get<1>(result[side]);
    auto&      used=std::get<2>(result[side]);

    const auto sideLetter=toLetter(side);
    qint64 moveTime_;
    if (side==FIRST_SIDE && sideToMove_==SECOND_SIDE && get<int>("plycount")==1 && mostRecentData.contains("tcmoveorig"))
      moveTime_=get<qint64>("tcmoveorig");
    else
      moveTime_=moveTime;
    const auto reserveString=QString("tc")+sideLetter+"reserve";
    potential=(moveTime_+mostRecentData.value(mostRecentData.contains(reserveString+'2') ? reserveString+'2' : reserveString).toLongLong())*1000;
    hard=std::min(maxTurnTime,potential);
    used=mostRecentData.value(sideLetter+QString("used")).toLongLong()*1000;
    if (side==sideToMove_) {
      if (status==LIVE)
        used+=timeEstimator.estimatedExtraTime();
      hard-=used;
      potential-=used;
      if (status==LIVE && sideToMove_==role()) {
        const auto predictedSendLag=timeEstimator.estimatedRoundTripTime()/2;
        hard-=predictedSendLag;
        potential-=predictedSendLag;
      }
    }
  }
  return result;
}

void ASIP::sit()
{
  QNetworkReply* sitReply=post(this,{{"action","sit"},dataPair("tid"),dataPair("grid")});
  connect(sitReply,&QNetworkReply::finished,this,[=]() {
    try {
      processReply(*sitReply);
      gameStateReply=post(this,{{"action","gamestate"},dataPair("sid")});
      connect(gameStateReply,&QNetworkReply::finished,this,[=]() {
        try {
          processReply(*gameStateReply);
          update(false);
        }
        catch (const std::exception& exception) {
          emit error(exception);
        }
      });
    }
    catch (const std::exception& exception) {
      emit error(exception);
    }
  });
}

void ASIP::forceUpdate()
{
  if (gameStateReply!=nullptr) {
    disconnect(gameStateReply,&QNetworkReply::finished,nullptr,nullptr);
    gameStateReply->deleteLater();
    gameStateReply=post(this,{{"action","gamestate"},dataPair("sid"),{"wait","0"},{"maxwait","0"}});
    connect(gameStateReply,&QNetworkReply::finished,this,[=]() {
      try {
        processReply(*gameStateReply);
        update(true);
      }
      catch (const std::exception& exception) {
        emit error(exception);
      }
    });
  }
}

void ASIP::start()
{
  postAuthDependingAction("startgame");
}

void ASIP::sendMove(const QString& move)
{
  postAuthDependingAction("move",{{"move",move}});
}

void ASIP::resign()
{
  postAuthDependingAction("resign");
}

void ASIP::requestTakeback()
{
  postAuthDependingAction("takeback",{{"takeback","req"}});
}

void ASIP::replyToTakeback(const bool accepted)
{
  postAuthDependingAction("takebackreply",{{"takebackreply",accepted ? "yes" : "no"}});
}

void ASIP::sendChat(const QString& chat)
{
  postAuthDependingAction("chat",{{"chat",chat}});
}

void ASIP::leave()
{
  if (role()!=NO_SIDE)
    postAuthDependingAction("leave");
}

void ASIP::update(const bool hardSynchronization)
{
  emit updated(hardSynchronization);
  QReadLocker readLocker(&mostRecentData_mutex);
  const auto oldMoves=mostRecentData.value("moves").toString();
  const auto oldChat=mostRecentData.value("chat").toString();
  readLocker.unlock();
  gameStateReply=post(this,{{"action","updategamestate"},dataPair("sid"),{"wait","1"},dataPair("lastchange"),dataPair("moveslength"),dataPair("chatlength")});
  connect(gameStateReply,&QNetworkReply::finished,this,[=]() {
    try {
      processReply(*gameStateReply);
      QWriteLocker writeLocker(&mostRecentData_mutex);
      auto& newMoves=mostRecentData["moves"];
      auto& newChat=mostRecentData["chat"];
      newMoves=oldMoves+newMoves.toString();
      newChat=oldChat+newChat.toString();
      writeLocker.unlock();
      update(false);
    }
    catch (const std::exception& exception) {
      emit error(exception);
    }
  });
}

void ASIP::postAuthDependingAction(const QString& action,const std::initializer_list<std::pair<QString,QString> >& extraItems)
{
  const auto doAction=[=]{
    std::vector<std::pair<QString,QString> > items={{"action",action},dataPair("sid"),dataPair("auth")};
    items.insert(items.end(),extraItems);
    QNetworkReply* actionReply=post(this,items);
    connect(actionReply,&QNetworkReply::finished,[=]{
      try {
        processReply(*actionReply);
      }
      catch (const std::exception& exception) {
        emit error(exception);
      }
    });};
  QReadLocker readLocker(&mostRecentData_mutex);
  if (mostRecentData.contains("auth")) {
    readLocker.unlock();
    doAction();
  }
  else {
    const QObject* const oneTime=new QObject(this);
    connect(this,&ASIP::updated,oneTime,[=]{
      delete oneTime;
      doAction();
    });
  }
}

std::pair<QString,QString> ASIP::dataPair(const QString& key) const
{
  const QReadLocker readLocker(&mostRecentData_mutex);
  const auto iterator=mostRecentData.find(key);
  return {iterator.key(),iterator.value().toString()};
}

QNetworkReply* ASIP::post(QObject* const requester,const std::vector<std::pair<QString,QString> >& items)
{
  QWriteLocker writeLocker(&mostRecentData_mutex);
  for (const auto& item:items)
    mostRecentData.insert(item.first,item.second);
  writeLocker.unlock();
  const auto rawData=getRequestData(items);
  const auto networkReply=networkAccessManager.post(server,rawData);
  networkReply->setProperty("post_time",QDateTime::currentDateTimeUtc());
  for (const auto& item:items)
    networkReply->setProperty(qPrintable(item.first),item.second);
  if (requester!=nullptr)
    networkReply->setParent(requester);
  return networkReply;
}

void ASIP::synchronizeData(const ASIP& source)
{
  QWriteLocker writeLocker(&mostRecentData_mutex);
  QReadLocker readLocker(&source.mostRecentData_mutex);
  mostRecentData=source.mostRecentData;
}

template<class Type>
Type ASIP::get(const QString& key) const
{
  runtime_assert(mostRecentData.contains(key),"Key "+key.toStdString()+" not present.");
  const auto value=mostRecentData.value(key);
  runtime_assert(value.canConvert<Type>(),"Value "+value.toString()+" not convertible to "+QMetaType::typeName(qMetaTypeId<Type>())+".");
  return value.value<Type>();
}

void ASIP::updateCache(const Data& replyData)
{
  const QWriteLocker writeLocker(&mostRecentData_mutex);
  for (auto iter=replyData.begin();iter!=replyData.end();++iter)
    mostRecentData.insert(iter.key(),iter.value());
}

bool ASIP::instantResponse(const QNetworkReply& networkReply)
{
  return ((networkReply.property("action")!="gamestate" &&
           networkReply.property("action")!="updategamestate") ||
          networkReply.property("wait")!="1");
}

EndCondition ASIP::toEndCondition(const char letter)
{
  switch (letter) {
    case 'g': return GOAL;
    case 'm': return IMMOBILIZATION;
    case 'e': return ELIMINATION;
    case 't': return TIME_OUT;
    case 'r': return RESIGNATION;
    case 'i': return ILLEGAL_MOVE;
    case 'f': return FORFEIT;
    case 'a': return ABANDONMENT;
    case 's': return SCORE;
    case 'p': return REPETITION;
    case 'n': return MUTUAL_ELIMINATION;
    default : throw std::runtime_error("Unrecognized termination code: "+std::string(1,letter));
  }
}
