#include <QMouseEvent>
#include <QPainter>
#include "board.hpp"
#include "globals.hpp"
#include "messagebox.hpp"
#include "io.hpp"

Board::Board(Globals& globals_,const Side viewpoint,const bool soundOn,const std::array<bool,NUM_SIDES>& controllableSides_,QWidget* const parent,const Qt::WindowFlags f) :
  QWidget(parent,f),
  southIsUp(viewpoint==SECOND_SIDE),
  globals(globals_),
  afterCurrentStep(potentialMove.end()),
  controllableSides(controllableSides_),
  autoRotate(false),
  drag{NO_SQUARE,NO_SQUARE}
{
  qMediaPlayer.setPlaylist(&qMediaPlaylist);
  toggleSound(soundOn);
  initSetup();

  globals.settings.beginGroup("Board");
  const auto stepMode=globals.settings.value("step_mode").toBool();
  const auto iconSet=static_cast<PieceIcons::Set>(globals.settings.value("icon_set",PieceIcons::VECTOR).toInt());
  globals.settings.endGroup();
  setStepMode(stepMode);
  setIconSet(iconSet);
}

bool Board::setupPhase() const
{
  return currentNode.inSetup();
}

Side Board::sideToMove() const
{
  return currentNode.sideToMove();
}

MoveTree& Board::currentMoveNode() const
{
  return currentNode.getMoveNode();
}

std::vector<GameState> Board::history() const
{
  return currentNode.history();
}

const GameState& Board::gameState() const
{
  return setupPhase() ? potentialSetup :
                        (afterCurrentStep==potentialMove.begin() ? currentMoveNode().currentState : std::get<RESULTING_STATE>(*(afterCurrentStep-1)));
}

bool Board::playable() const
{
  return currentNode.result().endCondition==NO_END && controllableSides[sideToMove()];
}

void Board::receiveSetup(const Placement& placement,const bool sound)
{
  runtime_assert(setupPhase(),"Not in setup phase.");
  potentialSetup.add(placement);
  finalizeSetup(placement,sound);
}

void Board::receiveMove(const PieceSteps& pieceSteps,const bool sound)
{
  runtime_assert(!setupPhase(),"Still in setup phase.");
  const ExtendedSteps move=GameState(gameState()).takePieceSteps(pieceSteps);
  if (sound)
    playStepSounds(move,true);
  finalizeMove(move);
}

void Board::receiveGameTree(const GameTreeNode& gameTreeNode,const bool sound)
{
  currentNode=gameTreeNode;
  if (setupPhase()) {
    if (sound && sideToMove()==SECOND_SIDE)
      playSound("qrc:/finished-setup.wav");

    potentialSetup=currentNode.history().back();
    initSetup();
  }
  else {
    if (sound) {
      if (currentMoveNode().previousNode==nullptr)
        playSound("qrc:/finished-setup.wav");
      else
        playStepSounds(currentMoveNode().previousNode->branches.back().first,true);
    }
    potentialMove.clear();
    afterCurrentStep=potentialMove.end();
  }
  if (autoRotate)
    setViewpoint(sideToMove());
  boardChanged();
  refreshHighlights(true);
  update();
}

void Board::rotate()
{
    southIsUp=!southIsUp;
    refreshHighlights(false);
    update();
    boardRotated(southIsUp);
}

void Board::setViewpoint(const Side side)
{
  if ((side==SECOND_SIDE)!=southIsUp)
    rotate();
}

void Board::setAutoRotate(const bool on)
{
  if (on) {
    autoRotate=true;
    setViewpoint(sideToMove());
  }
  else
    autoRotate=false;
}

void Board::toggleSound(const bool soundOn_)
{
  soundOn=soundOn_;
  qMediaPlayer.setVolume(soundOn ? 100 : 0);
}

void Board::setStepMode(const bool newStepMode)
{
  stepMode=newStepMode;
  setMouseTracking(stepMode);
  if (refreshHighlights(true))
    update();

  globals.settings.beginGroup("Board");
  globals.settings.setValue("step_mode",stepMode.get());
  globals.settings.endGroup();
}

