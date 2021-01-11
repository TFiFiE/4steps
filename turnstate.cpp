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
  for (SquareIndex square=FIRST_SQUARE;square<NUM_SQUARES;increment(square))
    if (squarePieces[square]!=NO_PIECE)
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

Placements TurnState::playedPlacements() const
{
  return placements(otherSide(sideToMove));
}

std::array<bool,NUM_PIECE_SIDE_COMBINATIONS> TurnState::piecesAtMax() const
{
  std::array<bool,NUM_PIECE_SIDE_COMBINATIONS> result={false};
  unsigned int pieceCounts[NUM_SIDES][NUM_PIECE_TYPES]={{0}};
  for (SquareIndex square=FIRST_SQUARE;square<NUM_SQUARES;increment(square)) {
    const PieceTypeAndSide pieceOnSquare=squarePieces[square];
    if (pieceOnSquare!=NO_PIECE) {
      const PieceType pieceType=toPieceType(pieceOnSquare);
      auto& pieceCount=pieceCounts[toSide(pieceOnSquare)][pieceType];
      if (++pieceCount==numStartingPiecesPerType[pieceType])
        result[pieceOnSquare]=true;
    }
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
    if (pieceTypeAndSide!=NO_PIECE) {
      const Side pieceSide=toSide(pieceTypeAndSide);
      for (const auto adjacentSquare:adjacentSquares(square))
        if (isSide(squarePieces[adjacentSquare],pieceSide))
          return false;
      return true;
    }
  }
  return false;
}

bool TurnState::legalPosition() const
{
  for (SquareIndex square=FIRST_SQUARE;square<NUM_SQUARES;increment(square))
    if (floatingPiece(square))
      return false;
  return true;
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
