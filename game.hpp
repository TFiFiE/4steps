#ifndef GAME_HPP
#define GAME_HPP

#include <memory>
#include <QMainWindow>
#include <QDockWidget>
#include <QTreeView>
#include <QTimer>
#include <QAction>
#include <QCheckBox>
#include <QPushButton>
class ASIP;
#include "treemodel.hpp"
#include "board.hpp"
#include "playerbar.hpp"
#include "offboard.hpp"

class Game : public QMainWindow {
  Q_OBJECT
public:
  explicit Game(Globals& globals_,const Side viewpoint,QWidget* const parent=nullptr,const std::shared_ptr<ASIP> session_=std::shared_ptr<ASIP>(),const bool customSetup=false);
private:
  void addDockWidget(const Qt::DockWidgetArea area,QDockWidget& dockWidget,const Qt::Orientation orientation,const bool before);
  void setWindowState();
  void addGameMenu(const bool controllable);
  void addBoardMenu();
  void addControlsMenu(const bool controllable);
  void addDockMenu();
  void moveContextMenu(const QPoint pos);
  void addCornerWidget();
  void initLiveGame();
  void saveDockStates();
  virtual bool event(QEvent* event) override;
  virtual bool eventFilter(QObject* watched,QEvent* event) override;
  static std::array<bool,NUM_SIDES> getControllableSides(const std::shared_ptr<ASIP> session);
  static std::vector<qint64> getTickTimes();
  void setPlayerBars(const bool southIsUp);
  void flipGalleries();
  void synchronize(const bool hard);
  void updateTimes();
  qint64 updateCornerMessage();
  void soundTicker(const Side sideToMove,const qint64 timeLeft);
  void setExploration(const bool on);
  bool processMoves(const std::pair<GameTree,size_t>& treeAndNumber,const Side role,const Result& result,const bool hardSynchronization);
  void receiveGameTree(const GameTree& gameTreeNode,const bool sound);
  void receiveNodeChange(const NodePtr& newNode);
  void processVisibleNode(const NodePtr& node);
  void synchronizeWithMoveCell(const QModelIndex& current);
  void updateMoveList();
  QPersistentModelIndex getCurrentIndex() const;
  std::pair<NodePtr,int> getNodeAndColumn() const;
  void setCurrentIndex(const QModelIndex& index);
  void announceResult(const Result& result);

  Globals& globals;
  const std::shared_ptr<ASIP> session;
  GameTree gameTree;
  TreeModel treeModel;
  NodePtr liveNode;
  Board board;
  enum {FIRST_GALLERY_INDEX=NUM_SIDES,MOVE_LIST_INDEX=2*NUM_SIDES,NUM_DOCK_WIDGETS};
  QDockWidget dockWidgets[NUM_DOCK_WIDGETS];
  bool dockWidgetResized;
  PlayerBar playerBars[NUM_SIDES];
  std::array<OffBoard,NUM_SIDES> galleries;
  QTreeView treeView;
  QTimer timer,ticker;
  size_t processedMoves;
  int nextTickTime;
  bool finished;
  bool moveSynchronization;

  QAction forceUpdate,resign,fullScreen,rotate,autoRotate,animate,sound,stepMode,confirm,moveList;
  std::unique_ptr<QAction> offBoards[NUM_SIDES];
  QWidget cornerWidget;
  QHBoxLayout cornerLayout;
    QLabel cornerMessage;
    QCheckBox explore;
    QPushButton current;
  QActionGroup iconSets;
};

#endif // GAME_HPP
