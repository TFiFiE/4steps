#ifndef TURNSTATE_HPP
#define TURNSTATE_HPP

#include <array>
#include "def.hpp"

class TurnState {
public:
  typedef std::array<PieceTypeAndSide,NUM_SQUARES> Board;
  typedef std::array<unsigned int,NUM_PIECE_SIDE_COMBINATIONS> PieceCounts;
  typedef std::array<PieceType,NUM_PIECE_TYPES> TypeToType;

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
  ExtendedSteps toExtendedSteps(const Steps& steps) const;
  ExtendedSteps toExtendedSteps(const PieceSteps& pieceSteps) const;
  static std::vector<SquareIndex> differentSquares(const Board& lhs,const Board& rhs);
  static Board flipSides(const Board& board);
  static Board mirror(const Board& board);

  void add(const Placements& placements);
  void remapPieces(const TypeToType& remapping);
  virtual void switchTurn();
  virtual void flipSides();
  virtual void mirror();

  static TypeToType typeToRanks(const PieceCounts& pieceCounts);
  static std::vector<TypeToType> typeRemappings(const PieceCounts& pieceCounts,const TypeToType& typeToRanks,const PieceType source,TypeToType& currentRemapping);
  static std::vector<TypeToType> typeRemappings(const PieceCounts& pieceCounts);

  Side sideToMove;
  Board squarePieces;
};

#endif // TURNSTATE_HPP
