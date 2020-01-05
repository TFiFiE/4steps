#ifndef NODE_HPP
#define NODE_HPP

#include <memory>
#include <mutex>
#include "gamestate.hpp"

struct Node {
  const NodePtr previousNode;
  ExtendedSteps move;
  const GameState currentState;
  const Result result;
  std::list<std::weak_ptr<Node> > children;
  mutable std::mutex children_mutex;

  explicit Node(NodePtr previousNode_,const ExtendedSteps& move_,const GameState& currentState_);
  bool isGameStart() const;
  bool inSetup() const;
  int numMovesBefore(const Node* descendant) const;
  MoveLegality legalMove(const GameState& resultingState) const;
  bool hasLegalMoves(const GameState& startingState) const;
  Result detectGameEnd() const;
private:
  NodePtr findChild(const GameState& gameState);
  static NodePtr addChild(const NodePtr& node,const ExtendedSteps& move,const GameState& newState);
public:
  static NodePtr addSetup(const NodePtr& node,const Placement& placement);
  static NodePtr makeMove(const NodePtr& node,const ExtendedSteps& move);
  static NodePtr makeMove(const NodePtr& node,const PieceSteps& move);
  static NodePtr root(NodePtr node);

  static GameTree createTree()
  {
    return GameTree(1,std::make_shared<Node>(nullptr,ExtendedSteps(),GameState()));
  }
};

#endif // NODE_HPP
