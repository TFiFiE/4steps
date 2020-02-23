#ifndef NODE_HPP
#define NODE_HPP

#include <memory>
#include <mutex>
#include "gamestate.hpp"

struct Node {
  const NodePtr previousNode;
  ExtendedSteps move;
  const int depth;
  const GameState currentState;
  const Result result;
  mutable std::list<std::weak_ptr<Node> > children;
  mutable std::mutex children_mutex;

  explicit Node(NodePtr previousNode_,const ExtendedSteps& move_,const GameState& currentState_);
  bool isGameStart() const;
  bool inSetup() const;
  std::string toString() const;
  std::vector<std::weak_ptr<Node> > ancestors() const;
  int numMovesBefore(const Node* descendant) const;
  NodePtr findClosestChild(const NodePtr& descendant) const;
  MoveLegality legalMove(const GameState& resultingState) const;
  bool hasLegalMoves(const GameState& startingState) const;
  Result detectGameEnd() const;
  int childIndex() const;
  NodePtr child(const int index) const;
  bool hasChild() const;
  int numChildren() const;
  int maxChildSteps() const;
  std::pair<NodePtr,int> findPartialMatchingChild(const Placements& placements) const;
  std::pair<NodePtr,int> findPartialMatchingChild(const ExtendedSteps& steps) const;
  std::pair<NodePtr,int> findMatchingChild(const Placements& subset) const;
  std::pair<NodePtr,int> findMatchingChild(const ExtendedSteps& move) const;
private:
  template<class Predicate> std::pair<NodePtr,int> findChild(Predicate predicate) const;
  template<class Predicate> std::pair<NodePtr,int> findChild_(Predicate predicate) const;
  static NodePtr addChild(const NodePtr& node,const ExtendedSteps& move,const GameState& newState,const bool after);
public:
  static NodePtr addSetup(const NodePtr& node,const Placements& placements,const bool after);
  static NodePtr makeMove(const NodePtr& node,const ExtendedSteps& move,const bool after);
  static NodePtr makeMove(const NodePtr& node,const PieceSteps& move,const bool after);
  static NodePtr root(const NodePtr& node);

  static GameTree createTree()
  {
    return GameTree(1,std::make_shared<Node>(nullptr,ExtendedSteps(),GameState()));
  }

  template<class Container>
  static void addToTree(Container& gameTree,const NodePtr& node)
  {
    for (auto& existing:gameTree)
      if (node->numMovesBefore(existing.get())>=0)
        return;
      else if (existing->numMovesBefore(node.get())>=0) {
        existing=node;
        return;
      }
    gameTree.emplace(gameTree.end(),node);
  }
};

#endif // NODE_HPP