void Board::setIconSet(const PieceIcons::Set newIconSet)
{
  if (iconSet!=newIconSet) {
    iconSet=newIconSet;
    update();

    globals.settings.beginGroup("Board");
    globals.settings.setValue("icon_set",static_cast<int>(iconSet));
    globals.settings.endGroup();
  }
}

void Board::playSound(const QString& soundFile)
{
  qMediaPlaylist.clear();
  qMediaPlaylist.addMedia(QUrl(soundFile));
  qMediaPlayer.play();
}

void Board::setControllable(const std::array<bool,NUM_SIDES>& controllableSides_)
{
  controllableSides=controllableSides_;
  update();
}

int Board::squareWidth() const
{
  return width()/NUM_FILES;
}

int Board::squareHeight() const
{
  return height()/NUM_RANKS;
}

void Board::normalizeOrientation(unsigned int& file,unsigned int& rank) const
{
  if (southIsUp)
    file=NUM_FILES-1-file;
  else
    rank=NUM_RANKS-1-rank;
}

QRect Board::visualRectangle(unsigned int file,unsigned int rank) const
{
  normalizeOrientation(file,rank);
  return QRect(file*squareWidth(),rank*squareHeight(),squareWidth(),squareHeight());
}

SquareIndex Board::orientedSquare(unsigned int file,unsigned int rank) const
{
  normalizeOrientation(file,rank);
  return toSquare(file,rank);
}

SquareIndex Board::positionToSquare(const QPoint& position) const
{
  const int file=position.x()/squareWidth();
  const int rank=position.y()/squareHeight();
  return orientedSquare(file,rank);
}

int Board::closestAxisDirection(unsigned int value,const unsigned int size)
{
  value*=2;
  if (value<size)
    return -1;
  else if (value>size)
    return 1;
  else
    return 0;
}

SquareIndex Board::closestAdjacentSquare(const QPoint& position) const
{
  const int file=position.x()/squareWidth();
  const int rank=position.y()/squareHeight();
  const int localX=position.x()%squareWidth();
  const int localY=position.y()%squareHeight();
  const int  widthDirection=(file==0 ? 1 : (file==NUM_FILES-1 ? -1 : closestAxisDirection(localX,squareWidth())));
  const int heightDirection=(rank==0 ? 1 : (rank==NUM_RANKS-1 ? -1 : closestAxisDirection(localY,squareHeight())));
  const int distanceX=( widthDirection==1) ? squareWidth() -localX : localX;
  const int distanceY=(heightDirection==1) ? squareHeight()-localY : localY;
  const int diffDistance=distanceX*squareHeight()-distanceY*squareWidth();
  if (diffDistance<0) {
    if (widthDirection!=0)
      return orientedSquare(file+widthDirection,rank);
  }
  else if (diffDistance>0)
    if (heightDirection!=0)
      return orientedSquare(file,rank+heightDirection);
  return NO_SQUARE;
}

bool Board::setupPlacementPhase() const
{
  return setupPhase() && currentSetupPiece>=0;
}

bool Board::validDrop() const
{
  return setupPhase() ? (isSetupSquare(sideToMove(),drag[DESTINATION]) && drag[ORIGIN]!=drag[DESTINATION]) : !dragSteps.empty();
}

void Board::initSetup()
{
  std::copy_n(&numStartingPiecesPerType[1],NUM_PIECE_TYPES-1,numSetupPieces.begin());
  nextSetupPiece();
}

void Board::nextSetupPiece()
{
  currentSetupPiece=numSetupPieces.size()-1;
  while (true) {
    if (currentSetupPiece<0) {
      // Fill remaining squares with most numerous piece type.
      const PieceTypeAndSide piece=toPieceTypeAndSide(FIRST_PIECE_TYPE,sideToMove());
      for (SquareIndex square=FIRST_SQUARE;square<NUM_SQUARES;increment(square)) {
        PieceTypeAndSide& squarePiece=potentialSetup.currentPieces[square];
        if (squarePiece==NO_PIECE && isSetupSquare(sideToMove(),square))
          squarePiece=piece;
      }
      break;
    }
    if (numSetupPieces[currentSetupPiece]>0)
      break;
    --currentSetupPiece;
  }
}

