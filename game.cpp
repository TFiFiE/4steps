#include <QMenuBar>
#include <QApplication>
#include <QScreen>
#include <QHeaderView>
#include "game.hpp"
#include "globals.hpp"
#include "mainwindow.hpp"
#include "asip.hpp"
#include "messagebox.hpp"
#include "offboard.hpp"
#include "io.hpp"

Game::Game(Globals& globals_,const Side viewpoint,QWidget* const parent,const std::shared_ptr<ASIP> session_,const bool customSetup) :
  QMainWindow(parent),
  globals(globals_),
  session(session_),
  gameTree(customSetup ? GameTree() : Node::createTree()),
  treeModel(gameTree.empty() ? nullptr : gameTree.front()),
  liveNode(session==nullptr ? nullptr : treeModel.root),
  board(globals,treeModel.root,session==nullptr,viewpoint,session!=nullptr,{session==nullptr,session==nullptr}),
  dockWidgetResized(false),
  galleries{{{board,FIRST_SIDE},{board,SECOND_SIDE}}},
  processedMoves(0),
  nextTickTime(-1),
  finished(false),
  moveSynchronization(true),
  forceUpdate("Force server &update"),
  resign(tr("&Resign")),
  fullScreen(tr("&Full screen")),
  rotate(tr("&Rotate")),
  autoRotate(tr("&Auto-rotate")),
  animate(tr("Animate &moves")),
  sound(tr("&Sound")),
  stepMode(tr("&Step mode")),
  confirm(tr("&Confirm move")),
  offBoard(tr("&Pieces off board")),
  moveList(tr("&Move list")),
  explore(tr("&Explore")),
  current(tr("&Current")),
  iconSets(this)
{
  setCentralWidget(&board);
  setDockOptions(QMainWindow::AllowNestedDocks);

  const auto controllableSides=getControllableSides(session);
  const bool controllable=(controllableSides[FIRST_SIDE] || controllableSides[SECOND_SIDE]);
  addGameMenu(controllable);
  addBoardMenu();
  addControlsMenu(controllable);
  addDockMenu();
  addCornerWidget();
  setWindowState();
  connect(&board,&Board::sendNodeChange,this,&Game::receiveNodeChange);
  initLiveGame();

  setAttribute(Qt::WA_DeleteOnClose);
  show();
}

void Game::addDockWidget(const Qt::DockWidgetArea area,QDockWidget& dockWidget,const Qt::Orientation orientation,const bool before)
{
  QMainWindow::addDockWidget(area,&dockWidget,orientation);
  if (before)
    for (const auto& child:findChildren<QDockWidget*>())
      if (dockWidgetArea(child)==area && child!=&dockWidget)
        QMainWindow::addDockWidget(area,child,orientation);
}

void Game::addGameMenu(const bool controllable)
{
  const auto gameMenu=menuBar()->addMenu(tr("&Game"));

  forceUpdate.setEnabled(session!=nullptr);
  forceUpdate.setShortcut(QKeySequence(Qt::CTRL+Qt::Key_U));
  if (session!=nullptr)
    connect(&forceUpdate,&QAction::triggered,session.get(),&ASIP::forceUpdate);
  gameMenu->addAction(&forceUpdate);

  resign.setEnabled(session!=nullptr && controllable);
  resign.setShortcut(QKeySequence(Qt::CTRL+Qt::Key_R));
  connect(&resign,&QAction::triggered,[=]{
    MessageBox messageBox(QMessageBox::Question,tr("Confirm resignation"),tr("Are you sure you want to resign?"),QMessageBox::Yes|QMessageBox::No,this);
    messageBox.setDefaultButton(QMessageBox::No);
    if (messageBox.exec()==QMessageBox::Yes)
      session->resign();
  });
  gameMenu->addAction(&resign);
}

