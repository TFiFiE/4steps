#ifndef ARIMAA_COM_HPP
#define ARIMAA_COM_HPP

class MainWindow;
#include "asip.hpp"

template<class ASIP>
class Arimaa_com : public ASIP {
public:
  explicit Arimaa_com(QNetworkAccessManager& networkAccessManager,const QString& serverURL,QObject* const parent=nullptr,typename ASIP::Data startingData=typename ASIP::Data());
  virtual std::unique_ptr<::ASIP> create(QNetworkAccessManager& networkAccessManager,const QString& serverURL,QObject* const parent=nullptr,typename ASIP::Data startingData=typename ASIP::Data()) const override;
  virtual typename ASIP::Data processReply(QNetworkReply& networkReply) override;
  virtual void enterGame(QObject* const requester,const QString& gameID,const Side side,const std::function<void(QNetworkReply*)> networkReplyAction) override;
  virtual std::unique_ptr<::ASIP> getGame(QNetworkReply& networkReply) override;
  virtual std::unique_ptr<QWidget> siteWidget(Server& server) override;
  static bool isBotName(const QString& playerName);
private:
  QUrl serverDirectory() const;
  bool possiblyDerateable(const QString& gameID) const;
  template<class Function> bool doMeDependingAction(Function action);
  std::unique_ptr<QWidget> chatroomWidget();
  std::unique_ptr<QWidget> botWidget(MainWindow& mainWindow);

  bool ratedMode;
  std::map<typename ASIP::GameListCategory,std::set<QString> > fixedGames;
};

#endif // ARIMAA_COM_HPP
