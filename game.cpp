#include <QMenuBar>
#include <QApplication>
#include <QScreen>
#include <QHeaderView>
#include <QClipboard>
#include <QMouseEvent>
#include "game.hpp"
#include "globals.hpp"
#include "mainwindow.hpp"
#include "asip.hpp"
#include "messagebox.hpp"
#include "offboard.hpp"
#include "io.hpp"

using namespace std;
Game::Game(Globals& globals_,const Side viewpoint,QWidget* const parent,const std::shared_ptr<ASIP> session_,const std::unique_ptr<GameState> customSetup) :
  QMainWindow(parent),
  globals(globals_),
  session(session_),
  gameTree(customSetup==nullptr ? Node::createTree() : GameTree()),
  treeModel(gameTree.empty() ? nullptr : gameTree.front()),
  liveNode(session==nullptr ? nullptr : treeModel.root),
  board(globals,treeModel.root,session==nullptr,viewpoint,session!=nullptr,{session==nullptr,session==nullptr},customSetup.get(),parent),
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
  moveList(tr("&Move list")),
  offBoards{make_unique<QAction>(tr("&Gold pieces off board")),
            make_unique<QAction>(tr("&Silver pieces off board"))},
  explore(tr("&Explore")),
  current(tr("&Current")),
  iconSets(this)
{
  setCentralWidget(&board);
  setDockOptions(QMainWindow::AllowNestedDocks);

  setWindowState();
  const auto controllableSides=getControllableSides(session);
  const bool controllable=(controllableSides[FIRST_SIDE] || controllableSides[SECOND_SIDE]);
  addGameMenu(controllable);
  addBoardMenu();
  addInputMenu();
  addDockMenu();
  addCornerWidget();
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

void Game::addInputMenu()
{
  const auto inputMenu=menuBar()->addMenu(tr("&Input"));

  stepMode.setCheckable(true);
  stepMode.setChecked(board.stepMode);
  stepMode.setShortcut(QKeySequence(Qt::CTRL+Qt::Key_S));
  connect(&stepMode,&QAction::toggled,&board,&Board::setStepMode);
  inputMenu->addAction(&stepMode);

  confirm.setEnabled(session!=nullptr);
  confirm.setCheckable(true);
  confirm.setChecked(board.confirm);
  confirm.setShortcut(QKeySequence(Qt::CTRL+Qt::Key_C));
  connect(&confirm,&QAction::toggled,&board,&Board::setConfirm);
  inputMenu->addAction(&confirm);
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
  treeView.viewport()->installEventFilter(this);
  treeView.setContextMenuPolicy(Qt::CustomContextMenu);
  connect(&treeView,&QTreeView::customContextMenuRequested,this,&Game::moveContextMenu);
  connect(treeView.selectionModel(),&QItemSelectionModel::currentChanged,this,&Game::synchronizeWithMoveCell);
  connect(&board,&Board::boardChanged,this,&Game::updateMoveList);
  updateMoveList();

  for (Side side=FIRST_SIDE;side<NUM_SIDES;increment(side)) {
    auto& dockWidget=dockWidgets[FIRST_GALLERY_INDEX+side];
    dockWidget.setObjectName(tr("%1 pieces off board").arg(sideWord(side)));
    dockWidget.setWindowTitle(dockWidget.objectName());
    dockWidget.setWidget(&galleries[side]);
    const auto dockWidgetArea=((side==FIRST_SIDE)==board.southIsUp ? Qt::TopDockWidgetArea : Qt::BottomDockWidgetArea);
    addDockWidget(dockWidgetArea,dockWidget,Qt::Vertical,dockWidgetArea==Qt::BottomDockWidgetArea);

    const auto offBoard=offBoards[side].get();
    connect(offBoard,&QAction::triggered,&dockWidget,&QDockWidget::setVisible);
    connect(&dockWidget,&QDockWidget::visibilityChanged,offBoard,&QAction::setChecked);
    offBoard->setCheckable(true);
    offBoard->setChecked(true);
    offBoard->setShortcut(QKeySequence(Qt::CTRL+(side==FIRST_SIDE ? Qt::Key_1 : Qt::Key_2)));
    dockMenu->addAction(offBoard);
  }
  connect(&board,&Board::boardRotated,this,&Game::flipGalleries);

  auto& dockWidget=dockWidgets[MOVE_LIST_INDEX];
  dockWidget.setObjectName("Move list");
  dockWidget.setAllowedAreas(Qt::LeftDockWidgetArea|Qt::RightDockWidgetArea);
  dockWidget.setFeatures(QDockWidget::DockWidgetClosable|QDockWidget::DockWidgetMovable|QDockWidget::DockWidgetFloatable);
  addDockWidget(Qt::RightDockWidgetArea,dockWidget,Qt::Horizontal,false);
  dockWidget.setWidget(&treeView);

  moveList.setCheckable(true);
  moveList.setChecked(true);
  moveList.setShortcut(QKeySequence(Qt::CTRL+Qt::Key_M));
  connect(&moveList,&QAction::triggered,&dockWidget,&QDockWidget::setVisible);
  connect(&dockWidget,&QDockWidget::visibilityChanged,&moveList,&QAction::setChecked);

  dockMenu->addAction(&moveList);
}

void Game::moveContextMenu(const QPoint pos)
{
  const QModelIndex index=treeView.indexAt(pos);
  if (!index.isValid())
    return;
  const auto node=treeModel.getItem(index);
  const std::weak_ptr<Node> weakNode=node;
  auto menu=new QMenu(&treeView);
  menu->setAttribute(Qt::WA_DeleteOnClose);

  const auto copyMove=new QAction(tr("Copy move to clipboard"),menu);
  const auto nodeString=QString::fromStdString(node->toString());
  connect(copyMove,&QAction::triggered,this,[nodeString] {
    QGuiApplication::clipboard()->setText(nodeString);
  });
  menu->addAction(copyMove);

  const auto copySequence=new QAction(tr("Copy sequence to clipboard"),menu);
  const auto cursorNode=getNodeAndColumn().first;
  std::vector<std::weak_ptr<Node> > sequence;
  if (node->isAncestorOfOrSameAs(cursorNode.get())) {
    sequence.emplace_back(cursorNode);
    append(sequence,cursorNode->ancestors(node.get()));
  }
  else if (cursorNode->isAncestorOfOrSameAs(node.get())) {
    sequence.emplace_back(node);
    append(sequence,node->ancestors(cursorNode.get()));
  }
  if (sequence.empty())
    copySequence->setEnabled(false);
  else {
    std::string sequenceString;
    for (auto move=sequence.rbegin();move!=sequence.rend();++move) {
      if (move!=sequence.rbegin())
        sequenceString+='\n';
      const auto sequenceNode=move->lock();
      sequenceString+=sequenceNode->toPlyString()+' '+sequenceNode->toString();
    }
    connect(copySequence,&QAction::triggered,this,[sequenceString] {
      QGuiApplication::clipboard()->setText(QString::fromStdString(sequenceString));
    });
  }
  menu->addAction(copySequence);

  const auto childIndex=node->childIndex();
  for (unsigned int direction=0;direction<2;++direction) {
    const auto shiftDirection=new QAction(direction==0 ? tr("Shift up") : tr("Shift down"),menu);
    if (childIndex>=0 && (direction==0 ? childIndex>0 : childIndex<node->previousNode->numChildren()-1)) {
      connect(shiftDirection,&QAction::triggered,this,[this,weakNode,direction] {
        if (const auto& node=weakNode.lock()) {
          node->previousNode->swapChildren(*node.get(),direction==0 ? -1 : 1);
          treeModel.reset();
          treeView.setCurrentIndex(getCurrentIndex());
          emit treeModel.layoutChanged();
        }
      });
    }
    else
      shiftDirection->setEnabled(false);
    menu->addAction(shiftDirection);
  }

  const auto deleteMove=new QAction(tr("Delete move"),menu);
  if (liveNode==nullptr || !node->isAncestorOfOrSameAs(liveNode.get())) {
    connect(deleteMove,&QAction::triggered,this,[this,weakNode] {
      if (auto node=weakNode.lock()) {
        const bool changeNode=node->isAncestorOfOrSameAs(board.currentNode.get().get());
        const auto parent=Node::deleteFromTree(gameTree,node);
        node.reset();
        treeModel.reset();
        if (parent!=nullptr) {
          if (changeNode) {
            board.setNode(parent);
            if (!parent->inSetup())
              if (const auto& child=parent->child(0)) {
                const auto& move=child->move;
                board.doSteps(move,false,move.size());
              }
          }
          else
            treeView.setCurrentIndex(getCurrentIndex());
          emit treeModel.layoutChanged();
        }
      }
    });
  }
  else
    deleteMove->setEnabled(false);
  menu->addAction(deleteMove);

  menu->popup(treeView.viewport()->mapToGlobal(pos));
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

  if (liveNode==nullptr)
    current.setEnabled(false);
  else
    connect(&current,&QPushButton::clicked,this,[this] {
      if (liveNode!=nullptr) {
        const auto& currentNode=board.currentNode.get();
        if (currentNode==liveNode) {
          expandToNode(*liveNode);
          treeView.scrollTo(treeModel.index(liveNode,0));
          if (!board.potentialMove.get().empty())
            board.undoSteps(true);
        }
        else {
          const auto& child=liveNode->findClosestChild(currentNode);
          board.setNode(liveNode);
          if (child!=nullptr)
            board.proposeMove(*child.get(),0);
        }
      }
    });
  cornerLayout.addWidget(&current);

  menuBar()->setCornerWidget(&cornerWidget);
}

void Game::initLiveGame()
{
  if (session!=nullptr) {
    connect(session.get(),&ASIP::error,this,[this](const std::exception& exception) {
      MessageBox(QMessageBox::Critical,tr("Error from game server"),exception.what(),QMessageBox::NoButton,this).exec();
    });
    for (Side side=FIRST_SIDE;side<NUM_SIDES;increment(side)) {
      auto& dockWidget=dockWidgets[side];
      dockWidget.setObjectName(sideWord(side));
      dockWidget.setWindowTitle(dockWidget.objectName());
      dockWidget.setFeatures(QDockWidget::DockWidgetMovable|QDockWidget::DockWidgetFloatable|QDockWidget::DockWidgetVerticalTitleBar);
      dockWidget.setWidget(&playerBars[side]);
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
      connect(&board,&Board::gameStarted,session.get(),&ASIP::start);

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

void Game::mousePressEvent(QMouseEvent* event)
{
  switch (event->button()) {
    case Qt::RightButton: {
      auto menu=new QMenu(this);
      menu->setAttribute(Qt::WA_DeleteOnClose);

      const auto showMove=new QAction(tr("Show last move"),menu);
      if (board.customSetup() || board.currentNode->isGameStart())
        showMove->setEnabled(false);
      else
        connect(showMove,&QAction::triggered,this,[this]{board.animateMove(true);});
      menu->addAction(showMove);

      const auto fromClipboard=new QAction(tr("Play from clipboard"),menu);
      if (QGuiApplication::clipboard()->text().isEmpty())
        fromClipboard->setEnabled(false);
      else
        connect(fromClipboard,&QAction::triggered,this,[this]{processInput(QGuiApplication::clipboard()->text().toStdString());});
      menu->addAction(fromClipboard);

      const auto customGame=new QAction(tr("Start custom setup with current position"),menu);
      connect(customGame,&QAction::triggered,this,[this] {
        using namespace std;
        new Game(globals,board.southIsUp ? SECOND_SIDE : FIRST_SIDE,parentWidget(),nullptr,make_unique<GameState>(board.gameState()));
      });
      menu->addAction(customGame);

      menu->popup(QCursor::pos());
    }
    break;
    case Qt::MidButton:
      processInput(QGuiApplication::clipboard()->text().toStdString());
    break;
    default:
      event->ignore();
      return;
    break;
  }
  event->accept();
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
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonDblClick:
      if (watched==treeView.viewport() && static_cast<QMouseEvent*>(event)->button()==Qt::RightButton)
        return true;
    break;
    case QEvent::Resize:
      for (const auto& dockWidget:dockWidgets)
        if (watched==&dockWidget) {
          dockWidgetResized=true;
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

  const auto technicalResult=get<0>(moves).front()->result;
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
}

void Game::processInput(const std::string& input)
{
  try {
    if (board.customSetup())
      board.proposeSetup(customizedGameState(input,board.gameState()));
    else {
      NodePtr tentativeChild=board.tentativeChildNode();
      const auto& startingNode=(tentativeChild==nullptr ? board.currentNode.get() : tentativeChild);
      auto moves=toTree(input,startingNode,tentativeChild!=nullptr);
      auto& addedGameTree=get<0>(moves);
      const auto nodeChanges=get<1>(moves);
      if (nodeChanges>1 || startingNode!=addedGameTree.front()) {
        if (get<2>(moves))
          tentativeChild=nullptr;
        else {
          tentativeChild=std::make_shared<Node>(nullptr,addedGameTree.front()->move,addedGameTree.front()->currentState); // detach from parent
          addedGameTree.front()=addedGameTree.front()->previousNode;
        }
        append(gameTree,addedGameTree);
        gameTree.erase(unsorted_unique(gameTree.begin(),gameTree.end()),gameTree.end());
        explore.setChecked(true);
        const auto& newNode=addedGameTree.front();
        board.setNode(newNode);
        expandToNode(*newNode);
        if (tentativeChild!=nullptr)
          board.proposeMove(*tentativeChild.get(),tentativeChild->move.size());
      }
    }
  }
  catch (const std::exception& exception) {
    MessageBox(QMessageBox::Critical,"Illegal input",exception.what(),QMessageBox::NoButton,this).exec();
  }
}

bool Game::processMoves(const std::tuple<GameTree,size_t,bool>& moves,const Side role,const Result& result,const bool hardSynchronization)
{
  const auto& receivedTree=get<0>(moves);
  const size_t sessionMoves=get<1>(moves);
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

  if (board.explore) {
    board.playMoveSounds(*liveNode);
    expandToNode(*liveNode);
  }
  else {
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

void Game::expandToNode(const Node& node)
{
  const auto& ancestors=node.ancestors();
  for (auto ancestor=ancestors.rbegin();ancestor!=ancestors.rend();++ancestor)
    treeView.expand(treeModel.index(ancestor->lock(),0));
}

void Game::processVisibleNode(const NodePtr& node)
{
  expandToNode(*node);
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
    setCurrentIndex(getCurrentIndex());
}

QPersistentModelIndex Game::getCurrentIndex() const
{
  if (board.currentNode.get()==nullptr)
    return QPersistentModelIndex();
  else {
    const auto nodeAndColumn=getNodeAndColumn();
    return treeModel.index(nodeAndColumn.first,nodeAndColumn.second);
  }
}

std::pair<NodePtr,int> Game::getNodeAndColumn() const
{
  const auto& node=board.currentNode;
  if (node->inSetup()) {
    const auto& currentPlacements=board.currentPlacements();
    if (!currentPlacements.empty())
      if (const auto& child=node->findPartialMatchingChild(currentPlacements).first) {
        const auto& childPlacements=child->currentState.playedPlacements();
        const auto pair=mismatch(currentPlacements.begin(),currentPlacements.end(),childPlacements.begin());
        return {child,pair.second==childPlacements.end() ? childPlacements.size()-numStartingPiecesPerType[0] : distance(childPlacements.begin(),pair.second)};
      }
  }
  else {
    const auto& potentialMove=board.potentialMove.get();
    const auto& currentSteps=potentialMove.current();
    NodePtr child;
    for (const auto& steps:{potentialMove.all(),currentSteps})
      if (!steps.empty() && (child=node->findPartialMatchingChild(steps).first)!=nullptr)
        return {child,currentSteps.size()};
  }
  return {node,treeModel.columnCount(node->move.size())-1};
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