void Game::addBoardMenu()
{
  const auto boardMenu=menuBar()->addMenu(tr("&Board"));

  fullScreen.setShortcut(QKeySequence(Qt::Key_F));
  connect(&fullScreen,&QAction::triggered,&board,[=]{
    if (windowState()==Qt::WindowMaximized)
      showNormal();
    else
      showMaximized();
  });
  boardMenu->addAction(&fullScreen);

  rotate.setShortcut(QKeySequence(Qt::Key_R));
  connect(&rotate,&QAction::triggered,&board,&Board::rotate);
  boardMenu->addAction(&rotate);

  autoRotate.setCheckable(true);
  autoRotate.setShortcut(QKeySequence(Qt::CTRL+Qt::Key_A));
  connect(&autoRotate,&QAction::toggled,&board,&Board::setAutoRotate);
  boardMenu->addAction(&autoRotate);

  animate.setEnabled(session!=nullptr);
  animate.setCheckable(true);
  animate.setChecked(board.animate);
  animate.setShortcut(QKeySequence(Qt::Key_M));
  connect(&animate,&QAction::toggled,&board,&Board::setAnimate);
  boardMenu->addAction(&animate);

  sound.setCheckable(true);
  sound.setChecked(board.soundOn);
  sound.setShortcut(QKeySequence(Qt::Key_S));
  connect(&sound,&QAction::toggled,&board,&Board::toggleSound);
  boardMenu->addAction(&sound);

  const auto iconMenu=boardMenu->addMenu(tr("&Piece icons"));
  for (const auto& iconSet:{tr("&Vector"),tr("&Raster")}) {
    const auto& action=iconMenu->addAction(iconSet);
    iconSets.addAction(action);
    action->setCheckable(true);
  }
  iconSets.actions()[board.iconSet]->setChecked(true);
  connect(&iconSets,&QActionGroup::triggered,&board,[this](QAction* const action) {
    board.setIconSet(static_cast<PieceIcons::Set>(iconSets.actions().indexOf(action)));
    for (auto& gallery:galleries)
      gallery.update();
  });
}

void Game::addControlsMenu(const bool controllable)
{
  const auto controlsMenu=menuBar()->addMenu(tr("&Controls"));

  stepMode.setEnabled(controllable);
  stepMode.setCheckable(true);
  stepMode.setChecked(board.stepMode);
  stepMode.setShortcut(QKeySequence(Qt::CTRL+Qt::Key_S));
  connect(&stepMode,&QAction::toggled,&board,&Board::setStepMode);
  controlsMenu->addAction(&stepMode);

  confirm.setEnabled(session!=nullptr);
  confirm.setCheckable(true);
  confirm.setChecked(board.confirm);
  confirm.setShortcut(QKeySequence(Qt::CTRL+Qt::Key_C));
  connect(&confirm,&QAction::toggled,&board,&Board::setConfirm);
  controlsMenu->addAction(&confirm);
}

void Game::addDockMenu()
{
  const auto dockMenu=menuBar()->addMenu(tr("&Docks"));

  treeView.setModel(&treeModel);
  treeView.setHeaderHidden(true);
  treeView.setAnimated(false);
  treeView.header()->setMinimumSectionSize(0);
  treeView.header()->setStretchLastSection(false);
  treeView.header()->setSectionResizeMode(QHeaderView::ResizeToContents);
  treeView.setIndentation(0);
  treeView.setSelectionBehavior(QAbstractItemView::SelectItems);
  connect(treeView.selectionModel(),&QItemSelectionModel::currentChanged,this,&Game::synchronizeWithMoveCell);
  connect(&board,&Board::boardChanged,this,&Game::updateMoveList);
  updateMoveList();

  for (Side side=FIRST_SIDE;side<NUM_SIDES;increment(side)) {
    auto& dockWidget=dockWidgets[FIRST_GALLERY_INDEX+side];
    dockWidget.setWindowTitle(tr("%1 pieces off board").arg(sideWord(side)));
    dockWidget.setWidget(&galleries[side]);
    dockWidget.setObjectName(dockWidget.windowTitle());
    const auto dockWidgetArea=((side==FIRST_SIDE)==board.southIsUp ? Qt::TopDockWidgetArea : Qt::BottomDockWidgetArea);
    addDockWidget(dockWidgetArea,dockWidget,Qt::Vertical,dockWidgetArea==Qt::BottomDockWidgetArea);
    connect(&offBoard,&QAction::toggled,&dockWidget,&QDockWidget::setVisible);
  }
  connect(&board,&Board::boardRotated,this,&Game::flipGalleries);
  offBoard.setCheckable(true);
  offBoard.setChecked(true);
  offBoard.setShortcut(QKeySequence(Qt::CTRL+Qt::Key_P));
  dockMenu->addAction(&offBoard);

  auto& dockWidget=dockWidgets[MOVE_LIST_INDEX];
  dockWidget.setAllowedAreas(Qt::LeftDockWidgetArea|Qt::RightDockWidgetArea);
  dockWidget.setFeatures(QDockWidget::DockWidgetClosable|QDockWidget::DockWidgetMovable|QDockWidget::DockWidgetFloatable);
  addDockWidget(Qt::RightDockWidgetArea,dockWidget,Qt::Horizontal,false);
  dockWidget.setWidget(&treeView);
  dockWidget.setObjectName("Move list");

  moveList.setCheckable(true);
  moveList.setChecked(true);
  moveList.setShortcut(QKeySequence(Qt::CTRL+Qt::Key_M));
  connect(&moveList,&QAction::toggled,&dockWidget,&QDockWidget::setVisible);

  dockMenu->addAction(&moveList);
}

