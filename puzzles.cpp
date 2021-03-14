#include <fstream>
#include <QPainter>
#include "puzzles.hpp"
#include "globals.hpp"
#include "messagebox.hpp"
#include "io.hpp"

const std::vector<SquareIndex> Puzzles::trapSquares=getTrapSquares();

using namespace std;
Puzzles::Puzzles(Globals& globals_,const std::string& fileName,QWidget* const parent) :
  Game(globals_,FIRST_SIDE,parent,nullptr,make_unique<TurnState>(),1),
  globals(globals_),
  pieceType(LAST_PIECE_TYPE),
  keepScore(false),
  reshuffle(tr("&Reshuffle")),
  autoAdvance(tr("Auto-adva&nce")),
  autoExplore(tr("Auto-e&xplore")),
  autoUndo(tr("Auto-&undo")),
  sounds(tr("&Sounds")),
  pieceIcon(*this),
  resetPuzzle(tr("Reset &puzzle")),
  hint(tr("&Hint")),
  answer(tr("&Answer"))
{
  animate.setEnabled(true);
  confirm.setEnabled(true);
  current.setEnabled(true);

  setWindowTitle(tr("Puzzles (%1)").arg(QString::fromStdString(fileName)));

  connect(&reshuffle,&QPushButton::clicked,this,[this] {
    shuffle(puzzles.begin(),puzzles.end(),globals.rndEngine);
    try {
      setPuzzle(0);
    }
    catch (...) {
      close();
    }
  });
  reshuffle.setToolTip(tr("Reshuffle puzzles and start from beginning"));
  layout.addWidget(&reshuffle);

  connect(&spinBox,static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),this,[this](const int value) {
    const unsigned int newPuzzleIndex=value-DISPLAYED_INDEX_OFFSET;
    if (newPuzzleIndex!=puzzleIndex)
      try {
        setPuzzle(newPuzzleIndex);
      }
      catch (...) {
        spinBox.setValue(puzzleIndex+DISPLAYED_INDEX_OFFSET);
      }
  });
  layout.addWidget(&spinBox);

  globals.settings.beginGroup("Puzzles");
  for (const auto& tuple:{make_tuple(&autoAdvance,"auto-advance",false,tr("Go to next puzzle after right answer")),
                          make_tuple(&autoExplore,"auto-explore",false,tr("Switch to exploration mode after wrong answer")),
                          make_tuple(&autoUndo,"auto-undo",false,tr("Undo move after wrong answer")),
                          make_tuple(&sounds,"sounds",true,tr("Play sound effect after attempt"))}) {
    const auto checkBox=get<0>(tuple);
    const QString key=get<1>(tuple);
    const auto defaultValue=get<2>(tuple);
    const auto& toolTip=get<3>(tuple);
    checkBox->setChecked(globals.settings.value(key,defaultValue).toBool());
    checkBox->setToolTip(toolTip);
    connect(checkBox,&QCheckBox::toggled,this,[this,key](const bool checked) {
      globals.settings.beginGroup("Puzzles");
      globals.settings.setValue(key,checked);
      globals.settings.endGroup();
    });
    layout.addWidget(checkBox);
  }
  globals.settings.endGroup();

  connect(&iconSets,&QActionGroup::triggered,&pieceIcon,static_cast<void (PieceIcon::*)()>(&PieceIcon::update));
  layout.addWidget(&pieceIcon);

  connect(&board,&Board::boardChanged,this,[this] {
    if (!keepScore) {
      evaluation.clear();
      if (solveTimer.isValid())
        solveTime.clear();
    }
  });
  for (const auto& pair:{make_pair(&evaluation,2.0),make_pair(&solveTime,1.5)}) {
    const auto widget=pair.first;
    QFont font(widget->font());
    font.setPointSizeF(font.pointSizeF()*pair.second);
    widget->setFont(font);
    widget->setTextInteractionFlags(Qt::TextSelectableByMouse);
    layout.addWidget(widget,0,Qt::AlignHCenter);
  }

  layout.addStretch();

  connect(&resetPuzzle,&QPushButton::clicked,this,[this] {
    evaluation.clear();
    solveTime.clear();
    gameTree=GameTree(1,liveNode);
    board.setNode(liveNode);
    explore.setChecked(false);
    clearHints();
    board.update();
    solveTimer.restart();
  });
  layout.addWidget(&resetPuzzle);

  connect(&hint,&QPushButton::clicked,this,[this] {
    const auto otherSide_=otherSide(board.sideToMove());
    if (pastRevealed==differentSquares.end()) {
      for (const auto differentSquare:differentSquares)
        board.customColors[differentSquare]=Board::SquareColorIndex(Board::HIGHLIGHT+otherSide_);
      hint.setEnabled(false);
    }
    else {
      const auto numUnrevealed=distance(pastRevealed,differentSquares.end());
      iter_swap(pastRevealed,std::next(pastRevealed,globals.rand(numUnrevealed)));
      board.customColors[*pastRevealed]=Board::SquareColorIndex(Board::HIGHLIGHT_MILD+otherSide_);
      ++pastRevealed;
    }
    board.update();
  });
  hint.setToolTip(tr("Highlight (another) square changed by solution or indicate there are no more"));
  layout.addWidget(&hint);

  connect(&answer,&QPushButton::clicked,this,[this] {
    explore.setChecked(true);
    const auto node=Node::makeMove(liveNode,solution,true);
    Node::addToTree(gameTree,node);
    board.setNode(node,false);
    solveTimer.invalidate();
  });
  layout.addWidget(&answer);

  puzzleDock.setLayout(&layout);

  auto& dockWidget=dockWidgets[NUM_STANDARD_DOCK_WIDGETS];
  dockWidget.setObjectName("Puzzle");
  dockWidget.setAllowedAreas(Qt::LeftDockWidgetArea|Qt::RightDockWidgetArea);
  dockWidget.setFeatures(QDockWidget::DockWidgetMovable|QDockWidget::DockWidgetFloatable);
  QMainWindow::addDockWidget(Qt::LeftDockWidgetArea,&dockWidget,Qt::Horizontal);
  dockWidget.setWidget(&puzzleDock);

  try {
    loadPuzzles(fileName);
  }
  catch (const std::exception& exception) {
    MessageBox(QMessageBox::Critical,tr("Error reading puzzle file"),exception.what(),QMessageBox::NoButton,this).exec();
    throw;
  }
  shuffle(puzzles.begin(),puzzles.end(),globals.rndEngine);
  try {
    setPuzzle(0);
  }
  catch (...) {
    throw;
  }
  setPuzzleCount();
  show();
}

