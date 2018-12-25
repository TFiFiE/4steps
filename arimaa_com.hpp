#ifndef ARIMAA_COM_HPP
#define ARIMAA_COM_HPP

#include "asip.hpp"

template<class ASIP>
class Arimaa_com : public ASIP {
public:
  Arimaa_com(QNetworkAccessManager& networkAccessManager,const QString& serverURL,QObject* const parent=nullptr,typename ASIP::Data startingData=typename ASIP::Data());
  virtual std::unique_ptr<::ASIP> create(QNetworkAccessManager& networkAccessManager,const QString& serverURL,QObject* const parent=nullptr,typename ASIP::Data startingData=typename ASIP::Data()) const override;
  virtual typename ASIP::Data processReply(QNetworkReply& networkReply) override;
  virtual QNetworkReply* enterGame(QObject* const requester,const QString& gameID,const Side side) override;
  virtual std::unique_ptr<::ASIP> getGame(QNetworkReply& networkReply) override;
};

#endif // ARIMAA_COM_HPP
