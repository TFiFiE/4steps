#ifndef MOVETREE_HPP
#define MOVETREE_HPP

#include "gamestate.hpp"

struct MoveTree {
  typedef std::list<std::pair<ExtendedSteps,MoveTree> > Branches;
  MoveTree* previousNode;
  Branches::iterator previousEdge;
  GameState currentState;
  Result result;
  Branches branches;

  explicit MoveTree(const GameState& currentState_);
  explicit MoveTree(const GameState& currentState_,MoveTree* const previousNode_);
  explicit MoveTree(const MoveTree& moveTree);
  MoveTree& operator=(const MoveTree& rhs)=delete;
  bool operator==(const MoveTree& rhs) const=delete;
  size_t numDescendants() const;
  std::vector<GameState> history() const;
  bool changedSquare(const SquareIndex square) const;
  MoveLegality legalMove(const GameState& resultingState) const;
  bool hasLegalMoves(const GameState& startingState) const;
  Result detectGameEnd();
  MoveTree& makeMove(const ExtendedSteps& move,const bool overwriteSteps=true);
};

#endif // MOVETREE_HPP
