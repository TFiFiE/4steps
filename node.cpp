#include "node.hpp"
#include "io.hpp"

Node::Node(NodePtr previousNode_,const ExtendedSteps& move_,const GameState& gameState_) :
  previousNode(std::move(previousNode_)),
  move(move_),
  depth(previousNode==nullptr ? 0 : previousNode->depth+1),
  gameState(gameState_),
  result(detectGameEnd())
{
}

const Node& Node::root() const
{
  return previousNode==nullptr ? *this : *root(previousNode).get();
}

bool Node::isGameStart() const
{
  return previousNode==nullptr && move.empty() && gameState.sideToMove==FIRST_SIDE && gameState.empty();
}

bool Node::inSetup() const
{
  return previousNode==nullptr ? (move.empty() && gameState.sideToMove==FIRST_SIDE && gameState.empty())
                               : (gameState.sideToMove==SECOND_SIDE && move.empty() && previousNode->isGameStart());
}

Placements Node::playedPlacements() const
{
  return gameState.placements(otherSide(gameState.sideToMove));
}

std::string Node::toPlyString() const
{
  return toPlyString(root());
}

std::string Node::toPlyString(const Node& root) const
{
  return depth==0 ? "" : ::toPlyString(depth-1,root);
}

std::string Node::nextPlyString() const
{
  return ::toPlyString(depth,root());
}

std::string Node::toString() const
{
  if (move.empty())
    return ::toString(playedPlacements());
  else
    return ::toString(move);
}

