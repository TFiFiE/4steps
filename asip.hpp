#ifndef ASIP_HPP
#define ASIP_HPP

#include <memory>
#include <QNetworkRequest>
#include <QReadWriteLock>
class QNetworkAccessManager;
class QNetworkReply;
#include "tree.hpp"
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
  virtual ~ASIP();
  Data processReply(QNetworkReply& networkReply);
  QUrl serverURL() const;

  // Game room
  QString username() const;
  QNetworkReply* login(QObject* const requester,const QString& username,const QString& password);
  QNetworkReply* createGame(QObject* const requester,const QString& timeControl,const bool rated,const Side role);
  QNetworkReply* myGames(QObject* const requester);
  QNetworkReply* invitedGames(QObject* const requester);
  QNetworkReply* openGames(QObject* const requester);
  QNetworkReply* enterGame(QObject* const requester,const QString& gameID,const Side side);
  QNetworkReply* cancelGame(QObject* const requester,const QString& gameID);
  QNetworkReply* logout(QObject* const requester);
  virtual std::vector<GameListCategory> availableGameListCategories() const;
  virtual void state();
  std::vector<GameInfo> getGameList(QNetworkReply& networkReply,const GameListCategory gameListCategory);
  std::unique_ptr<ASIP> getGame(QNetworkReply& networkReply);
signals:
  void sendGameList(const GameListCategory,const std::vector<GameInfo>&);
public:
  // Game server
  bool isEqualGame(const ASIP& otherGame) const;
  Side role() const;
  bool gameStateAvailable() const;
  Status getStatus() const;
  Side sideToMove() const;
  std::pair<GameTreeNode,unsigned int> getMoves() const;
  std::array<QString,NUM_SIDES> getPlayers() const;
  Result getResult() const;
  std::array<std::array<qint64,3>,NUM_SIDES> getTimes() const;
  void sit();
  void start();
  void sendMove(const QString& move);
  void resign();
  void requestTakeback();
  void replyToTakeback(const bool accepted);
  void sendChat(const QString& chat);
  void leave();
private:
  void update();
  void authDependingAction(const QString& action,const std::initializer_list<std::pair<QString,QString> >& extraItems={});
signals:
  void updated();
protected:
  std::pair<QString,QString> dataPair(const QString& key) const;
  QNetworkReply* post(QObject* const requester,const std::vector<std::pair<QString,QString> >& items);
private:
  static QNetworkRequest getNetworkRequest(const QString& urlString);
  template<class Type> Type get(const QString& key) const;
  void updateCache(const Data& replyData);
  virtual QByteArray getRequestData(const std::vector<std::pair<QString,QString> >& items)=0;
  virtual Data getReplyData(const QByteArray& data)=0;
  static bool instantResponse(const QNetworkReply& networkReply);
  static EndCondition toEndCondition(const char letter);

  QNetworkAccessManager& networkAccessManager;
  const QNetworkRequest server;
  Data mostRecentData;
  mutable QReadWriteLock mostRecentData_mutex;
  TimeEstimator timeEstimator;
};

#endif // ASIP_HPP
