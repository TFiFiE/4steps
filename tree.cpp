#include "tree.hpp"

GameTreeNode::GameTreeNode(std::shared_ptr<GameTree> gameTree_) :
  gameTree(gameTree_),
  firstSetupNode(gameTree->end()),
  moveNode(nullptr)
{
}

GameTreeNode::GameTreeNode(std::shared_ptr<GameTree> gameTree_,const GameTree::iterator firstSetupNode_,MoveTree* const moveNode_) :
  gameTree(gameTree_),
  firstSetupNode(firstSetupNode_),
  moveNode(moveNode_)
{
}

bool GameTreeNode::operator==(const GameTreeNode& rhs) const
{
  return gameTree==rhs.gameTree && firstSetupNode==rhs.firstSetupNode && moveNode==rhs.moveNode;
}

GameTreeNode GameTreeNode::create()
{
  return GameTreeNode(std::make_shared<GameTree>());
}

bool GameTreeNode::inSetup() const
{
  return moveNode==nullptr;
}

GameTreeNode::Phase GameTreeNode::currentPhase() const
{
  if (firstSetupNode==gameTree->end())
    return FIRST_SETUP;
  else if (moveNode==nullptr)
    return SECOND_SETUP;
  else
    return MOVING;
}

GameTree::iterator GameTreeNode::addFirstSetup(const Placement& placement) const
{
  const auto firstSetupNode=find_if(gameTree->begin(),gameTree->end(),[&](const GameTree::value_type& existing) {
    return placement==existing.first;
  });
  if (firstSetupNode==gameTree->end()) {
    gameTree->emplace_back(placement,MoveTrees());
    return --gameTree->end();
  }
  else
    return firstSetupNode;
}

MoveTree* GameTreeNode::addSecondSetup(const MoveTree& moveTree) const
{
  assert(firstSetupNode!=gameTree->end());
  MoveTrees& moveTrees=firstSetupNode->second;
  const auto found=find_if(moveTrees.begin(),moveTrees.end(),[&](const MoveTree& existing) {
    return moveTree.currentState==existing.currentState;
  });
  if (found==moveTrees.end()) {
    moveTrees.emplace_back(moveTree);
    return &moveTrees.back();
  }
  else
    return &*found;
}

void GameTreeNode::addSetup(const Placement& placement)
{
  switch (currentPhase()) {
    case FIRST_SETUP:
      firstSetupNode=addFirstSetup(placement);
      moveNode=nullptr;
    break;
    case SECOND_SETUP: {
      GameState gameState;
      gameState.add(firstSetupNode->first);
      gameState.add(placement);
      assert(gameState.sideToMove==FIRST_SIDE);
      moveNode=addSecondSetup(MoveTree(gameState));
    }
    break;
    case MOVING:
      assert(false);
    break;
  }
}

void GameTreeNode::customSetup(const MoveTree& moveTree)
{
  firstSetupNode=addFirstSetup(Placement());
  moveNode=addSecondSetup(moveTree);
}

void GameTreeNode::makeMove(const ExtendedSteps& move)
{
  assert(!inSetup());
  moveNode=&moveNode->makeMove(move);
}

void GameTreeNode::makeMove(const PieceSteps& move)
{
  assert(!inSetup());
  makeMove(moveNode->currentState.toExtendedSteps(move));
}

void GameTreeNode::undo()
{
  switch (currentPhase()) {
    case FIRST_SETUP:
      assert(false);
    break;
    case SECOND_SETUP:
      firstSetupNode=gameTree->end();
    break;
    case MOVING:
      if (moveNode->previousNode==nullptr)
        moveNode=nullptr;
      else
        moveNode=moveNode->previousNode;
    break;
  }
}

std::size_t GameTreeNode::numChildren() const
{
  switch (currentPhase()) {
    case  FIRST_SETUP: return gameTree->size();
    case SECOND_SETUP: return firstSetupNode->second.size();
    default: return moveNode->branches.size();
  }
}

