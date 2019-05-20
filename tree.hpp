#ifndef TREE_HPP
#define TREE_HPP

#include <memory>
#include "movetree.hpp"

class GameTreeNode {
public:
  enum Phase {
    FIRST_SETUP,
    SECOND_SETUP,
    MOVING
  };

  static GameTreeNode create();
  bool operator==(const GameTreeNode& rhs) const;
  bool inSetup() const;
  Phase currentPhase() const;
  void addSetup(const Placement& placement);
  void customSetup(const MoveTree& moveTree);
  void makeMove(const ExtendedSteps& move);
  void makeMove(const PieceSteps& move);
  void undo();
  std::size_t numChildren() const;
  std::vector<GameTreeNode> children() const;
  std::vector<GameState> history() const;
  Side sideToMove() const;
  Result result() const;
  MoveTree& getMoveNode() const;
  GameTreeNode mergeInto(const GameTreeNode& targetNode) const;
private:
  explicit GameTreeNode() {}
  explicit GameTreeNode(std::shared_ptr<GameTree> gameTree_);
  explicit GameTreeNode(std::shared_ptr<GameTree> gameTree_,const GameTree::iterator firstSetupNode_,MoveTree* const moveNode_=nullptr);
  GameTree::iterator addFirstSetup(const Placement& placement) const;
  MoveTree* addSecondSetup(const MoveTree& moveTree) const;
  template<class Function> void forEachChild(Function function) const;
  void addSubtree(const GameTreeNode& targetRoot,const GameTreeNode& sourceNode,GameTreeNode* const=nullptr) const;

  std::shared_ptr<GameTree> gameTree;
  GameTree::iterator firstSetupNode;
  MoveTree* moveNode;
};

#endif // TREE_HPP
