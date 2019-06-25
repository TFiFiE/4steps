#include "movetree.hpp"

MoveTree::MoveTree(const GameState& currentState_) :
  previousNode(nullptr),
  currentState(currentState_),
  result{NO_SIDE,NO_END}
{
}

MoveTree::MoveTree(const GameState& currentState_,MoveTree* const previousNode_) :
  previousNode(previousNode_),
  currentState(currentState_)
{
  detectGameEnd();
}

MoveTree::MoveTree(const MoveTree& moveTree) :
  previousNode(moveTree.previousNode),
  currentState(moveTree.currentState),
  result(moveTree.result),
  branches(moveTree.branches)
{
  for (auto branch=branches.begin();branch!=branches.end();++branch) {
    auto& child=branch->second;
    child.previousNode=this;
    child.previousEdge=branch;
  }
}

size_t MoveTree::numDescendants() const
{
  size_t result=branches.size();
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
  if (hasLegalMoves(currentState)) {
    result.endCondition=NO_END;
    result.winner=NO_SIDE;
  }
  else {
    result.endCondition=IMMOBILIZATION;
    result.winner=playedSide;
  }
  return result;
}

MoveTree& MoveTree::makeMove(const ExtendedSteps& move,const bool overwriteSteps)
{
  GameState gameState(std::get<RESULTING_STATE>(move.back()));
  gameState.switchTurn();
  const auto found=find_if(branches.begin(),branches.end(),[&gameState](const decltype(branches)::value_type& branch) {
    return gameState==branch.second.currentState;
  });
  if (found==branches.end()) {
    branches.emplace_back(move,MoveTree(gameState,this));
    MoveTree& newHistory=branches.back().second;
    newHistory.previousEdge=--branches.end();
    return newHistory;
  }
  else {
    if (overwriteSteps)
      found->first=move;
    return found->second;
  }
}