bool Board::setUpPiece(const SquareIndex destination)
{
  if (isSetupSquare(sideToMove(),destination)) {
    auto& currentPieces=potentialSetup.currentPieces;
    if (std::all_of(currentPieces.begin(),currentPieces.end(),[](const PieceTypeAndSide& piece){return piece==NO_PIECE;}))
      gameStarted();
    PieceTypeAndSide& currentPiece=currentPieces[destination];
    if (currentPiece!=NO_PIECE)
      ++numSetupPieces[toPieceType(currentPiece)-1];
    currentPiece=toPieceTypeAndSide(static_cast<PieceType>(currentSetupPiece+1),sideToMove());
    --numSetupPieces[currentSetupPiece];
    nextSetupPiece();
    return true;
  }
  else
    return false;
}

bool Board::refreshHighlights(const bool clearSelected)
{
  if (stepMode && !setupPlacementPhase())
    return updateStepHighlights();
  else {
    bool change=(highlighted[DESTINATION]!=NO_SQUARE);
    highlighted[DESTINATION]=NO_SQUARE;
    if (clearSelected) {
      change|=(highlighted[ORIGIN]!=NO_SQUARE);
      highlighted[ORIGIN]=NO_SQUARE;
    }
    return change;
  }
}

bool Board::updateStepHighlights()
{
  return updateStepHighlights(mapFromGlobal(QCursor::pos()));
}

bool Board::updateStepHighlights(const QPoint& mousePosition)
{
  const std::array<SquareIndex,2> oldSelected=highlighted;
  highlighted[ORIGIN]=positionToSquare(mousePosition);
  highlighted[DESTINATION]=closestAdjacentSquare(mousePosition);
  if (setupPhase()) {
    if (!isSetupSquare(sideToMove(),highlighted[ORIGIN]))
      fill(highlighted,NO_SQUARE);
    else if (!isSetupSquare(sideToMove(),highlighted[DESTINATION]))
      highlighted[DESTINATION]=NO_SQUARE;
  }
  else {
    Squares legalDestinations=gameState().legalDestinations(highlighted[ORIGIN]);
    if (legalDestinations.empty()) {
      legalDestinations=gameState().legalDestinations(highlighted[DESTINATION]);
      if (legalDestinations.empty())
        fill(highlighted,NO_SQUARE);
      else
        std::swap(highlighted[ORIGIN],highlighted[DESTINATION]);
    }
    if (!found(legalDestinations,highlighted[DESTINATION]))
      highlighted[DESTINATION]=NO_SQUARE;
  }
  return oldSelected!=highlighted;
}

bool Board::singleSquareAction(const SquareIndex square)
{
  if (setupPlacementPhase()) {
    if (setUpPiece(square)) {
      refreshHighlights(true);
      return true;
    }
  }
  else if (stepMode)
    return doubleSquareAction(highlighted[ORIGIN],highlighted[DESTINATION]);
  else {
    if (highlighted[ORIGIN]==NO_SQUARE) {
      if (setupPhase() ? isSetupSquare(sideToMove(),square) : gameState().legalOrigin(square)) {
        highlighted[ORIGIN]=square;
        return true;
      }
    }
    else if (highlighted[ORIGIN]==square) {
      highlighted[ORIGIN]=NO_SQUARE;
      return true;
    }
    else
      return doubleSquareAction(highlighted[ORIGIN],square);
  }
  return false;
}

bool Board::doubleSquareAction(const SquareIndex origin,const SquareIndex destination)
{
  if (origin==NO_SQUARE || destination==NO_SQUARE)
    return false;
  else if (setupPhase())
    return doubleSquareSetupAction(origin,destination);
  else {
    const ExtendedSteps route=gameState().preferredRoute(origin,destination);
    if (!route.empty()) {
      doSteps(route);
      return true;
    }
  }
  return false;
}

