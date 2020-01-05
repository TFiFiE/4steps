#include "node.hpp"

Node::Node(NodePtr previousNode_,const ExtendedSteps& move_,const GameState& currentState_) :
  previousNode(std::move(previousNode_)),
  move(move_),
  currentState(currentState_),
  result(detectGameEnd())
{
}

bool Node::isGameStart() const
{
  return previousNode==nullptr && move.empty() && currentState.sideToMove==FIRST_SIDE && currentState.empty();
}

bool Node::inSetup() const
{
  return previousNode==nullptr ? currentState.empty() : (currentState.sideToMove==SECOND_SIDE && move.empty());
}

int Node::numMovesBefore(const Node* descendant) const
{
  for (int numGenerations=0;descendant!=nullptr;++numGenerations) {
    if (this==descendant)
      return numGenerations;
    descendant=descendant->previousNode.get();
  }
  return -1;
}

MoveLegality Node::legalMove(const GameState& resultingState) const
{
  assert(!inSetup());
  if (resultingState.inPush)
    return ILLEGAL_PUSH_INCOMPLETION;
  assert(resultingState.sideToMove==currentState.sideToMove);
  if (resultingState.currentPieces==currentState.currentPieces)
    return ILLEGAL_PASS;
  unsigned int repetitionCount=0;
  for (auto currentNode=previousNode;currentNode!=nullptr;currentNode=currentNode->previousNode) {
    const GameState& earlierState=currentNode->currentState;
    if (resultingState.sideToMove!=earlierState.sideToMove &&
        resultingState.currentPieces==earlierState.currentPieces) {
      if (repetitionCount==MAX_ALLOWED_REPETITIONS)
        return ILLEGAL_REPETITION;
      else
        ++repetitionCount;
    }
    if (currentNode->move.empty())
      break;
  }
  return LEGAL;
}

bool Node::hasLegalMoves(const GameState& startingState) const
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

Result Node::detectGameEnd() const
{
  if (inSetup())
    return {NO_SIDE,NO_END};
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
    if (goal[side])
      return {side,GOAL};
    else if (eliminated[otherSide(side)])
      return {side,ELIMINATION};
  }
  if (hasLegalMoves(currentState))
    return {NO_SIDE,NO_END};
  else
    return {playedSide,IMMOBILIZATION};
}

NodePtr Node::findChild(const GameState& gameState)
{
  for (auto child=children.begin();child!=children.end();) {
    if (const auto lockedChild=child->lock()) {
      if (gameState==lockedChild->currentState)
        return lockedChild;
    }
    if (child->expired())
      child=children.erase(child);
    else
      ++child;
  }
  return nullptr;
}

NodePtr Node::addChild(const NodePtr& node,const ExtendedSteps& move,const GameState& gameState)
{
  const std::lock_guard<std::mutex> lock(node->children_mutex);
  const auto oldChild=node->findChild(gameState);
  if (oldChild==nullptr) {
    const auto newChild=std::make_shared<Node>(node,move,gameState);
    node->children.emplace_back(newChild);
    return newChild;
  }
  else
    return oldChild;
}

NodePtr Node::addSetup(const NodePtr& node,const Placement& placement)
{
  assert(node->inSetup());
  GameState gameState=node->currentState;
  gameState.add(placement);
  gameState.switchTurn();
  return addChild(node,ExtendedSteps(),gameState);
}

NodePtr Node::makeMove(const NodePtr& node,const ExtendedSteps& move)
{
  assert(!node->inSetup());
  GameState gameState(std::get<RESULTING_STATE>(move.back()));
  gameState.switchTurn();
  return addChild(node,move,gameState);
}

NodePtr Node::makeMove(const NodePtr& node,const PieceSteps& move)
{
  return makeMove(node,node->currentState.toExtendedSteps(move));
}

NodePtr Node::root(NodePtr node)
{
  assert(node!=nullptr);
  while (node->previousNode!=nullptr)
    node=node->previousNode;
  return node;
}
