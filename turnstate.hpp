#ifndef TURNSTATE_HPP
#define TURNSTATE_HPP

#include <array>
#include "def.hpp"

class TurnState {
public:
  typedef std::array<PieceTypeAndSide,NUM_SQUARES> Board;
  typedef std::array<unsigned int,NUM_PIECE_SIDE_COMBINATIONS> PieceCounts;

  explicit TurnState(const Side sideToMove_=FIRST_SIDE,Board squarePieces_=emptyBoard());
  static Board emptyBoard();

  bool empty() const;
  Placements placements(const Side side) const;
  PieceCounts pieceCounts() const;
  std::array<bool,NUM_PIECE_SIDE_COMBINATIONS> piecesAtMax() const;
  bool isSupported(const SquareIndex square,const Side side) const;
  bool isFrozen(const SquareIndex square) const;
  bool floatingPiece(const SquareIndex square) const;
  bool hasFloatingPieces() const;
  ExtendedSteps toExtendedSteps(const std::vector<Step>& steps) const;
  ExtendedSteps toExtendedSteps(const PieceSteps& pieceSteps) const;

  void add(const Placements& placements);
  virtual void switchTurn();

  Side sideToMove;
  Board squarePieces;
};

#endif // TURNSTATE_HPP