void Game::addCornerWidget()
{
  cornerWidget.setLayout(&cornerLayout);

  connect(&board,&Board::boardChanged,this,&Game::updateCornerMessage);
  cornerLayout.addWidget(&cornerMessage);

  explore.setChecked(board.explore);
  setExploration(board.explore);
  connect(&explore,&QCheckBox::toggled,this,&Game::setExploration);
  cornerLayout.addWidget(&explore);

  connect(&current,&QPushButton::clicked,this,[this] {
    const auto& currentNode=board.currentNode.get();
    if (currentNode==liveNode) {
      if (!board.potentialMove.get().empty())
        board.undoSteps(true);
    }
    else {
      const auto& child=liveNode->findClosestChild(currentNode);
      board.setNode(liveNode);
      if (child!=nullptr)
        board.proposeMove(*child.get(),0);
    }
  });
  cornerLayout.addWidget(&current);

  menuBar()->setCornerWidget(&cornerWidget);
}

void Game::setWindowState()
{
  globals.settings.beginGroup("Game");
  const auto size=globals.settings.value("size").toSize();
  const auto state=globals.settings.value("state");
  const auto orientation=globals.settings.value("orientation");
  globals.settings.endGroup();
  if (size.isValid())
    resize(size);
  else {
    const auto fullScreen=qApp->primaryScreen()->availableGeometry().size();
    resize(fullScreen.width()/3,fullScreen.height()/2);
  }
  if (state.isValid())
    restoreState(state.toByteArray());
  for (auto& dockWidget:dockWidgets) {
    dockWidget.setFloating(false);
    dockWidget.installEventFilter(this);
    connect(&dockWidget,&QDockWidget::dockLocationChanged,this,&Game::saveDockStates);
  }
  if (orientation!=board.southIsUp.get())
    flipGalleries();
}

void Game::initLiveGame()
{
  if (session!=nullptr) {
    connect(session.get(),&ASIP::error,this,[this](const std::exception& exception) {
      MessageBox(QMessageBox::Critical,tr("Error from game server"),exception.what(),QMessageBox::NoButton,this).exec();
    });
    connect(&board,&Board::gameStarted,session.get(),&ASIP::start);
    for (Side side=FIRST_SIDE;side<NUM_SIDES;increment(side)) {
      auto& dockWidget=dockWidgets[side];
      dockWidget.setWindowTitle(sideWord(side));
      dockWidget.setFeatures(QDockWidget::DockWidgetMovable|QDockWidget::DockWidgetFloatable|QDockWidget::DockWidgetVerticalTitleBar);
      dockWidget.setWidget(&playerBars[side]);
      dockWidget.setObjectName(dockWidget.windowTitle());
    }
    setPlayerBars(board.southIsUp);
    connect(&board,&Board::boardRotated,this,&Game::setPlayerBars);

    if (session->gameStateAvailable())
      synchronize(false);
    connect(session.get(),&ASIP::updated,this,&Game::synchronize);
    connect(session.get(),&ASIP::statusChanged,this,[this](const ASIP::Status oldStatus,const ASIP::Status newStatus) {
      QApplication::alert(this);
      if (oldStatus==ASIP::UNSTARTED && newStatus==ASIP::LIVE && !isActiveWindow())
        board.playSound("qrc:/game-start.wav");
    });
    connect(&timer,&QTimer::timeout,this,&Game::updateTimes);
    if (session->role()!=NO_SIDE) {
      ticker.setSingleShot(true);
      connect(&ticker,&QTimer::timeout,this,[&]{
        board.playSound("qrc:/clock-tick.wav");
        nextTickTime=-1;
        updateTimes();
      });
    }
  }
}

