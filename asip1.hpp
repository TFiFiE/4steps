#ifndef ASIP1_HPP
#define ASIP1_HPP

#include "asip.hpp"

class ASIP1 : public ASIP {
  Q_OBJECT
public:
  explicit ASIP1(QNetworkAccessManager& networkAccessManager,const QString& serverURL,QObject* const parent=nullptr,Data startingData=Data());
  virtual std::unique_ptr<ASIP> create(QNetworkAccessManager& networkAccessManager,const QString& serverURL,QObject* const parent=nullptr,Data startingData=Data()) const override;
private:
  virtual QByteArray getRequestData(const std::vector<std::pair<QString,QString> >& items) override;
  virtual Data getReplyData(const QByteArray& data) override;
};

#endif // ASIP1_HPP
