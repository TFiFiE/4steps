#include <QNetworkReply>
#include <QNetworkCookieJar>
#include <QNetworkCookie>
#include <QCheckBox>
#include <QDesktopServices>
#include "arimaa_com.hpp"
#include "asip1.hpp"
#include "asip2.hpp"
#include "server.hpp"
#include "io.hpp"
#include "bots.hpp"

template<class Base>
Arimaa_com<Base>::Arimaa_com(QNetworkAccessManager& networkAccessManager,const QString& serverURL,QObject* const parent,typename Base::Data startingData) :
  Base(networkAccessManager,serverURL,parent,startingData),
  ratedMode(true)
{
  Base::connect(this,&Base::sendGameList,[this](const typename Base::GameListCategory gameListCategory,const std::vector<typename Base::GameInfo>& games) {
    auto& fixedCatGames=fixedGames[gameListCategory];
    fixedCatGames.clear();
    const bool alreadyJoined=(gameListCategory!=Base::INVITED_GAMES && gameListCategory!=Base::OPEN_GAMES);
    for (const auto& game:games) {
      if (alreadyJoined || !game.rated || std::none_of(std::begin(game.players),std::end(game.players),isBotName))
        fixedCatGames.emplace(game.id);
    }
  });
}

template<class Base>
std::unique_ptr<::ASIP> Arimaa_com<Base>::create(QNetworkAccessManager& networkAccessManager,const QString& serverURL,QObject* const parent,typename Base::Data startingData) const
{
  using namespace std;
  return make_unique<Arimaa_com<Base> >(networkAccessManager,serverURL,parent,startingData);
}

template<class Base>
typename Base::Data Arimaa_com<Base>::processReply(QNetworkReply& networkReply)
{
  return Base::processReply(networkReply);
}

template<>
typename ASIP::Data Arimaa_com<ASIP2>::processReply(QNetworkReply& networkReply)
{
  try {
    return ASIP::processReply(networkReply);
  }
  catch (const std::exception&) {
    if (networkReply.property("action")=="cancelopengame") // SERVER BUG: Cancelling with ASIP 2.0 triggers error.
      return ASIP::Data({{"ok","1"}});
    throw;
  }
}

template<class Base>
void Arimaa_com<Base>::enterGame(QObject* const requester,const QString& gameID,const Side side,const std::function<void(QNetworkReply*)> networkReplyAction)
{
  Base::enterGame(requester,gameID,side,networkReplyAction);
}

template<>
void Arimaa_com<ASIP2>::enterGame(QObject* const requester,const QString& gameID,const Side side,const std::function<void(QNetworkReply*)> networkReplyAction)
{
  if (side==NO_SIDE) { // SERVER BUG: Can't spectate with ASIP 2.0.
    QReadLocker readLocker(&mostRecentData_mutex);
    ASIP1 asip1(networkAccessManager,serverDirectory().toString()+"client1gr.cgi",nullptr,mostRecentData);
    readLocker.unlock();
    asip1.enterGame(requester,gameID,side,networkReplyAction);
    synchronizeData(asip1);
    return;
  }
  else if (possiblyDerateable(gameID)) {
    const auto possible=doMeDependingAction([=] {
      QReadLocker readLocker(&mostRecentData_mutex);
      const auto me=mostRecentData.value("me").value<QVariantHash>();
      const auto sid=mostRecentData.value("sid");
      readLocker.unlock();

      const auto server=serverDirectory();
      const auto cookieJar=networkAccessManager.cookieJar();
      const auto oldCookies=cookieJar->cookiesForUrl(server);
      cookieJar->setCookiesFromUrl({QNetworkCookie("unrated"+me["id"].toByteArray(),"1"),QNetworkCookie("sid",sid.toByteArray())},server);
      const auto networkReply=networkAccessManager.get(QNetworkRequest(QUrl(server.toString()+"opengamewin.cgi?gameid="+gameID+"&side="+QString(toLetter(side)))));
      networkReply->setParent(this);
      for (const auto& cookie:cookieJar->cookiesForUrl(server))
        cookieJar->deleteCookie(cookie);
      cookieJar->setCookiesFromUrl(oldCookies,server);

      connect(networkReply,&QNetworkReply::finished,this,[=] {
        networkReply->deleteLater();
        ASIP::enterGame(requester,gameID,side,networkReplyAction);
      });
    });
    if (possible)
      return;
  }
  ASIP::enterGame(requester,gameID,side,networkReplyAction);
}

template<class Base>
std::unique_ptr<::ASIP> Arimaa_com<Base>::getGame(QNetworkReply& networkReply)
{
  return Base::getGame(networkReply);
}