void Game::saveDockStates()
{
  globals.settings.beginGroup("Game");
  globals.settings.setValue("state",saveState());
  globals.settings.setValue("orientation",board.southIsUp.get());
  globals.settings.endGroup();
}

bool Game::event(QEvent* event)
{
  switch (event->type()) {
    case QEvent::WindowActivate:
      globals.settings.beginGroup("Game");
      globals.settings.setValue("size",normalGeometry().size());
      globals.settings.endGroup();
    break;
    case QEvent::MouseButtonRelease:
      if (dockWidgetResized) {
        saveDockStates();
        dockWidgetResized=false;
      }
    break;
    case QEvent::Wheel:
      if (QCoreApplication::sendEvent(&board,event))
        return true;
    break;
    default: break;
  }
  return QMainWindow::event(event);
}

bool Game::eventFilter(QObject* watched,QEvent* event)
{
  switch (event->type()) {
    case QEvent::Resize:
      for (const auto& dockWidget:dockWidgets)
        if (watched==&dockWidget) {
          dockWidgetResized=true;
          break;
        }
    break;
    case QEvent::Close:
      if (watched==&dockWidgets[MOVE_LIST_INDEX])
        moveList.setChecked(false);
      else
        for (Side side=FIRST_SIDE;side<NUM_SIDES;increment(side))
          if (watched==&dockWidgets[FIRST_GALLERY_INDEX+side]) {
            dockWidgets[FIRST_GALLERY_INDEX+otherSide(side)].hide();
            offBoard.setChecked(false);
            break;
          }
    break;
    default: break;
  }
  return QMainWindow::eventFilter(watched,event);
}

std::array<bool,NUM_SIDES> Game::getControllableSides(const std::shared_ptr<ASIP> session)
{
  if (session==nullptr)
    return {true,true};
  else
    switch (session->role()) {
      case FIRST_SIDE:
        return {true,false};
      case SECOND_SIDE:
        return {false,true};
      default:
        return {false,false};
    }
}

std::vector<qint64> Game::getTickTimes()
{
  std::vector<qint64> tickTimes;
  for (unsigned int quarterSeconds=1;quarterSeconds<10;++quarterSeconds)
    tickTimes.push_back(quarterSeconds*250);
  for (unsigned int halfSeconds=5;halfSeconds<10;++halfSeconds)
    tickTimes.push_back(halfSeconds*500);
  for (unsigned int seconds=5;seconds<10;++seconds)
    tickTimes.push_back(seconds*1000);
  for (unsigned int decaseconds=1;decaseconds<=3;++decaseconds)
    tickTimes.push_back(decaseconds*10000);
  return tickTimes;
}

void Game::setPlayerBars(const bool southIsUp)
{
  for (Side side=FIRST_SIDE;side<NUM_SIDES;increment(side)) {
    auto& dockWidget=dockWidgets[side];
    const auto dockWidgetArea=((side==FIRST_SIDE)==southIsUp ? Qt::TopDockWidgetArea : Qt::BottomDockWidgetArea);
    dockWidget.setAllowedAreas(dockWidgetArea);
    addDockWidget(dockWidgetArea,dockWidget,Qt::Vertical,dockWidgetArea==Qt::TopDockWidgetArea);
  }
}

