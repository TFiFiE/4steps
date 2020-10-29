#ifndef ASIP_HPP
#define ASIP_HPP

#include <memory>
class QNetworkAccessManager;
class QNetworkReply;
#include <QNetworkRequest>
#include <QReadWriteLock>
class Server;
#include "node.hpp"
#include "timeestimator.hpp"

class ASIP : public QObject {
  Q_OBJECT
protected:
  typedef QVariantHash Data;
public:
  enum GameListCategory {
    MY_GAMES,
    INVITED_GAMES,
    OPEN_GAMES,
    LIVE_GAMES,
    RECENT_GAMES
  };

  struct GameInfo {
    QString id,timeControl;
    QString players[NUM_SIDES];
    bool rated,postal;
    unsigned long long createdts;
  };

  enum Status {
    OPEN,
    UNSTARTED,
    LIVE,
    FINISHED
  };

  explicit ASIP(QNetworkAccessManager& networkAccessManager_,const QString& serverURL,QObject* const parent=nullptr,Data startingData=Data());
  virtual std::unique_ptr<ASIP> create(QNetworkAccessManager& networkAccessManager,const QString& serverURL,QObject* const parent=nullptr,Data startingData=Data()) const=0;
  virtual ~ASIP() {}
  virtual Data processReply(QNetworkReply& networkReply);
  QUrl serverURL() const;
  qint64 timeSinceLastReply() const;

  QNetworkAccessManager& networkAccessManager;

  // Game room
  QString username() const;
  QNetworkReply* login(QObject* const requester,const QString& username,const QString& password);
  QNetworkReply* createGame(QObject* const requester,const QString& timeControl,const bool rated,const Side role);
  QNetworkReply* myGames();
  QNetworkReply* invitedGames();
  QNetworkReply* openGames();
  virtual void enterGame(QObject* const requester,const QString& gameID,const Side side,const std::function<void(QNetworkReply*)> networkReplyAction);
  QNetworkReply* cancelGame(QObject* const requester,const QString& gameID);
  QNetworkReply* logout(QObject* const requester);
  virtual std::vector<GameListCategory> availableGameListCategories() const;
  virtual std::vector<QNetworkReply*> state();
  std::vector<GameInfo> getGameList(QNetworkReply& networkReply,const GameListCategory gameListCategory);
  virtual std::unique_ptr<ASIP> getGame(QNetworkReply& networkReply);
  virtual std::unique_ptr<QWidget> siteWidget(Server&) {return nullptr;}
protected:
  std::unique_ptr<ASIP> getGame() const;
signals:
  void sendGameList(const GameListCategory,const std::vector<GameInfo>&);
  void childStatusChanged(const Status oldStatus,const Status newStatus);
private:
  QNetworkReply* gameListAction(const QString& action,const GameListCategory gameListCategory);
public:
  // Game server
  bool isEqualGame(const ASIP& otherGame) const;
  Side role() const;
  bool gameStateAvailable() const;
  Status getStatus() const;
  Side sideToMove() const;
  std::tuple<GameTree,size_t,bool> getMoves(NodePtr root) const;
  std::array<QString,NUM_SIDES> getPlayers() const;
  Result getResult() const;
  std::array<std::array<qint64,3>,NUM_SIDES> getTimes() const;
  void sit();
  void forceUpdate();
  void start();
  void sendMove(const QString& move);
  void resign();
  void requestTakeback();
  void replyToTakeback(const bool accepted);
  void sendChat(const QString& chat);
  void leave();
private:
  void update(const bool hardSynchronization);
  void postAuthDependingAction(const QString& action,const std::initializer_list<std::pair<QString,QString> >& extraItems={});
signals:
  void updated(const bool hardSynchronization);
  void statusChanged(const Status oldStatus,const Status newStatus);
protected:
  std::pair<QString,QString> dataPair(const QString& key) const;
  QNetworkReply* post(QObject* const requester,const std::vector<std::pair<QString,QString> >& items);
  void synchronizeData(const ASIP& source);

  Data mostRecentData;
  mutable QReadWriteLock mostRecentData_mutex;
private:
  template<class Type> Type get(const QString& key) const;
  void updateCache(const Data& replyData);
  virtual QByteArray getRequestData(const std::vector<std::pair<QString,QString> >& items)=0;
  virtual Data getReplyData(const QByteArray& data)=0;
  static bool instantResponse(const QNetworkReply& networkReply);

  const QNetworkRequest server;
  TimeEstimator timeEstimator;
  QDateTime lastReplyTime;
  QNetworkReply* gameStateReply;
signals:
  void error(const std::exception& exception);
};

#endif // ASIP_HPP
