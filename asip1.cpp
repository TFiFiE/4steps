#include <sstream>
#include <QUrlQuery>
#include "asip1.hpp"

ASIP1::ASIP1(QNetworkAccessManager& networkAccessManager,const QString& serverURL,QObject* const parent,Data startingData) :
  ASIP(networkAccessManager,serverURL,parent,startingData)
{
}

std::unique_ptr<ASIP> ASIP1::create(QNetworkAccessManager& networkAccessManager,const QString& serverURL,QObject* const parent,Data startingData) const
{
  using namespace std;
  return make_unique<ASIP1>(networkAccessManager,serverURL,parent,startingData);
}

QByteArray ASIP1::getRequestData(const std::vector<std::pair<QString,QString> >& items)
{
  QUrlQuery urlQuery;
  for (const auto& item:items)
    if (!item.second.isEmpty())
      urlQuery.addQueryItem((item.first),(item.second));
  return urlQuery.toString(QUrl::FullyEncoded).toUtf8();
}

ASIP::Data ASIP1::getReplyData(const QByteArray& data)
{
  Data map;
  std::stringstream ss;
  const std::string string=data.toStdString();
  const std::string prefix("&junkvar=");
  if (string.compare(0,prefix.size(),prefix)==0)
    ss<<string.substr(string.find_first_not_of('x',prefix.size()));
  else
    ss<<string;
  std::string line;
  while (getline(ss,line)) {
    if (line=="--END--")
      break;
    const size_t delimiter=line.find('=');
    std::string key=line.substr(0,delimiter);
    std::string encodedValue=line.substr(delimiter+1);
    std::string decodedValue=replaceString(replaceString(encodedValue,"%13","\n"),"%25","%");
    map.insert(QString::fromStdString(key),QString::fromStdString(decodedValue));
  }
  return map;
}