void Game::flipGalleries()
{
  for (Side side=FIRST_SIDE;side<NUM_SIDES;increment(side)) {
    auto& dockWidget=dockWidgets[FIRST_GALLERY_INDEX+side];
    const auto area=dockWidgetArea(&dockWidget);
    const auto flippableDockWidgetAreas=Qt::TopDockWidgetArea|Qt::BottomDockWidgetArea;
    if ((area&flippableDockWidgetAreas)!=0) {
      const auto newArea=(area==Qt::TopDockWidgetArea ? Qt::BottomDockWidgetArea : Qt::TopDockWidgetArea);
      addDockWidget(newArea,dockWidget,Qt::Vertical,newArea==Qt::TopDockWidgetArea);
    }
  }
}

void Game::synchronize(const bool hard)
{
  const auto status=session->getStatus();
  const auto role=session->role();
  switch (status) {
    case ASIP::OPEN:
      board.setControllable({false,false});
    break;
    case ASIP::UNSTARTED:
    case ASIP::LIVE:
      board.setControllable({FIRST_SIDE==role,SECOND_SIDE==role});
    break;
    case ASIP::FINISHED:
      board.setControllable({true,true});
    break;
  }
  const auto players=session->getPlayers();
  const auto sideToMove=(status==ASIP::LIVE ? session->sideToMove() : NO_SIDE);
  for (Side side=FIRST_SIDE;side<NUM_SIDES;increment(side)) {
    playerBars[side].player.setText(players[side]);
    playerBars[side].setActive(side==sideToMove);
  }
  const auto moves=session->getMoves(treeModel.root);
  auto result=session->getResult();
  if (processMoves(moves,role,result,hard))
    nextTickTime=-1;
  updateTimes();

  const auto technicalResult=moves.first.front()->result;
  if (technicalResult.endCondition!=NO_END && technicalResult!=result) {
    assert(technicalResult.winner!=NO_SIDE);
    if (otherSide(technicalResult.winner)==role) {
      if (result.endCondition==NO_END)
        session->resign();
      result=technicalResult;
    }
    else if (result.endCondition!=NO_END)
      result=technicalResult;
  }

  if (!finished && result.endCondition!=NO_END) {
    finished=true;
    resign.setEnabled(false);
    announceResult(result);
  }
}

void Game::updateTimes()
{
  auto nextChange=updateCornerMessage();
  const auto times=session->getTimes();
  for (Side side=FIRST_SIDE;side<NUM_SIDES;increment(side))
    playerBars[side].setTimes(times[side]);
  if (session->getStatus()==ASIP::LIVE) {
    const Side sideToMove=session->sideToMove();
    nextChange=std::min(nextChange,playerBars[sideToMove].nextChange.get());
    soundTicker(sideToMove,std::get<0>(times[sideToMove]));
  }
  else
    ticker.stop();
  if (nextChange==std::numeric_limits<qint64>::max())
    timer.stop();
  else
    timer.start(nextChange);
}

qint64 Game::updateCornerMessage()
{
  auto nextChange=std::numeric_limits<qint64>::max();
  if (session!=nullptr && session->getStatus()!=ASIP::FINISHED) {
    const auto timeSinceLastReply=session->timeSinceLastReply();
    cornerMessage.setText(tr("Last server response: ")+PlayerBar::timeDisplay(timeSinceLastReply));
    nextChange=1000-timeSinceLastReply%1000;
  }
  else
    cornerMessage.setText("");
  menuBar()->setCornerWidget(&cornerWidget);
  return nextChange;
}

void Game::soundTicker(const Side sideToMove,const qint64 timeLeft)
{
  if (session->role()==sideToMove) {
    const std::vector<qint64> tickTimes=getTickTimes();
    for (int index=static_cast<int>(tickTimes.size()-1);index>=0;--index) {
      const auto tickTime=tickTimes[index];
      if (tickTime<nextTickTime)
        break;
      else {
        const auto nextTick=timeLeft-tickTime;
        if (nextTick>=0) {
          ticker.start(nextTick);
          nextTickTime=tickTime;
          return;
        }
      }
    }
  }
  else
    ticker.stop();
}

