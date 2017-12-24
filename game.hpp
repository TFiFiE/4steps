#ifndef GAME_HPP
#define GAME_HPP

#include <memory>
#include <QMainWindow>
#include <QDockWidget>
#include <QTimer>
#include <QAction>
class MainWindow;
class ASIP;
#include "board.hpp"
#include "playerbar.hpp"

class Game : public QMainWindow {
  Q_OBJECT
public:
  explicit Game(MainWindow& mainWindow_,const Side viewpoint,const std::shared_ptr<ASIP> session_=std::shared_ptr<ASIP>());
private:
  static std::array<bool,NUM_SIDES> getControllableSides(const std::shared_ptr<ASIP> session);
  static std::vector<qint64> getTickTimes();
  void setDockWidgets(const bool southIsUp);
  void synchronize();
  void updateTimes();
  qint64 updateCornerMessage();
  void soundTicker(const Side sideToMove,const qint64 timeLeft);
  bool processMoves(const std::pair<GameTreeNode,unsigned int>& treeAndNumber,const Side role,const Result& result);
  void announceResult(const Result& result);

  const std::shared_ptr<ASIP> session;
  Board board;
  QDockWidget dockWidgets[2];
  PlayerBar playerBars[2];
  QTimer timer,ticker;
  unsigned int processedMoves;
  int nextTickTime;
  bool finished;

  QAction resign,fullScreen,rotate,autoRotate,sound,stepMode;
  QLabel cornerMessage;
};

#endif // GAME_HPP