void Puzzles::setPuzzleCount()
{
  spinBox.setRange(DISPLAYED_INDEX_OFFSET,puzzles.size()-1+DISPLAYED_INDEX_OFFSET);
  dockWidgets[NUM_STANDARD_DOCK_WIDGETS].setWindowTitle(tr("%1 puzzle(s)").arg(QString::number(puzzles.size())));
}

void Puzzles::loadPuzzles(const std::string& fileName)
{
  std::ifstream is(fileName,std::ios::binary);
  is.seekg(0,std::ios::end);
  runtime_assert(is.tellg()%PUZZLE_BYTE_SIZE==0,"File size is not a multiple of "+QString::number(PUZZLE_BYTE_SIZE)+" bytes.");
  is.seekg(0);
  PuzzleData puzzle;
  while (true) {
    is.read(reinterpret_cast<char*>(puzzle.data()),puzzle.size());
    const auto gcount=is.gcount();
    if (gcount==PUZZLE_BYTE_SIZE)
      puzzles.emplace_back(puzzle);
    else {
      assert(gcount==0);
      break;
    }
  }
  runtime_assert(!puzzles.empty(),"File is empty.");
}

Puzzles::Puzzle Puzzles::toPuzzle(const PuzzleData& puzzleData)
{
  TurnState::Board board;
  Steps move;

  auto bytePtr=puzzleData.cbegin();
  unsigned int pieceCounts[NUM_PIECE_SIDE_COMBINATIONS]={};
  for (auto square=board.begin();square!=board.end();++bytePtr) {
    unsigned char byte=*bytePtr;
    for (unsigned int byteSquareIndex=0;byteSquareIndex<SQUARES_PER_BYTES && square!=board.end();++byteSquareIndex,++square) {
      *square=toSquareValue(byte%SQUARE_DATA_RANGE);
      if (*square!=NO_PIECE) {
        const auto pieceType=toPieceType(*square);
        auto& pieceCount=pieceCounts[*square];
        ++pieceCount;
        runtime_assert(pieceCount<=numStartingPiecesPerType[pieceType],tr("Too many of: %1").arg(pieceName(*square)));
        if (pieceType==WINNING_PIECE_TYPE) {
          const auto squareIndex=SquareIndex(distance(board.begin(),square));
          runtime_assert(!isGoal(squareIndex,toSide(*square)),tr("%1 on goal square ").arg(pieceName(*square))+toCoordinates(squareIndex).data()+'.');
        }
      }
      byte/=SQUARE_DATA_RANGE;
    }
    assert(byte==0);
  }
  for (Side side=FIRST_SIDE;side<NUM_SIDES;increment(side)) {
    const auto piece=toPieceTypeAndSide(WINNING_PIECE_TYPE,side);
    runtime_assert(pieceCounts[piece]>0,tr("No %1.").arg(pieceName(piece)));
  }
  for (const auto& trapSquare:trapSquares) {
    const auto piece=board[trapSquare];
    if (piece!=NO_PIECE) {
      const auto side=toSide(piece);
      bool supported=false;
      forEachAdjacentSquare(trapSquare,[&board,&side,&supported](const SquareIndex adjacentSquare) {
        if (isSide(board[adjacentSquare],side)) {
          supported=true;
          return true;
        }
        else
          return false;
      });
      runtime_assert(supported,tr("Unsupported %1 on trap square ").arg(pieceName(piece))+toCoordinates(trapSquare).data()+'.');
    }
  }
  assert(toStep(PASS,false).second==NO_SQUARE);
  bool passed=false;
  for (unsigned int stepIndex=0;stepIndex<MAX_STEPS_PER_MOVE;++stepIndex,++bytePtr) {
    if (*bytePtr==PASS) {
      runtime_assert(stepIndex>0,"Empty solution move.");
      passed=true;
    }
    else {
      runtime_assert(!passed,"Extra steps after pass in solution.");
      move.emplace_back(toStep(*bytePtr));
    }
  }

  return {TurnState(FIRST_SIDE,board),move};
}

