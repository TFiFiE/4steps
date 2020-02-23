#include <QMouseEvent>
#include <QPainter>
#include "board.hpp"
#include "globals.hpp"
#include "messagebox.hpp"
#include "io.hpp"
#include "popup.hpp"

Board::Board(Globals& globals_,NodePtr currentNode_,const bool explore_,const Side viewpoint,const bool soundOn,const std::array<bool,NUM_SIDES>& controllableSides_,QWidget* const parent,const Qt::WindowFlags f) :
  QWidget(parent,f),
  explore(explore_),
  southIsUp(viewpoint==SECOND_SIDE),
  currentNode(currentNode_),
  globals(globals_),
  controllableSides(controllableSides_),
  autoRotate(false),
  drag{NO_SQUARE,NO_SQUARE},
  neutralColor(0x89,0x65,0x7B)
{
  qMediaPlayer.setPlaylist(&qMediaPlaylist);
  toggleSound(soundOn);
  if (!customSetup())
    initSetup();

  globals.settings.beginGroup("Board");
  const auto stepMode=globals.settings.value("step_mode").toBool();
  const auto iconSet=static_cast<PieceIcons::Set>(globals.settings.value("icon_set",PieceIcons::VECTOR).toInt());
  const auto animate=globals.settings.value("animate",true).toBool();
  const auto animationDelay=globals.settings.value("animation_delay",375).toInt();
  globals.settings.endGroup();

  setStepMode(stepMode);
  setIconSet(iconSet);
  setAnimate(animate);
  setAnimationDelay(animationDelay);

  connect(&animationTimer,&QTimer::timeout,this,&Board::animateNextStep);
}

bool Board::customSetup() const
{
  return currentNode.get()==nullptr;
}

bool Board::setupPhase() const
{
  return customSetup() || currentNode->inSetup();
}

bool Board::setupPlacementPhase() const
{
  return !customSetup() && currentNode->inSetup() && currentSetupPiece>=0;
}

Side Board::sideToMove() const
{
  return customSetup() ? potentialSetup.sideToMove : currentNode->currentState.sideToMove;
}

const GameState& Board::gameState() const
{
  return setupPhase() ? potentialSetup :
                        (potentialMove.get().empty() ? currentNode->currentState : std::get<RESULTING_STATE>(potentialMove.get().back()));
}

const GameState& Board::displayedGameState() const
{
  if (isAnimating()) {
    const auto& previousNode=currentNode->previousNode;
    assert(previousNode!=nullptr);
    const auto& previousMove=currentNode->move;
    if (nextAnimatedStep==previousMove.begin())
      return previousNode->currentState;
    else
      return std::get<RESULTING_STATE>(*--decltype(nextAnimatedStep)(nextAnimatedStep));
  }
  else
    return gameState();
}

Placements Board::currentPlacements() const
{
  return gameState().placements(sideToMove());
}

std::string Board::tentativeMoveString() const
{
  if (setupPhase())
    return toString(currentPlacements());
  else
    return toString(potentialMove.get().current());
}

bool Board::gameEnd() const
{
  return !customSetup() && currentNode->result.endCondition!=NO_END;
}

bool Board::playable() const
{
  return !gameEnd() && (explore || controllableSides[sideToMove()]);
}

bool Board::setNode(NodePtr newNode,const bool sound,bool keepState)
{
  keepState&=(newNode==currentNode.get());
  const bool transition=(newNode->previousNode==currentNode.get() && potentialMove.data.empty());
  currentNode=std::move(newNode);
  if (sound && currentNode->previousNode!=nullptr) {
    if (currentNode->previousNode->inSetup())
      playSound("qrc:/finished-setup.wav");
    else if (animate)
      animateMove(!transition);
    else
      playStepSounds(currentNode->move,true);
  }
  if (!keepState) {
    if (setupPhase()) {
      potentialSetup=currentNode->currentState;
      initSetup();
    }
    else
      potentialMove.data.clear();
  }
  if (autoRotate)
    setViewpoint(sideToMove());
  emit boardChanged();
  refreshHighlights(true);
  update();
  return !keepState;
}