bool Board::doubleSquareSetupAction(const SquareIndex origin,const SquareIndex destination)
{
  if (isSetupSquare(sideToMove(),destination)) {
    GameState::Board& currentPieces=potentialSetup.currentPieces;
    std::swap(currentPieces[origin],currentPieces[destination]);
    refreshHighlights(true);
    return true;
  }
  else
    return false;
}

void Board::doSteps(const ExtendedSteps& steps)
{
  if (!steps.empty()) {
    potentialMove.erase(afterCurrentStep,potentialMove.end());
    append(potentialMove,steps);
    afterCurrentStep=potentialMove.end();
    boardChanged();
    refreshHighlights(true);
    playStepSounds(steps,false);
  }
}

void Board::finalizeSetup(const Placement& placement,const bool sound)
{
  if (sound)
    playSound("qrc:/finished-setup.wav");

  if (sideToMove()==FIRST_SIDE) {
    potentialSetup.sideToMove=SECOND_SIDE;
    initSetup();
  }
  currentNode.addSetup(placement);

  if (autoRotate)
    setViewpoint(sideToMove());
  boardChanged();
  refreshHighlights(true);
  update();
}

void Board::finalizeMove(const ExtendedSteps& move)
{
  currentNode.makeMove(move);
  potentialMove.clear();
  afterCurrentStep=potentialMove.end();
  boardChanged();
  if (currentNode.result().endCondition==NO_END && autoRotate)
    setViewpoint(sideToMove());
  refreshHighlights(true);
  update();
}

void Board::endDrag()
{
  drag[ORIGIN]=NO_SQUARE;
  dragSteps.clear();
  update();
}

void Board::mousePressEvent(QMouseEvent* event)
{
  if (!playable()) {
    event->ignore();
    return;
  }
  switch (event->button()) {
    case Qt::LeftButton: {
      const SquareIndex square=positionToSquare(event->pos());
      if (setupPhase() ? isSide(gameState().currentPieces[square],sideToMove()) : gameState().legalOrigin(square)) {
        fill(drag,square);
        dragSteps.clear();
        update();
      }
    }
    break;
    case Qt::RightButton:
      if (drag[ORIGIN]!=NO_SQUARE)
        endDrag();
      else if (!stepMode && highlighted[ORIGIN]!=NO_SQUARE) {
        highlighted[ORIGIN]=NO_SQUARE;
        update();
      }
      else if (afterCurrentStep!=potentialMove.begin()) {
        assert(!setupPhase());
        --afterCurrentStep;
        boardChanged();
        refreshHighlights(true);
        update();
      }
    break;
    case Qt::MidButton:
      if (afterCurrentStep!=potentialMove.end()) {
        assert(!setupPhase());
        ++afterCurrentStep;
        boardChanged();
        refreshHighlights(true);
        update();
      }
    break;
    default:
      event->ignore();
      return;
    break;
  }
  event->accept();
}

void Board::mouseMoveEvent(QMouseEvent* event)
{
  if (!playable()) {
    event->ignore();
    return;
  }
  event->accept();
  if ((event->buttons()&Qt::LeftButton)!=0) {
    if (drag[ORIGIN]!=NO_SQUARE) {
      const SquareIndex square=positionToSquare(event->pos());
      if (drag[DESTINATION]!=square) {
        drag[DESTINATION]=square;
        if (drag[ORIGIN]==drag[DESTINATION])
          dragSteps.clear();
        else if (!setupPhase())
          dragSteps=gameState().preferredRoute(drag[ORIGIN],drag[DESTINATION],dragSteps);
      }
      update();
    }
  }
  if (refreshHighlights(false))
    update();
}

