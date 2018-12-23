#ifndef ASIP2_HPP
#define ASIP2_HPP

#include "asip.hpp"

class ASIP2 : public ASIP {
  Q_OBJECT
public:
  explicit ASIP2(QNetworkAccessManager& networkAccessManager,const QString& serverURL,QObject* const parent=nullptr,Data startingData=Data());
  virtual std::unique_ptr<ASIP> create(QNetworkAccessManager& networkAccessManager,const QString& serverURL,QObject* const parent=nullptr,Data startingData=Data()) const override;
  virtual std::vector<GameListCategory> availableGameListCategories() const override;
  virtual void state() override;
private:
  std::vector<ASIP::GameInfo> getGameList(const QVariantList& games);
  virtual QByteArray getRequestData(const std::vector<std::pair<QString,QString> >& items) override;
  virtual Data getReplyData(const QByteArray& data) override;
};

#endif // ASIP2_HPP
