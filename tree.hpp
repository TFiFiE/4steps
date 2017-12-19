#ifndef TREE_HPP
#define TREE_HPP

#include <memory>
#include "movetree.hpp"

class GameTreeNode {
public:
  explicit GameTreeNode(std::shared_ptr<GameTree> gameTree_=std::make_shared<GameTree>()) :
    gameTree(gameTree_),
    firstSetupNode(gameTree->end()),
    moveNode(nullptr)
  {
  }
  explicit GameTreeNode(std::shared_ptr<GameTree> gameTree_,const GameTree::iterator firstSetupNode_,MoveTree* const moveNode_=nullptr) :
    gameTree(gameTree_),
    firstSetupNode(firstSetupNode_),
    moveNode(moveNode_)
  {
  }
  void addSetup(const Placement& placement)
  {
    if (firstSetup()) {
      gameTree->emplace_back(placement,MoveTrees());
      firstSetupNode=--gameTree->end();
      moveNode=nullptr;
    }
    else if (secondSetup()) {
      MoveTrees& moveTrees=firstSetupNode->second;
      GameState gameState;
      gameState.add(firstSetupNode->first);
      gameState.add(placement);
      moveTrees.emplace_back(gameState);
      moveNode=&moveTrees.back();
    }
    else
      assert(false);
  }
  void makeMove(const ExtendedSteps& move)
  {
    assert(moveNode!=nullptr);
    moveNode=&moveNode->makeMove(move);
  }
  void makeMove(const PieceSteps& move)
  {
    assert(moveNode!=nullptr);
    makeMove(moveNode->currentState.toExtendedSteps(move));
  }
  void undo()
  {
    assert(!firstSetup());
    if (secondSetup())
      firstSetupNode=gameTree->end();
    else if (moveNode->previousNode==nullptr)
      moveNode=nullptr;
    else
      moveNode=moveNode->previousNode;
  }
  std::vector<GameTreeNode> children() const
  {
    std::vector<GameTreeNode> result;
    if (firstSetup()) {
      result.reserve(gameTree->size());
      for (auto firstMove=gameTree->begin();firstMove!=gameTree->end();++firstMove)
        result.emplace_back(gameTree,firstMove,nullptr);
    }
    else if (secondSetup()) {
      result.reserve(firstSetupNode->second.size());
      for (auto& secondMove:firstSetupNode->second)
        result.emplace_back(gameTree,firstSetupNode,&secondMove);
    }
    else {
      result.reserve(moveNode->branches.size());
      for (auto& move:moveNode->branches)
        result.emplace_back(gameTree,firstSetupNode,&move.second);
    }
    return result;
  }
  bool firstSetup() const
  {
    return firstSetupNode==gameTree->end();
  }
  bool secondSetup() const
  {
    return moveNode==nullptr;
  }
  bool inSetup() const
  {
    return moveNode==nullptr;
  }
  std::vector<GameState> history() const
  {
    if (firstSetup())
      return std::vector<GameState>(1);
    else if (secondSetup()) {
      GameState result;
      result.add(firstSetupNode->first);
      result.sideToMove=SECOND_SIDE;
      return std::vector<GameState>(1,result);
    }
    else
      return moveNode->history();
  }
  Side sideToMove() const
  {
    if (firstSetup())
      return FIRST_SIDE;
    else if (secondSetup())
      return SECOND_SIDE;
    else
      return moveNode->currentState.sideToMove;
  }
  Result result() const
  {
    if (moveNode==nullptr)
      return {NO_SIDE,NO_END};
    else
      return moveNode->result;
  }
  MoveTree& getMoveNode() const
  {
    assert(moveNode!=nullptr);
    return *moveNode;
  }
private:
  std::shared_ptr<GameTree> gameTree;
  GameTree::iterator firstSetupNode;
  MoveTree* moveNode;
};

#endif // TREE_HPP
