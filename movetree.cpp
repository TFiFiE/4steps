#include "movetree.hpp"

MoveTree::MoveTree(const GameState& currentState_,MoveTree* const previousHistory) :
  previousNode(previousHistory),
  currentState(currentState_),
  result{NO_SIDE,NO_END}
{
}

MoveTree::MoveTree(const MoveTree& moveTree) :
  previousNode(moveTree.previousNode),
  currentState(moveTree.currentState),
  result(moveTree.result),
  branches(moveTree.branches)
{
  for (auto& branch:branches)
    branch.second.previousNode=this;
}

MoveTree& MoveTree::operator=(const MoveTree& rhs)
{
  if (this!=&rhs) {
    previousNode=rhs.previousNode;
    currentState=rhs.currentState;
    result=rhs.result;
    branches=rhs.branches;
    for (auto& branch:branches)
      branch.second.previousNode=this;
  }
  return *this;
}

bool MoveTree::operator==(const MoveTree& rhs) const
{
  return (previousNode==nullptr)==(rhs.previousNode==nullptr) &&
         currentState==rhs.currentState &&
         branches==rhs.branches;
}

unsigned int MoveTree::numDescendants() const
{
  unsigned int result=branches.size();
  for (const auto& branch:branches)
    result+=branch.second.numDescendants();
  return result;
}

std::vector<GameState> MoveTree::history() const
{
  if (previousNode==nullptr)
    return std::vector<GameState>(1,currentState);
  else {
    auto result=previousNode->history();
    result.push_back(currentState);
    return result;
  }
}

bool MoveTree::changedSquare(const SquareIndex square) const
{
  if (previousNode==nullptr)
    return false;
  return currentState.currentPieces[square]!=previousNode->currentState.currentPieces[square];
}

MoveLegality MoveTree::legalMove(const GameState& resultingState) const
{
  if (resultingState.inPush)
    return ILLEGAL_PUSH_INCOMPLETION;
  assert(resultingState.sideToMove==currentState.sideToMove);
  if (resultingState.currentPieces==currentState.currentPieces)
    return ILLEGAL_PASS;
  unsigned int repetitionCount=0;
  const MoveTree* currentNode=previousNode;
  while (currentNode!=nullptr) {
    const GameState& earlierState=currentNode->currentState;
    assert(resultingState.sideToMove!=earlierState.sideToMove);
    if (resultingState.currentPieces==earlierState.currentPieces) {
      if (repetitionCount==MAX_ALLOWED_REPETITIONS)
        return ILLEGAL_REPETITION;
      else
        ++repetitionCount;
    }
    currentNode=currentNode->previousNode;
    if (currentNode==nullptr)
      break;
    currentNode=currentNode->previousNode;
  }
  return LEGAL;
}

bool MoveTree::hasLegalMoves(const GameState& startingState) const
{
  if (legalMove(startingState)==LEGAL)
    return true;
  for (SquareIndex origin=FIRST_SQUARE;origin<NUM_SQUARES;increment(origin))
    for (const SquareIndex adjacentSquare:adjacentSquares(origin))
      if (startingState.legalStep(origin,adjacentSquare)) {
        GameState changedState(startingState);
        changedState.takeStep(origin,adjacentSquare);
        if (hasLegalMoves(changedState))
          return true;
      }
  return false;
}

Result MoveTree::detectGameEnd()
{
  assert(currentState.stepsAvailable==MAX_STEPS_PER_MOVE);
  bool goal[NUM_SIDES]={false,false};
  bool eliminated[NUM_SIDES]={true,true};
  for (SquareIndex square=FIRST_SQUARE;square<NUM_SQUARES;increment(square)) {
    const PieceTypeAndSide pieceTypeAndSide=currentState.currentPieces[square];
    if (pieceTypeAndSide!=NO_PIECE) {
      const PieceType pieceType=toPieceType(pieceTypeAndSide);
      if (pieceType==WINNING_PIECE_TYPE) {
        const Side pieceSide=toSide(pieceTypeAndSide);
        if (isGoal(square,pieceSide))
          goal[pieceSide]=true;
        eliminated[pieceSide]=false;
      }
    }
  }
  const Side playedSide=otherSide(currentState.sideToMove);
  const auto prioritySides={playedSide,currentState.sideToMove};
  for (const Side side:prioritySides) {
    if (goal[side]) {
      result.endCondition=GOAL;
      result.winner=side;
      return result;
    }
    else if (eliminated[otherSide(side)]) {
      result.endCondition=ELIMINATION;
      result.winner=side;
      return result;
    }
  }
  if (!hasLegalMoves(currentState)) {
    result.endCondition=IMMOBILIZATION;
    result.winner=playedSide;
  }
  return result;
}

MoveTree& MoveTree::makeMove(const ExtendedSteps& move)
{
  branches.emplace_back(move,MoveTree(std::get<RESULTING_STATE>(move.back()),this));
  MoveTree& newHistory=branches.back().second;
  newHistory.currentState.switchTurn();
  newHistory.detectGameEnd();
  return newHistory;
}
