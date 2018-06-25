#ifndef GAME_HPP
#define GAME_HPP

#include <memory>
#include <QMainWindow>
#include <QDockWidget>
#include <QTimer>
#include <QAction>
class ASIP;
#include "board.hpp"
#include "playerbar.hpp"

class Game : public QMainWindow {
  Q_OBJECT
public:
  explicit Game(Globals& globals_,const Side viewpoint,QWidget* const parent=nullptr,const std::shared_ptr<ASIP> session_=std::shared_ptr<ASIP>());
  void closeEvent(QCloseEvent*) override;
private:
  static std::array<bool,NUM_SIDES> getControllableSides(const std::shared_ptr<ASIP> session);
  static std::vector<qint64> getTickTimes();
  void setDockWidgets(const bool southIsUp);
  void synchronize(const bool hard);
  void updateTimes();
  qint64 updateCornerMessage();
  void soundTicker(const Side sideToMove,const qint64 timeLeft);
  bool processMoves(const std::pair<GameTreeNode,unsigned int>& treeAndNumber,const Side role,const Result& result,const bool hardSynchronization);
  void announceResult(const Result& result);

  Globals& globals;
  const std::shared_ptr<ASIP> session;
  Board board;
  QDockWidget dockWidgets[NUM_SIDES];
  PlayerBar playerBars[NUM_SIDES];
  QTimer timer,ticker;
  unsigned int processedMoves;
  int nextTickTime;
  bool finished;

  QAction forceUpdate,resign,fullScreen,rotate,autoRotate,sound,stepMode;
  QLabel cornerMessage;
};

#endif // GAME_HPP