std::vector<std::weak_ptr<Node> > Node::ancestors(const Node* const final) const
{
  if (this==final || previousNode==nullptr)
    return {};
  else
    return selfAndAncestors(previousNode,final);
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

bool Node::isAncestorOfOrSameAs(const Node* descendant) const
{
  return numMovesBefore(descendant)>=0;
}

NodePtr Node::findClosestChild(const NodePtr& descendant) const
{
  for (auto nodePtr=&descendant;;) {
    const auto& node=*nodePtr;
    if (node==nullptr)
      return nullptr;
    const auto& previousNode=node->previousNode;
    if (this==previousNode.get())
      return node;
    else
      nodePtr=&previousNode;
  }
}

MoveLegality Node::legalMove(const GameState& resultingState) const
{
  assert(!inSetup());
  if (resultingState.inPush)
    return MoveLegality::ILLEGAL_PUSH_INCOMPLETION;
  assert(resultingState.sideToMove==gameState.sideToMove);
  if (resultingState.squarePieces==gameState.squarePieces)
    return MoveLegality::ILLEGAL_PASS;
  unsigned int repetitionCount=0;
  for (auto currentNode=previousNode;currentNode!=nullptr;currentNode=currentNode->previousNode) {
    const GameState& earlierState=currentNode->gameState;
    if (resultingState.sideToMove!=earlierState.sideToMove &&
        resultingState.squarePieces==earlierState.squarePieces) {
      if (repetitionCount==MAX_ALLOWED_REPETITIONS)
        return MoveLegality::ILLEGAL_REPETITION;
      else
        ++repetitionCount;
    }
    if (currentNode->move.empty())
      break;
  }
  return MoveLegality::LEGAL;
}

MoveLegality Node::legalMove(const ExtendedSteps& move) const
{
  return legalMove(move.empty() ? gameState : resultingState(move));
}

bool Node::legalPartialMove(const ExtendedSteps& move) const
{
  return move.empty() || hasLegalMoves(resultingState(move));
}

bool Node::hasLegalMoves(const GameState& startingState) const
{
  if (legalMove(startingState)==MoveLegality::LEGAL)
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
  assert(gameState.stepsAvailable==MAX_STEPS_PER_MOVE);
  bool goal[NUM_SIDES]={false,false};
  bool eliminated[NUM_SIDES]={true,true};
  for (SquareIndex square=FIRST_SQUARE;square<NUM_SQUARES;increment(square)) {
    const PieceTypeAndSide pieceTypeAndSide=gameState.squarePieces[square];
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
  const Side playedSide=otherSide(gameState.sideToMove);
  const auto prioritySides={playedSide,gameState.sideToMove};
  for (const Side side:prioritySides) {
    if (goal[side])
      return {side,GOAL};
    else if (eliminated[otherSide(side)])
      return {side,ELIMINATION};
  }
  if (hasLegalMoves(gameState))
    return {NO_SIDE,NO_END};
  else
    return {playedSide,IMMOBILIZATION};
}

int Node::childIndex() const
{
  if (previousNode==nullptr)
    return 0;
  else
    return previousNode->findChild([this](const NodePtr& child,const int){return this==child.get();}).second;
}

int Node::cumulativeChildIndex() const
{
  return previousNode==nullptr ? 0 : childIndex()+previousNode->cumulativeChildIndex();
}

NodePtr Node::child(const int index) const
{
  return findChild([index](const NodePtr&,const int rhs){return index==rhs;}).first;
}

bool Node::hasChild() const
{
  return findChild([](const NodePtr&,const int){return true;}).first!=nullptr;
}

int Node::numChildren() const
{
  return findChild([](const NodePtr&,const int){return false;}).second;
}

size_t Node::maxChildSteps() const
{
  size_t result=0;
  findChild([&result](const NodePtr& child,const int) {
    result=std::max(result,child->move.size());
    return result==MAX_STEPS_PER_MOVE;
  });
  return result;
}

size_t Node::maxDescendantSteps() const
{
  size_t result=0;
  findChild([&result](const NodePtr& child,const int) {
    result=std::max(result,child->move.size());
    if (result<MAX_STEPS_PER_MOVE)
      result=std::max(result,child->maxDescendantSteps());
    return result==MAX_STEPS_PER_MOVE;
  });
  return result;
}

std::pair<NodePtr,int> Node::findPartialMatchingChild(const Placements& subset) const
{
  return findChild([&](const NodePtr& child,const int) {
    const auto& set=child->playedPlacements();
    return includes(set.begin(),set.end(),subset.begin(),subset.end());
  });
}

std::pair<NodePtr,int> Node::findPartialMatchingChild(const ExtendedSteps& steps) const
{
  return findChild([&](const NodePtr& child,const int) {
    return startsWith(child->move,steps);
  });
}

std::pair<NodePtr,int> Node::findMatchingChild(const Placements& subset) const
{
  return findChild([&](const NodePtr& child,const int) {
    return subset==child->playedPlacements();
  });
}

std::pair<NodePtr,int> Node::findMatchingChild(const ExtendedSteps& move) const
{
  return findChild([&](const NodePtr& child,const int) {
    return move==child->move;
  });
}

template<class Predicate>
std::pair<NodePtr,int> Node::findChild(Predicate predicate) const
{
  const std::lock_guard<std::mutex> lock(children_mutex);
  return findChild_(predicate);
}

template<class Predicate>
std::pair<NodePtr,int> Node::findChild_(Predicate predicate) const
{
  int index=0;
  for (auto child=children.begin();child!=children.end();) {
    if (const auto& lockedChild=child->lock()) {
      if (predicate(lockedChild,index))
        return {std::move(lockedChild),index};
      ++child;
      ++index;
    }
    else
      child=children.erase(child);
  }
  return {nullptr,index};
}

NodePtr Node::addChild(const NodePtr& node,const ExtendedSteps& move,const GameState& gameState,const bool after)
{
  const std::lock_guard<std::mutex> lock(node->children_mutex);
  const auto oldChild=node->findChild_([&gameState](const NodePtr& child,const int){return gameState==child->gameState;}).first;
  if (oldChild==nullptr) {
    auto& children=node->children;
    const auto newChild=std::make_shared<Node>(node,move,gameState);
    if (after)
      children.emplace_back(newChild);
    else
      children.emplace(children.begin(),newChild);
    return newChild;
  }
  else {
    oldChild->move=move;
    return oldChild;
  }
}

NodePtr Node::addSetup(const NodePtr& node,const Placements& placements,const bool after)
{
  assert(node->inSetup());
  GameState gameState=node->gameState;
  gameState.add(placements);
  gameState.switchTurn();
  return addChild(node,ExtendedSteps(),gameState,after);
}

NodePtr Node::makeMove(const NodePtr& node,const ExtendedSteps& move,const bool after)
{
  assert(!node->inSetup());
  GameState gameState(resultingState(move));
  gameState.switchTurn();
  return addChild(node,move,gameState,after);
}

NodePtr Node::makeMove(const NodePtr& node,const PieceSteps& move,const bool after)
{
  return makeMove(node,node->gameState.toExtendedSteps(move),after);
}

void Node::swapChildren(const Node& firstChild,const int siblingOffset) const
{
  for (auto child=children.begin();child!=children.end();) {
    if (const auto& lockedChild=child->lock()) {
      if (lockedChild.get()==&firstChild) {
        iter_swap(child,next(child,siblingOffset));
        return;
      }
      ++child;
    }
    else
      child=children.erase(child);
  }
}

NodePtr Node::root(const NodePtr& node)
{
  assert(node!=nullptr);
  for (auto nodePtr=&node;;) {
    const auto& node=*nodePtr;
    const auto& previousNode=node->previousNode;
    if (previousNode==nullptr)
      return node;
    else
      nodePtr=&previousNode;
  }
}

NodePtr Node::reroot(NodePtr source,NodePtr target)
{
  assert(source!=nullptr);
  assert(target==nullptr || target->previousNode==nullptr);
  std::vector<std::pair<ExtendedSteps,GameState> > line;
  do {
    line.emplace_back(source->move,source->gameState);
    source=source->previousNode;
  } while (source!=nullptr);
  auto ancestor=line.rbegin();
  if (target==nullptr)
    target=std::make_shared<Node>(nullptr,ancestor->first,ancestor->second);
  else if (ancestor->second!=target->gameState)
    return nullptr;
  for (++ancestor;ancestor!=line.rend();++ancestor)
    target=Node::addChild(target,ancestor->first,ancestor->second,true);
  return target;
}

std::vector<std::weak_ptr<Node> > Node::selfAndAncestors(const NodePtr& node,const Node* const final)
{
  assert(node!=nullptr);
  std::vector<std::weak_ptr<Node> > result;
  for (auto nodePtr=&node;;) {
    const auto& node=*nodePtr;
    result.emplace_back(node);
    if (node.get()==final)
      break;
    const auto& previousNode=node->previousNode;
    if (previousNode==nullptr)
      break;
    else
      nodePtr=&previousNode;
  }
  return result;
}