void Board::mouseReleaseEvent(QMouseEvent* event)
{
  if (!playable()) {
    event->ignore();
    return;
  }
  switch (event->button()) {
    case Qt::LeftButton: {
      const SquareIndex eventSquare=positionToSquare(event->pos());
      if (drag[ORIGIN]!=NO_SQUARE) {
        assert(eventSquare==drag[DESTINATION]);
        if (drag[ORIGIN]==eventSquare)
          singleSquareAction(eventSquare);
        else if (setupPhase())
          doubleSquareSetupAction(drag[ORIGIN],eventSquare);
        else
          doSteps(dragSteps);
        endDrag();
      }
      else if (singleSquareAction(eventSquare))
        update();
    }
    break;
    default:
      event->ignore();
      return;
    break;
  }
  event->accept();
}

void Board::mouseDoubleClickEvent(QMouseEvent* event)
{
  if (!playable()) {
    event->ignore();
    return;
  }
  switch (event->button()) {
    case Qt::LeftButton: {
      if (setupPhase()) {
        if (currentSetupPiece<0 && !isSetupSquare(sideToMove(),positionToSquare(event->pos()))) {
          const Placement placement=gameState().placement(sideToMove());
          sendSetup(placement,sideToMove());
          finalizeSetup(placement,true);
        }
      }
      else if (gameState().currentPieces[positionToSquare(event->pos())]==NO_PIECE && (!stepMode || found(highlighted,NO_SQUARE))) {
        const ExtendedSteps playedMove(potentialMove.cbegin(),afterCurrentStep);
        switch (currentMoveNode().legalMove(gameState())) {
          case LEGAL:
            sendMove(playedMove,sideToMove());
            playSound("qrc:/loud-step.wav");
            finalizeMove(playedMove);
          break;
          case ILLEGAL_PUSH_INCOMPLETION:
            playSound("qrc:/illegal-move.wav");
            (new MessageBox<230>(QMessageBox::Critical,tr("Incomplete push"),tr("Push was not completed."),QMessageBox::NoButton,this))->show();
          break;
          case ILLEGAL_PASS:
            playSound("qrc:/illegal-move.wav");
            (new MessageBox<183>(QMessageBox::Critical,tr("Illegal pass"),tr("Move did not change board."),QMessageBox::NoButton,this))->show();
          break;
          case ILLEGAL_REPETITION:
            playSound("qrc:/illegal-move.wav");
            (new MessageBox<>(QMessageBox::Critical,tr("Illegal repetition"),tr("Move would repeat board and side to move too often."),QMessageBox::NoButton,this))->show();
          break;
        }
      }
    }
    break;
    case Qt::RightButton: {
      if (afterCurrentStep!=potentialMove.begin()) {
        assert(!setupPhase());
        afterCurrentStep=potentialMove.begin();
        boardChanged();
        refreshHighlights(true);
        update();
      }
    }
    break;
    case Qt::MidButton:
      if (afterCurrentStep!=potentialMove.end()) {
        assert(!setupPhase());
        afterCurrentStep=potentialMove.end();
        boardChanged();
        refreshHighlights(true);
        update();
      }
    break;
    default:
      event->ignore();
      return;
    break;
  }
  event->accept();
}

void Board::keyPressEvent(QKeyEvent* event)
{
  switch (event->key()) {
    default:
      event->ignore();
      return;
    break;
  }
  event->accept();
}

void Board::focusOutEvent(QFocusEvent*)
{
  endDrag();
}

