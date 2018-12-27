#include <QNetworkReply>
#include "arimaa_com.hpp"
#include "asip1.hpp"
#include "asip2.hpp"

template class Arimaa_com<ASIP1>;
template class Arimaa_com<ASIP2>;

template<class ASIP>
Arimaa_com<ASIP>::Arimaa_com(QNetworkAccessManager& networkAccessManager,const QString& serverURL,QObject* const parent,typename ASIP::Data startingData) :
  ASIP(networkAccessManager,serverURL,parent,startingData)
{
}

template<class ASIP>
std::unique_ptr<::ASIP> Arimaa_com<ASIP>::create(QNetworkAccessManager& networkAccessManager,const QString& serverURL,QObject* const parent,typename ASIP::Data startingData) const
{
  using namespace std;
  return make_unique<Arimaa_com<ASIP> >(networkAccessManager,serverURL,parent,startingData);
}

template<class ASIP>
typename ASIP::Data Arimaa_com<ASIP>::processReply(QNetworkReply& networkReply)
{
  return ASIP::processReply(networkReply);
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

template<class ASIP>
void Arimaa_com<ASIP>::enterGame(QObject* const requester,const QString& gameID,const Side side,const std::function<void(QNetworkReply*)> networkReplyAction)
{
  ASIP::enterGame(requester,gameID,side,networkReplyAction);
}

template<>
void Arimaa_com<ASIP2>::enterGame(QObject* const requester,const QString& gameID,const Side side,const std::function<void(QNetworkReply*)> networkReplyAction)
{
  if (side==NO_SIDE) { // SERVER BUG: Can't spectate with ASIP 2.0.
    QReadLocker readLocker(&mostRecentData_mutex);
    ASIP1 asip1(networkAccessManager,serverURL().adjusted(QUrl::RemoveFilename).toString()+"client1gr.cgi",nullptr,mostRecentData);
    readLocker.unlock();
    asip1.enterGame(requester,gameID,side,networkReplyAction);
    synchronizeData(asip1);
    return;
  }
  else
    ASIP::enterGame(requester,gameID,side,networkReplyAction);
}

template<class ASIP>
std::unique_ptr<::ASIP> Arimaa_com<ASIP>::getGame(QNetworkReply& networkReply)
{
  return ASIP::getGame(networkReply);
}

template<>
std::unique_ptr<::ASIP> Arimaa_com<ASIP2>::getGame(QNetworkReply& networkReply)
{
  const auto server=serverURL();
  if (networkReply.property("role")=="v" && networkReply.property("action")=="reserveseat") { // SERVER BUG: Can't spectate with ASIP 2.0.
    QReadLocker readLocker(&mostRecentData_mutex);
    ASIP1 asip1(networkAccessManager,server.adjusted(QUrl::RemoveFilename).toString()+"client1gr.cgi",nullptr,mostRecentData);
    readLocker.unlock();
    asip1.processReply(networkReply);
    synchronizeData(asip1);
  }
  else
    processReply(networkReply);

  QWriteLocker writeLocker(&mostRecentData_mutex);
  auto gsurl=mostRecentData.value("gsurl").toString();
  if (QUrl(gsurl).isRelative()) // SERVER BUG: Joining with ASIP 2.0 returns partial game server URL.
    gsurl=server.adjusted(QUrl::RemoveFilename).toString()+"../java/ys/ms4/v5/"+gsurl;
  gsurl=QUrl(gsurl).adjusted(QUrl::RemoveFilename).toString()+"client2gs.cgi"; // SERVER BUG: Returned game server always uses ASIP 1.0.
  mostRecentData.insert("gsurl",gsurl);
  writeLocker.unlock();

  return ASIP::getGame();
}