void Game::setExploration(const bool on)
{
  board.setExploration(on);
  if (on)
    treeModel.exclusive=QModelIndex();
  else if (liveNode==nullptr)
    treeModel.exclusive=treeView.currentIndex();
  else {
    const auto& index=treeModel.lastIndex(liveNode);
    treeModel.exclusive=index;
    const auto& currentNode=board.currentNode.get();
    if (currentNode!=liveNode) {
      const auto& child=liveNode->findClosestChild(currentNode);
      board.setNode(liveNode);
      if (child!=nullptr && session->role()==board.sideToMove())
        board.proposeMove(*child.get(),child->move.size());
    }
  }
  emit treeModel.layoutChanged();
  current.setEnabled(on && liveNode!=nullptr);
}

bool Game::processMoves(const std::pair<GameTree,size_t>& treeAndNumber,const Side role,const Result& result,const bool hardSynchronization)
{
  const auto& receivedTree=treeAndNumber.first;
  const size_t sessionMoves=treeAndNumber.second;
  const auto& serverNode=*receivedTree.front();
  const int movesAhead=serverNode.numMovesBefore(liveNode.get());
  if (movesAhead==0)
    return false;
  else if (movesAhead>0 && !hardSynchronization && sessionMoves+movesAhead==processedMoves) {
    if (result.endCondition==NO_END)
      return false;
    else
      receiveGameTree(receivedTree,false);
  }
  else
    receiveGameTree(receivedTree,role!=otherSide(serverNode.currentState.sideToMove));
  processedMoves=sessionMoves;
  return true;
}

void Game::receiveGameTree(const GameTree& gameTreeNode,const bool sound)
{
  liveNode=gameTreeNode.front();
  const int movesBehind=gameTree.front()->numMovesBefore(liveNode.get());
  int insertPos;
  if (movesBehind>=0) {
    if (movesBehind>0)
      gameTree.front()=liveNode;
    insertPos=1;
  }
  else
    insertPos=0;
  gameTree.insert(gameTree.begin()+insertPos,gameTreeNode.begin()+insertPos,gameTreeNode.end());
  gameTree.erase(unsorted_unique(gameTree.begin(),gameTree.end()),gameTree.end());

  const auto oldNode=board.currentNode.get();
  const bool changed=board.setNode(liveNode,sound,true);
  explore.setChecked(false);
  if (session->role()==board.sideToMove()) {
    if (!changed)
      board.undoSteps(true);
    else if (const auto& child=liveNode->findClosestChild(oldNode))
      board.proposeMove(*child.get(),0);
  }
  processVisibleNode(liveNode);
}

void Game::receiveNodeChange(const NodePtr& newNode)
{
  Node::addToTree(gameTree,newNode);

  if (session!=nullptr) {
    if (!board.explore) {
      liveNode=newNode;
      session->sendMove(QString::fromStdString(newNode->toString()));
      ++processedMoves;
      nextTickTime=-1;
    }
  }
  if (treeModel.root==nullptr)
    treeModel.root=Node::root(newNode);

  processVisibleNode(newNode);
}

void Game::processVisibleNode(const NodePtr& node)
{
  const auto& ancestors=node->ancestors();
  for (auto ancestor=ancestors.rbegin();ancestor!=ancestors.rend();++ancestor)
    treeView.expand(treeModel.index(ancestor->lock(),0));
  const auto& index=treeModel.lastIndex(node);
  if (!board.explore)
    treeModel.exclusive=index;
  setCurrentIndex(index);
  emit treeModel.layoutChanged();
}

void Game::synchronizeWithMoveCell(const QModelIndex& current)
{
  if (moveSynchronization) {
    moveSynchronization=false;
    const auto node=treeModel.getItem(current);
    const auto column=current.column();
    if (current.siblingAtColumn(column+1)==QModelIndex()) {
      board.setNode(node);
      if (!node->inSetup())
        if (const auto& child=node->child(0)) {
          const auto& move=child->move;
          board.doSteps(move,false,move.size());
        }
    }
    else {
      board.setNode(node->previousNode);
      board.proposeMove(*node.get(),column);
    }
    moveSynchronization=true;
  }
}