void Board::proposeMove(const Node& child,const unsigned int playedOutSteps)
{
  if (currentNode->inSetup())
    proposeSetup(child.currentState);
  else {
    const auto& move=child.move;
    doSteps(move,false,move.size()-playedOutSteps);
  }
}

void Board::proposeSetup(GameState gameState)
{
  potentialSetup=std::move(gameState);
  currentSetupPiece=-1;
  emit boardChanged();
  refreshHighlights(true);
  update();
}

void Board::doSteps(const ExtendedSteps& steps,const bool sound,const int undoneSteps)
{
  if (!steps.empty()) {
    potentialMove.data.append(steps);
    potentialMove.data.shiftEnd(-undoneSteps);
    if (sound && !explore)
      playStepSounds(steps,false);
    if (!autoFinalize(true)) {
      emit boardChanged();
      refreshHighlights(true);
    }
  }
}

void Board::undoSteps(const bool all)
{
  bool updated=potentialMove.data.shiftEnd(false,all);
  if (explore && !updated && currentNode->previousNode!=nullptr) {
    const auto oldNode=currentNode;
    setNode(currentNode->previousNode);
    if (currentNode->inSetup()) {
      if (!all)
        proposeSetup(oldNode->currentState);
    }
    else {
      const auto& move=oldNode->move;
      if (!all)
        doSteps(move,false,move.size()==MAX_STEPS_PER_MOVE ? 1 : 0);
    }
    updated=true;
  }
  if (updated) {
    emit boardChanged();
    refreshHighlights(true);
    update();
  }
}

void Board::redoSteps(const bool all)
{
  const bool updated=potentialMove.data.shiftEnd(true,all);
  if (!autoFinalize(updated) && updated) {
    emit boardChanged();
    refreshHighlights(true);
    update();
  }
}

void Board::rotate()
{
  southIsUp=!southIsUp;
  refreshHighlights(false);
  update();
  emit boardRotated(southIsUp);
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
  setSetting(stepMode,newStepMode,"step_mode");
  setMouseTracking(stepMode);
  if (refreshHighlights(true))
    update();
}

void Board::setIconSet(const PieceIcons::Set newIconSet)
{
  if (setSetting(iconSet,newIconSet,"icon_set"))
    update();
}

void Board::setAnimate(const bool newAnimate)
{
  setSetting(animate,newAnimate,"animate");
}

void Board::setAnimationDelay(const int newAnimationDelay)
{
  if (setSetting(animationDelay,newAnimationDelay,"animation_delay"))
    animationTimer.setInterval(animationDelay);
}

void Board::playSound(const QString& soundFile)
{
  qMediaPlaylist.clear();
  qMediaPlaylist.addMedia(QUrl(soundFile));
  qMediaPlayer.play();
}

void Board::setExploration(const bool on)
{
  explore=on;
  if (on)
    autoFinalize(false);
  update();
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
  if (isValidSquare(file,rank)) {
    normalizeOrientation(file,rank);
    return toSquare(file,rank);
  }
  else
    return NO_SQUARE;
}

