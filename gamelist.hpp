#ifndef GAMELIST_HPP
#define GAMELIST_HPP

#include <QGridLayout>
#include <QLabel>
#include <QTableWidget>
struct Server;
#include "asip.hpp"

class GameList : public QGridLayout {
  Q_OBJECT
public:
  explicit GameList(Server* const server,const QString& labelText);
  void receiveGames(std::vector<ASIP::GameInfo> games_);
private:
  QLabel description,lastUpdated;
  QTableWidget tableWidget;
  std::vector<ASIP::GameInfo> games;
  friend class Server;
};

#endif // GAMELIST_HPP