Puzzles::PuzzleData Puzzles::toData(Puzzle puzzle)
{
  const auto& state=puzzle.first;
  const auto& board=state.squarePieces;
  const auto& move=puzzle.second;
  if (state.sideToMove==SECOND_SIDE)
    flipSides(puzzle);
  assert(state.sideToMove==FIRST_SIDE);

  PuzzleData result={};

  auto bytePtr=result.begin();
  for (auto square=board.begin();square!=board.end();++bytePtr) {
    unsigned int product=1;
    for (unsigned int byteSquareIndex=0;byteSquareIndex<SQUARES_PER_BYTES && square!=board.end();++byteSquareIndex,++square) {
      *bytePtr+=toData(*square)*product;
      product*=SQUARE_DATA_RANGE;
    }
  }
  for (unsigned int stepIndex=0;stepIndex<MAX_STEPS_PER_MOVE;++stepIndex,++bytePtr)
    *bytePtr=(stepIndex<move.size() ? toData(move[stepIndex]) : PASS);

  return result;
}

PieceTypeAndSide Puzzles::toSquareValue(const unsigned char bits)
{
  static_assert(NUM_PIECE_SIDE_COMBINATIONS<SQUARE_DATA_RANGE,"");
  if (bits==EMPTY_VALUE)
    return NO_PIECE;
  else {
    runtime_assert(bits<NUM_PIECE_SIDE_COMBINATIONS,"Square value out of range: "+QString::number(bits));
    return PieceTypeAndSide(bits);
  }
}