SquareIndex Board::positionToSquare(const QPoint& position) const
{
  const int file=floorDiv(position.x(),squareWidth());
  const int rank=floorDiv(position.y(),squareHeight());
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
  const int file=floorDiv(position.x(),squareWidth());
  const int rank=floorDiv(position.y(),squareHeight());
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

bool Board::isSetupSquare(const Side side,const SquareIndex square) const
{
  return customSetup() ? square!=NO_SQUARE : ::isSetupSquare(side,square);
}

bool Board::validDrop() const
{
  return setupPhase() ? (isSetupSquare(sideToMove(),drag[DESTINATION]) && drag[ORIGIN]!=drag[DESTINATION]) : !dragSteps.empty();
}

bool Board::isAnimating() const
{
  return animationTimer.isActive();
}

template<class Type>
bool Board::setSetting(readonly<Board,Type>& currentValue,const Type newValue,const QString& key)
{
  if (currentValue==newValue)
    return false;
  else {
    currentValue=newValue;

    globals.settings.beginGroup("Board");
    globals.settings.setValue(key,currentValue.get());
    globals.settings.endGroup();

    return true;
  }
}

void Board::initSetup()
{
  std::copy_n(&numStartingPiecesPerType[1],NUM_PIECE_TYPES-1,numSetupPieces.begin());
  nextSetupPiece();
}

void Board::nextSetupPiece()
{
  currentSetupPiece=static_cast<int>(numSetupPieces.size()-1);
  while (true) {
    if (currentSetupPiece<0) {
      // Fill remaining squares with most numerous piece type.
      const PieceTypeAndSide piece=toPieceTypeAndSide(FIRST_PIECE_TYPE,sideToMove());
      for (SquareIndex square=FIRST_SQUARE;square<NUM_SQUARES;increment(square)) {
        PieceTypeAndSide& squarePiece=potentialSetup.currentPieces[square];
        if (squarePiece==NO_PIECE && isSetupSquare(sideToMove(),square))
          squarePiece=piece;
      }
      autoFinalize(true);
      emit boardChanged();
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
      emit gameStarted();
    PieceTypeAndSide& currentPiece=currentPieces[destination];
    if (currentPiece!=NO_PIECE)
      ++numSetupPieces[toPieceType(currentPiece)-1];
    currentPiece=toPieceTypeAndSide(static_cast<PieceType>(currentSetupPiece+1),sideToMove());
    emit boardChanged();
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
    if (!isSetupSquare(sideToMove(),highlighted[ORIGIN]) || potentialSetup.currentPieces[highlighted[ORIGIN]]==NO_PIECE)
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
  if (square==NO_SQUARE)
    return false;
  else if (setupPlacementPhase()) {
    if (setUpPiece(square)) {
      refreshHighlights(true);
      return true;
    }
  }
  else if (stepMode)
    return doubleSquareAction(highlighted[ORIGIN],highlighted[DESTINATION]);
  else {
    if (highlighted[ORIGIN]==NO_SQUARE) {
      if (setupPhase() ? (isSetupSquare(sideToMove(),square) && potentialSetup.currentPieces[square]!=NO_PIECE) : gameState().legalOrigin(square)) {
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
      doSteps(route,true);
      return true;
    }
  }
  return false;
}

bool Board::doubleSquareSetupAction(const SquareIndex origin,const SquareIndex destination)
{
  if (isSetupSquare(sideToMove(),destination)) {
    GameState::Board& currentPieces=potentialSetup.currentPieces;
    if (customSetup()) {
      currentPieces[destination]=currentPieces[origin];
      currentPieces[origin]=NO_PIECE;
    }
    else
      std::swap(currentPieces[origin],currentPieces[destination]);
    emit boardChanged();
    refreshHighlights(true);
    return true;
  }
  else
    return false;
}

void Board::finalizeSetup(const Placements& placements)
{
  if (sideToMove()==FIRST_SIDE) {
    potentialSetup.sideToMove=SECOND_SIDE;
    initSetup();
  }
  else
    potentialMove.data.clear();

  const auto newNode=Node::addSetup(currentNode,placements,explore);
  emit sendNodeChange(newNode,currentNode);
  currentNode=std::move(newNode);

  emit boardChanged();
  if (autoRotate)
    setViewpoint(sideToMove());
  refreshHighlights(true);
  update();
}

void Board::finalizeMove(const ExtendedSteps& move)
{
  const auto newNode=Node::makeMove(currentNode,move,explore);
  emit sendNodeChange(newNode,currentNode);
  currentNode=std::move(newNode);

  potentialMove.data.clear();
  emit boardChanged();
  if (autoRotate && currentNode->result.endCondition==NO_END)
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

bool Board::autoFinalize(const bool stepsTaken)
{
  if (!customSetup() && explore) {
    if (currentNode->inSetup()) {
      if (currentSetupPiece<0)
        finalizeSetup(currentPlacements());
      else if (const auto& child=currentNode->findPartialMatchingChild(currentPlacements()).first) {
        proposeSetup(child->currentState);
        return true;
      }
      else
        return false;
    }
    else {
      const auto& currentSteps=potentialMove.get().current();
      if (gameState().stepsAvailable==0 || (!stepsTaken && currentNode->findMatchingChild(currentSteps).first!=nullptr))
        finalizeMove(currentSteps);
      else {
        if (const auto& child=currentNode->findPartialMatchingChild(potentialMove.get().all()).first)
          potentialMove.data.set(child->move,currentSteps.size());
        return false;
      }
    }
    if (const auto& child=currentNode->child(0))
      proposeMove(*child.get(),0);
    return true;
  }
  else
    return false;
}

void Board::mousePressEvent(QMouseEvent* event)
{
  disableAnimation();
  const SquareIndex square=positionToSquare(event->pos());
  switch (event->button()) {
    case Qt::LeftButton:
      if (square!=NO_SQUARE && playable()) {
        const PieceTypeAndSide currentPiece=gameState().currentPieces[square];
        if (customSetup() ? currentPiece!=NO_PIECE : (currentNode->inSetup() ? isSide(currentPiece,sideToMove()) : gameState().legalOrigin(square))) {
          fill(drag,square);
          dragSteps.clear();
          update();
        }
      }
    break;
    case Qt::RightButton:
      if (playable()) {
        if (drag[ORIGIN]!=NO_SQUARE)
          endDrag();
        else if (!stepMode && highlighted[ORIGIN]!=NO_SQUARE) {
          highlighted[ORIGIN]=NO_SQUARE;
          update();
        }
        else if (customSetup()) {
          if (square!=NO_SQUARE)
            new Popup(*this,square);
        }
        else
          undoSteps(false);
      }
    break;
    case Qt::MidButton:
      animateMove(true);
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
    disableAnimation();
    if (drag[ORIGIN]!=NO_SQUARE) {
      const SquareIndex square=positionToSquare(event->pos());
      if (drag[DESTINATION]!=square) {
        drag[DESTINATION]=square;
        if (square==NO_SQUARE || drag[ORIGIN]==drag[DESTINATION])
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
  switch (event->button()) {
    case Qt::LeftButton:
      disableAnimation();
      if (playable()) {
        const SquareIndex eventSquare=positionToSquare(event->pos());
        if (drag[ORIGIN]!=NO_SQUARE) {
          assert(eventSquare==drag[DESTINATION]);
          if (drag[ORIGIN]==eventSquare)
            singleSquareAction(eventSquare);
          else if (setupPhase())
            doubleSquareSetupAction(drag[ORIGIN],eventSquare);
          else
            doSteps(dragSteps,true);
          endDrag();
        }
        else if (singleSquareAction(eventSquare))
          update();
      }
    break;
    case Qt::MidButton:
    break;
    default:
      disableAnimation();
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
      disableAnimation();
      const SquareIndex eventSquare=positionToSquare(event->pos());
      if ((eventSquare==NO_SQUARE || gameState().currentPieces[eventSquare]==NO_PIECE) && (!stepMode || found(highlighted,NO_SQUARE))) {
        if (customSetup()) {
          if (potentialSetup.legalPosition()) {
            const auto node=std::make_shared<Node>(nullptr,ExtendedSteps(),potentialSetup);
            if (!node->inSetup() && node->result.endCondition==NO_END) {
              emit sendNodeChange(node,currentNode);
              currentNode=std::move(node);
              if (autoRotate)
                setViewpoint(sideToMove());
              emit boardChanged();
              refreshHighlights(true);
              update();
            }
            else
              MessageBox(QMessageBox::Critical,tr("Terminal position"),tr("Game already finished in this position."),QMessageBox::NoButton,this).exec();
          }
          else
            MessageBox(QMessageBox::Critical,tr("Illegal position"),tr("Unprotected piece on trap."),QMessageBox::NoButton,this).exec();
        }
        else if (currentNode->inSetup()) {
          if (currentSetupPiece<0) {
            if (!explore)
              playSound("qrc:/finished-setup.wav");
            finalizeSetup(currentPlacements());
          }
        }
        else {
          const auto& playedMove=potentialMove.get().current();
          switch (currentNode->legalMove(gameState())) {
            case LEGAL:
              if (!explore)
                playSound("qrc:/loud-step.wav");
              finalizeMove(playedMove);
            break;
            case ILLEGAL_PUSH_INCOMPLETION:
              playSound("qrc:/illegal-move.wav");
              MessageBox(QMessageBox::Critical,tr("Incomplete push"),tr("Push was not completed."),QMessageBox::NoButton,this).exec();
            break;
            case ILLEGAL_PASS:
              playSound("qrc:/illegal-move.wav");
              if (!playedMove.empty())
                MessageBox(QMessageBox::Critical,tr("Illegal pass"),tr("Move did not change board."),QMessageBox::NoButton,this).exec();
            break;
            case ILLEGAL_REPETITION:
              playSound("qrc:/illegal-move.wav");
              MessageBox(QMessageBox::Critical,tr("Illegal repetition"),tr("Move would repeat board and side to move too often."),QMessageBox::NoButton,this).exec();
            break;
          }
        }
      }
    }
    break;
    case Qt::RightButton:
      if (!customSetup()) {
        disableAnimation();
        undoSteps(true);
      }
    break;
    default:
      event->ignore();
      return;
    break;
  }
  event->accept();
}

void Board::wheelEvent(QWheelEvent* event)
{
  if (!customSetup()) {
    disableAnimation();
    if (event->angleDelta().y()<0)
      undoSteps(false);
    else
      redoSteps(false);
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
  const QColor mildSideColors[NUM_SIDES]={{0xC9,0xC9,0xC9},{0x23,0x23,0x23}};
  QPainter qPainter(this);

  qreal factor=squareHeight()/static_cast<qreal>(qPainter.fontMetrics().height());
  for (SquareIndex square=FIRST_SQUARE;square<NUM_SQUARES;increment(square))
    if (isTrap(square))
      factor=std::min(factor,squareWidth()/static_cast<qreal>(qPainter.fontMetrics().horizontalAdvance(toCoordinates(square,'A').data())));
  if (factor>0) {
    QFont qFont(qPainter.font());
    qFont.setPointSizeF(qFont.pointSizeF()*factor);
    qPainter.setFont(qFont);
  }

  const GameState& gameState_=displayedGameState();
  const auto previousPieces=(customSetup() || currentNode->move.empty() ? nullptr : &currentNode->previousNode->currentState.currentPieces);

  QPen qPen(Qt::SolidPattern,1);
  const QPoint mousePosition=mapFromGlobal(QCursor::pos());
  for (unsigned int pass=0;pass<2;++pass)
    for (SquareIndex square=FIRST_SQUARE;square<NUM_SQUARES;increment(square)) {
      const unsigned int file=toFile(square);
      const unsigned int rank=toRank(square);
      const bool isTrapSquare=isTrap(file,rank);
      if (!isAnimating() && (drag[ORIGIN]==NO_SQUARE ? found(highlighted,square) || (gameState_.inPush && square==gameState_.followupDestination)
                                                     : square==drag[DESTINATION] && validDrop())) {
        if (pass==0)
          continue;
        else {
          qPainter.setBrush(sideColors[sideToMove()]);
          qPen.setColor(sideColors[otherSide(sideToMove())]);
        }
      }
      else if (pass==1)
        continue;
      else {
        if (!isAnimating() && !setupPhase() && (square==drag[ORIGIN] || found<ORIGIN>(dragSteps,square)))
          qPainter.setBrush(mildSideColors[sideToMove()]);
        else if (setupPhase() ? !customSetup() && playable() && isSetupRank(sideToMove(),rank)
                              : (isAnimating() || gameState_.stepsAvailable==MAX_STEPS_PER_MOVE) &&
                                previousPieces!=nullptr && (*previousPieces)[square]!=gameState_.currentPieces[square])
          qPainter.setBrush(sideColors[otherSide(sideToMove())]);
        else {
          if (isTrapSquare)
            qPainter.setBrush(rank<NUM_RANKS/2 ? QColor(0xF6,0x66,0x10) : QColor(0x42,0x00,0xF2));
          else if (customSetup())
            qPainter.setBrush(mildSideColors[sideToMove()]);
          else if (explore && found(controllableSides,false))
            qPainter.setBrush(QColor(0xC9,0x2D,0x0C));
          else
            qPainter.setBrush(neutralColor);
        }
        qPen.setColor(sideColors[sideToMove()]);
      }
      qPainter.setPen(qPen);
      const QRect qRect=visualRectangle(file,rank);
      qPainter.drawRect(qRect);
      if (isTrapSquare) {
        if (gameEnd())
          qPainter.setPen(neutralColor);
        qPainter.drawText(qRect,Qt::AlignCenter,toCoordinates(file,rank,'A').data());
      }
      if (square!=drag[ORIGIN] || isAnimating()) {
        const PieceTypeAndSide pieceOnSquare=gameState_.currentPieces[square];
        if (pieceOnSquare!=NO_PIECE)
          globals.pieceIcons.drawPiece(qPainter,iconSet,pieceOnSquare,qRect);
      }
    }
  if (playable() && setupPlacementPhase())
    globals.pieceIcons.drawPiece(qPainter,iconSet,toPieceTypeAndSide(static_cast<PieceType>(currentSetupPiece+1),sideToMove()),QRect((NUM_FILES-1)/2.0*squareWidth(),(NUM_RANKS-1)/2.0*squareHeight(),squareWidth(),squareHeight()));
  if (drag[ORIGIN]!=NO_SQUARE && !isAnimating())
    globals.pieceIcons.drawPiece(qPainter,iconSet,gameState_.currentPieces[drag[ORIGIN]],QRect(mousePosition.x()-squareWidth()/2,mousePosition.y()-squareHeight()/2,squareWidth(),squareHeight()));
  qPainter.end();
}

void Board::animateMove(const bool showStart)
{
  if (customSetup() || currentNode->isGameStart())
    return;
  else if (currentNode->move.empty())
    playSound("qrc:/finished-setup.wav");
  else {
    qMediaPlaylist.clear();
    const auto& previousMove=currentNode->move;
    nextAnimatedStep=previousMove.begin();
    assert(nextAnimatedStep!=previousMove.end());
    if (showStart) {
      animationTimer.start();
      update();
    }
    else
      animateNextStep();
  }
}

void Board::animateNextStep()
{
  if (qMediaPlayer.state()==QMediaPlayer::StoppedState)
    qMediaPlaylist.clear();
  const auto lastStep=currentNode->move.end();
  if (!playCaptureSound(*nextAnimatedStep++))
    qMediaPlaylist.addMedia(QUrl(nextAnimatedStep==lastStep ? "qrc:/loud-step.wav" : "qrc:/soft-step.wav"));
  qMediaPlayer.play();
  if (nextAnimatedStep==lastStep)
    animationTimer.stop();
  else
    animationTimer.start();
  update();
}

void Board::disableAnimation()
{
  if (isAnimating()) {
    animationTimer.stop();
    update();
  }
}

void Board::playStepSounds(const ExtendedSteps& steps,const bool emphasize)
{
  qMediaPlaylist.clear();
  for (const auto& step:steps)
    playCaptureSound(step);
  if (qMediaPlaylist.isEmpty())
    qMediaPlaylist.addMedia(QUrl(emphasize ? "qrc:/loud-step.wav" : "qrc:/soft-step.wav"));
  qMediaPlayer.play();
}

bool Board::playCaptureSound(const ExtendedStep& step)
{
  const auto trappedPiece=std::get<TRAPPED_PIECE>(step);
  if (trappedPiece==NO_PIECE)
    return false;
  else {
    if (toSide(trappedPiece)==std::get<RESULTING_STATE>(step).sideToMove)
      qMediaPlaylist.addMedia(QUrl("qrc:/dropped-piece.wav"));
    else
      qMediaPlaylist.addMedia(QUrl("qrc:/captured-piece.wav"));
    return true;
  }
}
