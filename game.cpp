#include <QMenuBar>
#include <QApplication>
#include <QScreen>
#include "game.hpp"
#include "globals.hpp"
#include "mainwindow.hpp"
#include "asip.hpp"
#include "messagebox.hpp"
#include "io.hpp"

Game::Game(Globals& globals_,const Side viewpoint,QWidget* const parent,const std::shared_ptr<ASIP> session_) :
  QMainWindow(parent),
  globals(globals_),
  session(session_),
  board(globals,viewpoint,{session==nullptr,session==nullptr}),
  dockWidgetResized(false),
  processedMoves(0),
  nextTickTime(-1),
  finished(false),
  forceUpdate("Force server &update"),
  resign(tr("&Resign")),
  fullScreen(tr("&Full screen")),
  rotate(tr("&Rotate")),
  autoRotate(tr("&Auto-rotate")),
  sound(tr("&Sound")),
  stepMode(tr("&Step mode"))
{
  setCentralWidget(&board);
  const auto gameMenu=menuBar()->addMenu(tr("&Game"));

  forceUpdate.setEnabled(session!=nullptr);
  forceUpdate.setShortcut(QKeySequence(Qt::CTRL+Qt::Key_U));
  if (session!=nullptr)
    connect(&forceUpdate,&QAction::triggered,session.get(),&ASIP::forceUpdate);
  gameMenu->addAction(&forceUpdate);

  const auto controllableSides=getControllableSides(session);
  const bool controllable=(controllableSides[FIRST_SIDE] || controllableSides[SECOND_SIDE]);
  resign.setEnabled(session!=nullptr && controllable);
  resign.setShortcut(QKeySequence(Qt::CTRL+Qt::Key_R));
  connect(&resign,&QAction::triggered,[=]{
    const QMessageBox::StandardButton result=QMessageBox::question(this,tr("Confirm resignation"),tr("Are you sure you want to resign?"),QMessageBox::Yes|QMessageBox::No,QMessageBox::No);
    if (result==QMessageBox::Yes)
      session->resign();
  });
  gameMenu->addAction(&resign);

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

  sound.setCheckable(true);
  sound.setChecked(true);
  sound.setShortcut(QKeySequence(Qt::Key_S));
  connect(&sound,&QAction::toggled,&board,&Board::toggleSound);
  boardMenu->addAction(&sound);

  const auto controlsMenu=menuBar()->addMenu(tr("&Controls"));

  stepMode.setEnabled(controllable);
  stepMode.setCheckable(true);
  stepMode.setChecked(board.stepMode);
  stepMode.setShortcut(QKeySequence(Qt::CTRL+Qt::Key_S));
  connect(&stepMode,&QAction::toggled,&board,&Board::setStepMode);
  controlsMenu->addAction(&stepMode);

  menuBar()->setCornerWidget(&cornerMessage);
  connect(&board,&Board::boardChanged,this,&Game::updateCornerMessage);

  globals.settings.beginGroup("Game");
  const auto size=globals.settings.value("size").toSize();
  const auto state=globals.settings.value("state");
  globals.settings.endGroup();
  if (size.isValid())
    resize(size);
  else {
    const auto fullScreen=qApp->primaryScreen()->availableGeometry().size();
    resize(fullScreen.width()/3,fullScreen.height()/2);
  }
  if (state.isValid())
    restoreState(state.toByteArray());
  else
    restoreState(saveState()); // QT BUG: https://bugreports.qt.io/browse/QTBUG-65592

  if (session!=nullptr) {
    connect(&board,&Board::gameStarted,session.get(),&ASIP::start);
    for (Side side=FIRST_SIDE;side<NUM_SIDES;increment(side)) {
      auto& dockWidget=dockWidgets[side];
      dockWidget.setWindowTitle(side==FIRST_SIDE ? tr("Gold") : tr("Silver"));
      dockWidget.setFeatures(QDockWidget::DockWidgetMovable|QDockWidget::DockWidgetFloatable|QDockWidget::DockWidgetVerticalTitleBar);
      dockWidget.setWidget(&playerBars[side]);
      dockWidget.installEventFilter(this);
      dockWidget.setObjectName(dockWidget.windowTitle());
    }
    setDockWidgets(board.southIsUp);
    connect(&board,&Board::boardRotated,this,&Game::setDockWidgets);

    connect(&board,&Board::sendSetup,this,[=](const Placement& placement) {
      assert(placement==toPlacement(toString(placement)));
      session->sendMove(QString::fromStdString(toString(placement)));
      ++processedMoves;
      nextTickTime=-1;
    });
    connect(&board,&Board::sendMove,this,[=](const ExtendedSteps& move) {
      session->sendMove(QString::fromStdString(toString(move)));
      ++processedMoves;
      nextTickTime=-1;
    });
    if (session->gameStateAvailable())
      synchronize(false);
    connect(session.get(),&ASIP::updated,this,&Game::synchronize);
    connect(session.get(),&ASIP::statusChanged,this,[this](const ASIP::Status oldStatus,const ASIP::Status newStatus) {
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
  setAttribute(Qt::WA_DeleteOnClose);
  show();
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
        globals.settings.beginGroup("Game");
        globals.settings.setValue("state",saveState());
        globals.settings.endGroup();
        dockWidgetResized=false;
      }
    break;
    default: break;
  }
  return QMainWindow::event(event);
}

bool Game::eventFilter(QObject* watched,QEvent* event)
{
  if (event->type()==QEvent::Resize)
    for (const auto& dockWidget:dockWidgets)
      if (watched==&dockWidget)
        dockWidgetResized=true;
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

void Game::setDockWidgets(const bool southIsUp)
{
  for (Side side=FIRST_SIDE;side<NUM_SIDES;increment(side)) {
    auto& dockWidget=dockWidgets[side];
    const auto dockWidgetArea=((side==FIRST_SIDE)==southIsUp ? Qt::TopDockWidgetArea : Qt::BottomDockWidgetArea);
    dockWidget.setAllowedAreas(dockWidgetArea);
    addDockWidget(dockWidgetArea,&dockWidget);
  }
}

void Game::synchronize(const bool hard)
{
  const auto status=session->getStatus();
  const auto role=session->role();
  if (status==ASIP::UNSTARTED || status==ASIP::LIVE)
    board.setControllable({FIRST_SIDE==role,SECOND_SIDE==role});
  else
    board.setControllable({false,false});
  const auto players=session->getPlayers();
  const auto sideToMove=(status==ASIP::LIVE ? session->sideToMove() : NO_SIDE);
  for (Side side=FIRST_SIDE;side<NUM_SIDES;increment(side)) {
    playerBars[side].player.setText(players[side]);
    playerBars[side].setActive(side==sideToMove);
  }
  const auto moves=session->getMoves();
  auto result=session->getResult();
  if (processMoves(moves,role,result,hard))
    nextTickTime=-1;
  updateTimes();

  const auto technicalResult=moves.first.result();
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
  if (board.playable() && !board.setupPhase())
    cornerMessage.setText(tr("Steps left: ")+QString::number(board.gameState().stepsAvailable));
  else if (session!=nullptr && session->getStatus()!=ASIP::FINISHED) {
    const auto timeSinceLastReply=session->timeSinceLastReply();
    cornerMessage.setText(tr("Last server response: ")+PlayerBar::timeDisplay(timeSinceLastReply));
    nextChange=1000-timeSinceLastReply%1000;
  }
  else
    cornerMessage.setText("");
  menuBar()->setCornerWidget(&cornerMessage);
  return nextChange;
}

void Game::soundTicker(const Side sideToMove,const qint64 timeLeft)
{
  if (session->role()==sideToMove) {
    const std::vector<qint64> tickTimes=getTickTimes();
    for (int index=tickTimes.size()-1;index>=0;--index) {
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

bool Game::processMoves(const std::pair<GameTreeNode,unsigned int>& treeAndNumber,const Side role,const Result& result,const bool hardSynchronization)
{
  const auto& receivedTree=treeAndNumber.first;
  const unsigned int sessionMoves=treeAndNumber.second;
  const auto& receivedHistory=receivedTree.history();
  const auto& localHistory=board.history();
  bool sound=(role!=otherSide(receivedTree.sideToMove()));
  if (search(localHistory.begin(),localHistory.end(),receivedHistory.begin(),receivedHistory.end())==localHistory.begin()) {
    const unsigned int movesAhead=localHistory.size()-receivedHistory.size();
    if (movesAhead==0)
      return false;
    else if (!hardSynchronization && sessionMoves+movesAhead==processedMoves) {
      if (result.endCondition!=NO_END)
        sound=false;
      else
        return false;
    }
  }
  board.receiveGameTree(receivedTree,sound);
  processedMoves=sessionMoves;
  return true;
}

void Game::announceResult(const Result& result)
{
  if (result.winner==NO_SIDE || (session!=nullptr && otherSide(result.winner)==session->role()))
    board.playSound("qrc:/game-loss.wav");
  else
    board.playSound("qrc:/game-win.wav");

  const QString winner=(result.winner==FIRST_SIDE ? tr("Gold") : tr("Silver"));
  const QString  loser=(result.winner==FIRST_SIDE ? tr("Silver") : tr("Gold"));
  const QString title=(result.winner==NO_SIDE ? tr("No winner.") : winner+tr(" wins!"));

  QString message;
  switch (result.endCondition) {
    case GOAL:               message=winner+tr(" rabbit reached final row."); break;
    case IMMOBILIZATION:     message=loser+tr(" has no legal moves."); break;
    case ELIMINATION:        message=tr("All ")+loser+tr(" rabbits eliminated."); break;
    case TIME_OUT:           message=loser+tr(" ran out of time."); break;
    case RESIGNATION:        message=loser+tr(" resigned."); break;
    case ILLEGAL_MOVE:       message=loser+tr(" played an illegal move."); break;
    case FORFEIT:            message=loser+tr(" forfeited."); break;
    case ABANDONMENT:        message=tr("Neither player present when time ran out. Game abandoned."); break;
    case SCORE:              message=tr("Global time limit ran out. ")+winner+tr(" wins by score."); break;
    case REPETITION:         message=loser+tr(" repeated board and side to move too many times."); break;
    case MUTUAL_ELIMINATION: message=tr("All rabbits eliminated."); break;
    default: assert(false);
  }

  const auto messageBox=new MessageBox<187>(QMessageBox::Information,title,message,QMessageBox::NoButton,this);
  messageBox->setWindowModality(Qt::NonModal);
  messageBox->show();
}
