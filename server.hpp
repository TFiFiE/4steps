#ifndef SERVER_HPP
#define SERVER_HPP

#include <memory>
#include <QVBoxLayout>
#include <QPushButton>
#include "asip.hpp"
class MainWindow;
struct GameList;

class Server : public QWidget {
  Q_OBJECT
public:
  explicit Server(ASIP& session_,MainWindow& mainWindow_);
  ~Server();
  void refreshPage() const;

  ASIP& session;
  MainWindow& mainWindow;
private:
  void createGame();
  void resizeEvent(QResizeEvent* event) override;

  QVBoxLayout vBoxLayout;
    QHBoxLayout hBoxLayout;
      QPushButton newGame,refresh;

  std::map<ASIP::GameListCategory,std::unique_ptr<GameList> > gameLists;
};

#endif // SERVER_HPP