template<>
std::unique_ptr<::ASIP> Arimaa_com<ASIP2>::getGame(QNetworkReply& networkReply)
{
  if (networkReply.property("role")=="v" && networkReply.property("action")=="reserveseat") { // SERVER BUG: Can't spectate with ASIP 2.0.
    QReadLocker readLocker(&mostRecentData_mutex);
    ASIP1 asip1(networkAccessManager,serverDirectory().toString()+"client1gr.cgi",nullptr,mostRecentData);
    readLocker.unlock();
    asip1.processReply(networkReply);
    synchronizeData(asip1);
  }
  else
    processReply(networkReply);

  QWriteLocker writeLocker(&mostRecentData_mutex);
  auto gsurl=mostRecentData.value("gsurl").toString();
  if (QUrl(gsurl).isRelative()) // SERVER BUG: Joining with ASIP 2.0 returns partial game server URL.
    gsurl=serverDirectory().toString()+"../java/ys/ms4/v5/"+gsurl;
  gsurl=QUrl(gsurl).adjusted(QUrl::RemoveFilename).toString()+"client2gs.cgi"; // SERVER BUG: Returned game server always uses ASIP 1.0.
  mostRecentData.insert("gsurl",gsurl);
  writeLocker.unlock();

  return ASIP::getGame();
}

template<class Base>
bool Arimaa_com<Base>::possiblyDerateable(const QString& gameID) const
{
  if (ratedMode)
    return false;
  for (const auto& category:fixedGames)
    if (category.second.find(gameID)!=category.second.end())
      return false;
  return true;
}

template<class Base>
std::unique_ptr<QWidget> Arimaa_com<Base>::siteWidget(Server& server)
{
  const auto mainLayout=new QHBoxLayout;

  mainLayout->addWidget(botWidget(server.mainWindow).release());

  if (std::is_same<Base,ASIP2>::value) {
    const auto ratedBox=new QCheckBox("Keep joined bot games rated");
    mainLayout->addWidget(ratedBox,0,Qt::AlignCenter);
    ratedBox->setChecked(ratedMode);
    Base::connect(ratedBox,&QCheckBox::toggled,[this](const bool checked) {ratedMode=checked;});

    mainLayout->addWidget(chatroomWidget().release());
  }

  using namespace std;
  auto widget=make_unique<QWidget>();
  widget->setLayout(mainLayout);
  return widget;
}

template<class Base>
bool Arimaa_com<Base>::isBotName(const QString& playerName)
{
  return playerName.indexOf("bot_")==0;
}

template<class Base>
QUrl Arimaa_com<Base>::serverDirectory() const
{
  return Base::serverURL().adjusted(QUrl::RemoveFilename);
}

template<class Base> template<class Function>
bool Arimaa_com<Base>::doMeDependingAction(Function action)
{
  static_cast<void>(action);
  return false;
}

template<> template<class Function>
bool Arimaa_com<ASIP2>::doMeDependingAction(Function action)
{
  QReadLocker readLocker(&mostRecentData_mutex);
  if (mostRecentData.contains("me")) {
    readLocker.unlock();
    action();
  }
  else {
    readLocker.unlock();
    const auto networkReplies=state();
    connect(networkReplies.front(),&QNetworkReply::finished,this,[this,action] {
      const QReadLocker readLocker(&mostRecentData_mutex);
      action();
    });
  }
  return true;
}

template<class Base>
std::unique_ptr<QWidget> Arimaa_com<Base>::chatroomWidget()
{
  return nullptr;
}

template<>
std::unique_ptr<QWidget> Arimaa_com<ASIP2>::chatroomWidget()
{
  using namespace std;
  auto chatroom=make_unique<QPushButton>("Enter chat room");

  connect(chatroom.get(),&QPushButton::clicked,this,[this] {
    doMeDependingAction([this] {
      const auto me=mostRecentData.value("me").value<QVariantHash>();
      const auto url=serverDirectory().toString()+"../chat/chat.php?user="+username()+"&auth="+me["auth"].toString();
      QDesktopServices::openUrl(url);
    });
  });

  return chatroom;
}

template<class Base>
std::unique_ptr<QWidget> Arimaa_com<Base>::botWidget(MainWindow& mainWindow)
{
  using namespace std;
  auto bots=make_unique<QPushButton>("Bot launcher");

  const auto url=serverDirectory().toString()+"botLadderAll.cgi";
  Base::connect(bots.get(),&QPushButton::clicked,this,[this,url,&mainWindow] {
    #if 1
    new Bots(*this,url+"?u="+Base::username(),mainWindow);
    #else
    const bool possible=doMeDependingAction([this,url,&mainWindow] {
      const auto me=Base::mostRecentData.value("me").template value<QVariantHash>();
      new Bots(*this,url+"?u="+me["id"].toString(),mainWindow);
    });
    if (!possible)
      new Bots(*this,url,mainWindow);
    #endif
  });

  return bots;
}

template class Arimaa_com<ASIP1>;
template class Arimaa_com<ASIP2>;
