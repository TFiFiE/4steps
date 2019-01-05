#ifndef SERVER_HPP
#define SERVER_HPP

#include <memory>
#include <QVBoxLayout>
#include <QPushButton>
#include "asip.hpp"
struct Globals;
class MainWindow;
class GameList;

class Server : public QWidget {
  Q_OBJECT
public:
  explicit Server(Globals& globals_,ASIP& session_,MainWindow& mainWindow_);
  ~Server();
  void refreshPage() const;
  void enterGame(const ASIP::GameInfo& game,const Side role,const Side viewpoint);
  void addGame(QNetworkReply& networkReply,const Side viewpoint,const bool guaranteedUnique=false) const;

  ASIP& session;
  MainWindow& mainWindow;
private:
  void resizeEvent(QResizeEvent* event) override;

  Globals& globals;

  QVBoxLayout vBoxLayout;
    QHBoxLayout hBoxLayout;
      QPushButton newGame,openGame,refresh;

  std::map<ASIP::GameListCategory,std::unique_ptr<GameList> > gameLists;
};

#endif // SERVER_HPP