void Game::updateMoveList()
{
  const auto& node=board.currentNode;
  auto& dockWidget=dockWidgets[MOVE_LIST_INDEX];
  std::string moveString;
  if (node.get()==nullptr) {
    for (Side side=FIRST_SIDE;side<NUM_SIDES;increment(side)) {
      const auto& placementString=toString(board.gameState().placements(side));
      if (!placementString.empty()) {
        if (!moveString.empty())
          moveString+=' ';
        moveString+=placementString;
      }
    }
  }
  else
    moveString=toPlyString(node->depth,*treeModel.root)+' '+board.tentativeMoveString();
  dockWidget.setWindowTitle(QString::fromStdString(moveString));

  if (moveSynchronization)
    setCurrentIndex(getCurrentIndex(node));
}

QPersistentModelIndex Game::getCurrentIndex(const NodePtr& node) const
{
  if (node.get()==nullptr)
    return QPersistentModelIndex();
  else if (node->inSetup()) {
    const auto& currentPlacements=board.currentPlacements();
    if (!currentPlacements.empty())
      if (const auto& child=node->findPartialMatchingChild(currentPlacements).first) {
        const auto& childPlacements=child->currentState.playedPlacements();
        const auto pair=mismatch(currentPlacements.begin(),currentPlacements.end(),childPlacements.begin());
        return treeModel.index(child,pair.second==childPlacements.end() ? childPlacements.size()-numStartingPiecesPerType[0] : distance(childPlacements.begin(),pair.second));
      }
  }
  else {
    const auto& potentialMove=board.potentialMove.get();
    const auto& currentSteps=potentialMove.current();
    NodePtr child;
    for (const auto& steps:{potentialMove.all(),currentSteps})
      if (!steps.empty() && (child=node->findPartialMatchingChild(steps).first)!=nullptr)
        return treeModel.index(child,currentSteps.size());
  }
  return treeModel.index(node,treeModel.columnCount(node->move.size())-1);
}

void Game::setCurrentIndex(const QModelIndex& index)
{
  moveSynchronization=false;
  treeView.setCurrentIndex(index);
  moveSynchronization=true;
  treeView.scrollTo(index.siblingAtColumn(0));
}

void Game::announceResult(const Result& result)
{
  if (result.winner==NO_SIDE || (session!=nullptr && otherSide(result.winner)==session->role()))
    board.playSound("qrc:/game-loss.wav");
  else
    board.playSound("qrc:/game-win.wav");

  const QString winner=sideWord(result.winner);
  const QString loser=sideWord(otherSide(result.winner));
  const QString title=(result.winner==NO_SIDE ? tr("No winner") : tr("%1 wins!").arg(winner));

  QString message;
  switch (result.endCondition) {
    case GOAL:               message=tr("%1 rabbit reached final row.").arg(winner); break;
    case IMMOBILIZATION:     message=tr("%1 has no legal moves.").arg(loser); break;
    case ELIMINATION:        message=tr("All %1 rabbits eliminated.").arg(loser); break;
    case TIME_OUT:           message=tr("%1 ran out of time.").arg(loser); break;
    case RESIGNATION:        message=tr("%1 resigned.").arg(loser); break;
    case ILLEGAL_MOVE:       message=tr("%1 played an illegal move.").arg(loser); break;
    case FORFEIT:            message=tr("%1 forfeited.").arg(loser); break;
    case ABANDONMENT:        message=tr("Neither player present when time ran out.\nGame abandoned."); break;
    case SCORE:              message=tr("Global time limit ran out.\n%1 wins by score.").arg(winner); break;
    case REPETITION:         message=tr("%1 repeated board and side to move too many times.").arg(loser); break;
    case MUTUAL_ELIMINATION: message=tr("All rabbits eliminated."); break;
    default: assert(false);
  }

  const auto messageBox=new MessageBox(QMessageBox::Information,title,message,QMessageBox::NoButton,this);
  messageBox->setWindowModality(Qt::NonModal);
  messageBox->show();
}
