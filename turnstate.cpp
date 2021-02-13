#include "turnstate.hpp"
#include "gamestate.hpp"

TurnState::TurnState(const Side sideToMove_,Board squarePieces_) :
  sideToMove(sideToMove_),
  squarePieces(std::move(squarePieces_))
{
}

TurnState::Board TurnState::emptyBoard()
{
  Board squarePieces;
  fill(squarePieces,NO_PIECE);
  return squarePieces;
}

bool TurnState::empty() const
{
  for (const auto squarePiece:squarePieces)
    if (squarePiece!=NO_PIECE)
      return false;
  return true;
}

Placements TurnState::placements(const Side side) const
{
  Placements result;
  for (SquareIndex square=FIRST_SQUARE;square<NUM_SQUARES;increment(square)) {
    const PieceTypeAndSide pieceType=squarePieces[square];
    if (isSide(pieceType,side))
      result.emplace(Placement{square,pieceType});
  }
  return result;
}

TurnState::PieceCounts TurnState::pieceCounts() const
{
  PieceCounts result={};
  for (const auto pieceOnSquare:squarePieces)
    if (pieceOnSquare!=NO_PIECE)
      ++result[pieceOnSquare];
  return result;
}

std::array<bool,NUM_PIECE_SIDE_COMBINATIONS> TurnState::piecesAtMax() const
{
  std::array<bool,NUM_PIECE_SIDE_COMBINATIONS> result;
  const auto pieceCounts_=pieceCounts();
  for (PieceTypeAndSide piece=PieceTypeAndSide(0);piece<NUM_PIECE_SIDE_COMBINATIONS;increment(piece)) {
    const auto diff=int(numStartingPiecesPerType[toPieceType(piece)])-int(pieceCounts_[piece]);
    assert(diff>=0);
    result[piece]=(diff==0);
  }
  return result;
}

bool TurnState::isSupported(const SquareIndex square,const Side side) const
{
  for (const auto adjacentSquare:adjacentSquares(square))
    if (isSide(squarePieces[adjacentSquare],side))
      return true;
  return false;
}

bool TurnState::isFrozen(const SquareIndex square) const
{
  const PieceTypeAndSide piece=squarePieces[square];
  const Side side=toSide(piece);
  bool dominatingNeighbor=false;
  for (const auto adjacentSquare:adjacentSquares(square)) {
    const PieceTypeAndSide neighbor=squarePieces[adjacentSquare];
    if (isSide(neighbor,side))
      return false;
    else
      dominatingNeighbor|=dominates(neighbor,piece);
  }
  return dominatingNeighbor;
}

bool TurnState::floatingPiece(const SquareIndex square) const
{
  if (isTrap(square)) {
    const PieceTypeAndSide pieceTypeAndSide=squarePieces[square];
    if (pieceTypeAndSide!=NO_PIECE)
      return !isSupported(square,toSide(pieceTypeAndSide));
  }
  return false;
}

bool TurnState::hasFloatingPieces() const
{
  for (SquareIndex square=FIRST_SQUARE;square<NUM_SQUARES;increment(square))
    if (floatingPiece(square))
      return true;
  return false;
}

ExtendedSteps TurnState::toExtendedSteps(const std::vector<Step>& steps) const
{
  return GameState(*this).takeSteps(steps);
}

ExtendedSteps TurnState::toExtendedSteps(const PieceSteps& pieceSteps) const
{
  return GameState(*this).takePieceSteps(pieceSteps);
}

void TurnState::add(const Placements& placements)
{
  for (const auto& pair:placements) {
    PieceTypeAndSide& pieceOnSquare=squarePieces[pair.location];
    runtime_assert(pieceOnSquare==NO_PIECE,"Square already has a piece.");
    pieceOnSquare=pair.piece;
  }
}

void TurnState::switchTurn()
{
  sideToMove=otherSide(sideToMove);
}