unsigned char Puzzles::toData(const PieceTypeAndSide squareValue)
{
  return squareValue==NO_PIECE ? EMPTY_VALUE : static_cast<unsigned char>(squareValue);
}

Step Puzzles::toStep(const unsigned char byte,const bool strict)
{
  const auto origin=SquareIndex(byte%NUM_SQUARES);
  const auto direction=byte/NUM_SQUARES;
  assert(direction<NUM_DIRECTIONS);
  const SquareIndex destination=toDestination(origin,Direction(direction),strict);
  return {origin,destination};
}

unsigned char Puzzles::toData(const Step& step)
{
  return step.first+NUM_SQUARES*toDirection(step.first,step.second);
}

void Puzzles::flipSides(Puzzle& puzzle)
{
  puzzle.first.flipSides();
  for (auto& step:puzzle.second)
    step={invert(step.first),invert(step.second)};
}

void Puzzles::mirror(Puzzle& puzzle)
{
  puzzle.first.mirror();
  for (auto& step:puzzle.second)
    step={::mirror(step.first),::mirror(step.second)};
}

template<unsigned int numValues,class Number>
std::array<Number,numValues> Puzzles::clock(const std::array<Number,numValues-1>& circles,Number number)
{
  std::array<Number,numValues> result;
  auto value=result.begin();
  for (const auto& circle:circles) {
    *value=std::fmod(number,circle);
    number=std::floor(number/circle);
    ++value;
  }
  *value=number;
  return result;
}

QString Puzzles::displayedTime(const qint64 milliseconds)
{
  QString timeString;
  constexpr auto numberOfUnits=3;
  const char units[numberOfUnits]={'s','m','h'};
  const auto values=clock<numberOfUnits>({60,60},std::round(milliseconds/100.0)/10);
  for (int unit=numberOfUnits-1;unit>=0;--unit)
    if (values[unit]!=0) {
      if (!timeString.isEmpty())
        timeString+=' ';
      timeString+=QString::number(values[unit])+units[unit];
    }
  return timeString;
}

void Puzzles::randomize(Puzzle& puzzle) const
{
  if (globals.rand(2)!=0)
    flipSides(puzzle);
  if (globals.rand(2)!=0)
    mirror(puzzle);
  auto& state=puzzle.first;
  const auto remappings=TurnState::typeRemappings(state.pieceCounts());
  state.remapPieces(remappings[globals.rand(remappings.size())]);
}

void Puzzles::setPuzzle(unsigned int newIndex)
{
  while (true) {
    try {
      assert(newIndex<puzzles.size());
      setPuzzle(toPuzzle(puzzles[newIndex]));
      break;
    }
    catch (const std::exception& exception) {
      puzzles.erase(puzzles.begin()+newIndex);
      setPuzzleCount();
      if (newIndex>=puzzles.size()) {
        MessageBox(QMessageBox::Critical,tr("Error parsing puzzle data"),QString(exception.what())+"\n\n"+tr("No more data."),QMessageBox::NoButton,this).exec();
        throw;
      }
      else {
        MessageBox messageBox(QMessageBox::Question,tr("Error parsing puzzle data"),QString(exception.what())+"\n\n"+tr("Try next chunk?"),QMessageBox::Yes|QMessageBox::No,this);
        if (messageBox.exec()==QMessageBox::No)
          throw;
      }
    }
  }
  puzzleIndex=newIndex;
  spinBox.setValue(puzzleIndex+DISPLAYED_INDEX_OFFSET);
}