template<class Function>
void GameTreeNode::forEachChild(Function function) const
{
  switch (currentPhase()) {
    case FIRST_SETUP:
      for (auto firstMove=gameTree->begin();firstMove!=gameTree->end();++firstMove)
        function(GameTreeNode(gameTree,firstMove));
    break;
    case SECOND_SETUP:
      for (auto& secondMove:firstSetupNode->second)
        function(GameTreeNode(gameTree,firstSetupNode,&secondMove));
    break;
    case MOVING:
      for (auto& move:moveNode->branches)
        function(GameTreeNode(gameTree,firstSetupNode,&move.second));
    break;
  }
}

std::vector<GameTreeNode> GameTreeNode::children() const
{
  std::vector<GameTreeNode> result;
  result.reserve(numChildren());
  forEachChild([&](const GameTreeNode& child) {
    result.emplace_back(child);
  });
  return result;
}

std::vector<GameState> GameTreeNode::history() const
{
  switch (currentPhase()) {
    case FIRST_SETUP:
    return std::vector<GameState>(1);
    break;
    case SECOND_SETUP: {
      GameState result;
      result.add(firstSetupNode->first);
      result.sideToMove=SECOND_SIDE;
      return std::vector<GameState>(1,result);
    }
    break;
    default:
    return moveNode->history();
    break;
  }
}

Side GameTreeNode::sideToMove() const
{
  switch (currentPhase()) {
    case  FIRST_SETUP: return  FIRST_SIDE;
    case SECOND_SETUP: return SECOND_SIDE;
    default: return moveNode->currentState.sideToMove;
  }
}

Result GameTreeNode::result() const
{
  if (inSetup())
    return {NO_SIDE,NO_END};
  else
    return moveNode->result;
}

MoveTree& GameTreeNode::getMoveNode() const
{
  assert(!inSetup());
  return *moveNode;
}

void GameTreeNode::addSubtree(const GameTreeNode& targetRoot,const GameTreeNode& sourceNode,GameTreeNode* const targetNode) const
{
  assert(currentPhase()==targetRoot.currentPhase());

  if (*this==sourceNode && targetNode!=nullptr)
    *targetNode=targetRoot;

  switch (currentPhase()) {
    case FIRST_SETUP:
      for (auto firstMove=gameTree->begin();firstMove!=gameTree->end();++firstMove) {
        GameTreeNode newSourceRoot(gameTree,firstMove);
        GameTreeNode newTargetRoot(targetRoot.gameTree,targetRoot.addFirstSetup(firstMove->first));
        newSourceRoot.addSubtree(newTargetRoot,sourceNode,targetNode);
      }
    break;
    case SECOND_SETUP:
      for (auto& secondMove:firstSetupNode->second) {
        GameTreeNode newSourceRoot(gameTree,firstSetupNode,&secondMove);
        GameTreeNode newTargetRoot(targetRoot.gameTree,targetRoot.firstSetupNode,targetRoot.addSecondSetup(secondMove));
        newSourceRoot.addSubtree(newTargetRoot,sourceNode,targetNode);
      }
    break;
    case MOVING:
      for (auto& move:moveNode->branches) {
        GameTreeNode newSourceRoot(gameTree,firstSetupNode,&move.second);
        GameTreeNode newTargetRoot(targetRoot.gameTree,targetRoot.firstSetupNode,&targetRoot.moveNode->makeMove(move.first,false));
        newSourceRoot.addSubtree(newTargetRoot,sourceNode,targetNode);
      }
    break;
  }
}

GameTreeNode GameTreeNode::mergeInto(const GameTreeNode& target) const
{
  if (gameTree==target.gameTree)
    return *this;
  else {
    GameTreeNode counterpart;
    GameTreeNode(gameTree).addSubtree(GameTreeNode(target.gameTree),*this,&counterpart);
    return counterpart;
  }
}