void Board::paintEvent(QPaintEvent*)
{
  const QColor sideColors[NUM_SIDES]={Qt::white,Qt::black};
  QPainter qPainter(this);

  qreal factor=squareHeight()/static_cast<qreal>(qPainter.fontMetrics().height());
  for (SquareIndex square=FIRST_SQUARE;square<NUM_SQUARES;increment(square))
    if (isTrap(square))
      factor=std::min(factor,squareWidth()/static_cast<qreal>(qPainter.fontMetrics().width(toCoordinates(square,'A').begin())));
  if (factor>0) {
    QFont qFont(qPainter.font());
    qFont.setPointSizeF(qFont.pointSizeF()*factor);
    qPainter.setFont(qFont);
  }

  QPen qPen(Qt::SolidPattern,1);
  const QPoint mousePosition=mapFromGlobal(QCursor::pos());
  for (unsigned int pass=0;pass<2;++pass)
    for (SquareIndex square=FIRST_SQUARE;square<NUM_SQUARES;increment(square)) {
      const unsigned int file=toFile(square);
      const unsigned int rank=toRank(square);
      const bool isTrapSquare=isTrap(file,rank);
      if (drag[ORIGIN]==NO_SQUARE ? found(highlighted,square) || (gameState().inPush && square==gameState().followupDestination)
                                  : validDrop() && square==drag[DESTINATION]) {
        if (pass==0)
          continue;
        else {
          qPainter.setBrush(sideColors[sideToMove()]);
          qPen.setColor(sideColors[!sideToMove()]);
        }
      }
      else if (pass==1)
        continue;
      else {
        if (!setupPhase() && (square==drag[ORIGIN] || found<ORIGIN>(dragSteps,square)))
          qPainter.setBrush(sideToMove()==FIRST_SIDE ? QColor(0xC9,0xC9,0xC9) : QColor(0x23,0x23,0x23));
        else if (setupPhase() ? playable() && isSetupRank(sideToMove(),rank) : gameState().stepsAvailable==MAX_STEPS_PER_MOVE && currentMoveNode().changedSquare(square))
          qPainter.setBrush(sideColors[!sideToMove()]);
        else {
          if (isTrapSquare)
            qPainter.setBrush(rank<NUM_RANKS/2 ? QColor(0xF6,0x66,0x10) : QColor(0x42,0x00,0xF2));
          else
            qPainter.setBrush(QColor(0x89,0x65,0x7B));
        }
        qPen.setColor(sideColors[sideToMove()]);
      }
      qPainter.setPen(qPen);
      const QRect qRect=visualRectangle(file,rank);
      qPainter.drawRect(qRect);
      if (isTrapSquare) {
        if (currentNode.result().endCondition!=NO_END)
          qPainter.setPen(QColor(0x89,0x65,0x7B));
        qPainter.drawText(qRect,Qt::AlignCenter,toCoordinates(file,rank,'A').begin());
      }
      if (square!=drag[ORIGIN]) {
        const PieceTypeAndSide pieceOnSquare=gameState().currentPieces[square];
        if (pieceOnSquare!=NO_PIECE)
          globals.pieceIcons.drawPiece(qPainter,iconSet,pieceOnSquare,qRect);
      }
    }
  if (playable() && setupPlacementPhase())
    globals.pieceIcons.drawPiece(qPainter,iconSet,toPieceTypeAndSide(static_cast<PieceType>(currentSetupPiece+1),sideToMove()),QRect((NUM_FILES-1)/2.0*squareWidth(),(NUM_RANKS-1)/2.0*squareHeight(),squareWidth(),squareHeight()));
  if (drag[ORIGIN]!=NO_SQUARE)
    globals.pieceIcons.drawPiece(qPainter,iconSet,gameState().currentPieces[drag[ORIGIN]],QRect(mousePosition.x()-squareWidth()/2,mousePosition.y()-squareHeight()/2,squareWidth(),squareHeight()));
  qPainter.end();
}

void Board::playStepSounds(const ExtendedSteps& steps,const bool emphasize)
{
  qMediaPlaylist.clear();
  for (const auto& step:steps) {
    const auto trappedPiece=std::get<TRAPPED_PIECE>(step);
    if (trappedPiece!=NO_PIECE) {
      if (toSide(trappedPiece)==std::get<RESULTING_STATE>(step).sideToMove)
        qMediaPlaylist.addMedia(QUrl("qrc:/dropped-piece.wav"));
      else
        qMediaPlaylist.addMedia(QUrl("qrc:/captured-piece.wav"));
    }
  }
  if (qMediaPlaylist.isEmpty()) {
    if (emphasize)
      qMediaPlaylist.addMedia(QUrl("qrc:/loud-step.wav"));
    else
      qMediaPlaylist.addMedia(QUrl("qrc:/soft-step.wav"));
  }
  qMediaPlayer.play();
}