void Puzzles::setPuzzle(Puzzle puzzle)
{
  randomize(puzzle);
  const auto& state=puzzle.first;
  const auto node=std::make_shared<Node>(nullptr,ExtendedSteps(),GameState(state));
  const auto solution=state.toExtendedSteps(puzzle.second);
  runtime_assert(node->legalMove(solution)==MoveLegality::LEGAL,"Illegal solution move.");
  return setPuzzle(node,solution);
}

void Puzzles::setPuzzle(const NodePtr node,const ExtendedSteps& newSolution)
{
  const auto& state=node->gameState;

  gameTree=GameTree(1,node);
  treeModel.root=Node::root(node);
  liveNode=node;
  board.setNode(node);
  explore.setChecked(false);
  board.setControllable({state.sideToMove==FIRST_SIDE,state.sideToMove==SECOND_SIDE});

  solution=newSolution;
  differentSquares=TurnState::differentSquares(state.squarePieces,resultingState(solution).squarePieces);
  assert(!differentSquares.empty());
  clearHints();
  board.setViewpoint(state.sideToMove);

  pieceType=PieceType((pieceType+1)%NUM_PIECE_TYPES);
  pieceIcon.setToolTip(tr("Solve for %1").arg(sideWord(state.sideToMove)));
  pieceIcon.update();

  if (!keepScore)
    solveTime.clear();
  solveTimer.restart();
}

void Puzzles::clearHints()
{
  pastRevealed=differentSquares.begin();
  fill(board.customColors,Board::REGULAR);
  hint.setEnabled(true);
}

void Puzzles::receiveNodeChange(const NodePtr& newNode)
{
  if (board.explore)
    Game::receiveNodeChange(newNode);
  else {
    Node::addToTree(gameTree,newNode);
    emit treeModel.layoutChanged();
    if (newNode->gameState.squarePieces==resultingState(solution).squarePieces) {
      const auto elapsed=solveTimer.elapsed();
      if (solveTimer.isValid()) {
        if (sounds.isChecked())
          board.playSound("qrc:/right.wav",true);
        solveTime.setText(displayedTime(elapsed));
      }
      const bool solved=(solveTimer.isValid() || !solveTime.text().isEmpty());
      evaluation.setStyleSheet(solved ? "color:#00ff00;" : "");
      evaluation.setText("RIGHT");
      keepScore=true;
      bool puzzleChanged=false;
      if (autoAdvance.isChecked() && puzzleIndex+1<puzzles.size()) {
        try {
          setPuzzle(puzzleIndex+1);
          puzzleChanged=true;
        }
        catch (...) {}
      }
      if (!puzzleChanged) {
        solveTimer.invalidate();
        explore.setChecked(true);
      }
      keepScore=false;
    }
    else {
      if (sounds.isChecked())
        board.playSound("qrc:/wrong.wav",true);
      const auto currentNode=board.currentNode;
      if (autoExplore.isChecked())
        explore.setChecked(true);
      if (autoUndo.isChecked() || !autoExplore.isChecked()) {
        board.setNode(newNode->previousNode);
        board.proposeMove(*currentNode.get(),autoUndo.isChecked() ? 0 : currentNode->move.size());
      }
      evaluation.setStyleSheet("color:#ff00ff;");
      evaluation.setText("WRONG");
    }
  }
}

Puzzles::PieceIcon::PieceIcon(Puzzles& puzzles_) :
  puzzles(puzzles_)
{
  QSizePolicy sizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
  sizePolicy.setHeightForWidth(true);
  setSizePolicy(sizePolicy);
}

int Puzzles::PieceIcon::heightForWidth(int w) const
{
  return w;
}

void Puzzles::PieceIcon::paintEvent(QPaintEvent*)
{
  QPainter qPainter(this);
  const auto value=std::min(width(),height());
  QRect qRect((width()-value)/2,(height()-value)/2,value,value);
  const auto piece=toPieceTypeAndSide(puzzles.pieceType,puzzles.liveNode->gameState.sideToMove);
  puzzles.globals.pieceIcons.drawPiece(qPainter,puzzles.board.iconSet,piece,qRect);
  qPainter.end();
}
