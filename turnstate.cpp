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

ExtendedSteps TurnState::toExtendedSteps(const Steps& steps) const
{
  return GameState(*this).takeSteps(steps);
}

ExtendedSteps TurnState::toExtendedSteps(const PieceSteps& pieceSteps) const
{
  return GameState(*this).takePieceSteps(pieceSteps);
}

std::vector<SquareIndex> TurnState::differentSquares(const Board& lhs,const Board& rhs)
{
  std::vector<SquareIndex> result;
  for (SquareIndex square=FIRST_SQUARE;square<NUM_SQUARES;increment(square))
    if (lhs[square]!=rhs[square])
      result.emplace_back(square);
  return result;
}

TurnState::Board TurnState::flipSides(const Board& board)
{
  Board result;
  for (SquareIndex square=FIRST_SQUARE;square<NUM_SQUARES;increment(square))
    result[square]=toOtherSide(board[invert(square)]);
  return result;
}

TurnState::Board TurnState::mirror(const Board& board)
{
  Board result;
  for (SquareIndex square=FIRST_SQUARE;square<NUM_SQUARES;increment(square))
    result[square]=board[::mirror(square)];
  return result;
}

void TurnState::add(const Placements& placements)
{
  for (const auto& pair:placements) {
    PieceTypeAndSide& pieceOnSquare=squarePieces[pair.location];
    runtime_assert(pieceOnSquare==NO_PIECE,"Square already has a piece.");
    pieceOnSquare=pair.piece;
  }
}

void TurnState::remapPieces(const TypeToType& remapping)
{
  for (auto& squarePiece:squarePieces)
    if (squarePiece!=NO_PIECE)
      squarePiece=toPieceTypeAndSide(remapping[toPieceType(squarePiece)],toSide(squarePiece));
}

void TurnState::switchTurn()
{
  sideToMove=otherSide(sideToMove);
}

void TurnState::flipSides()
{
  TurnState::switchTurn();
  squarePieces=flipSides(squarePieces);
}

void TurnState::mirror()
{
  squarePieces=mirror(squarePieces);
}

TurnState::TypeToType TurnState::typeToRanks(const PieceCounts& pieceCounts)
{
  TypeToType result;

  auto source=FIRST_PIECE_TYPE;
  auto target=FIRST_PIECE_TYPE;
  result[source]=target;

  std::array<bool,NUM_SIDES> hadStrongestType={true,true};
  for (increment(source);source<NUM_PIECE_TYPES;increment(source)) {
    std::array<bool,NUM_SIDES> hasType;
    bool anyPresent=false;
    bool allPresent=true;
    for (Side side=FIRST_SIDE;side<NUM_SIDES;increment(side)) {
      const bool present=(pieceCounts[toPieceTypeAndSide(source,side)]>0);
      hasType[side]=present;
      anyPresent|=present;
      allPresent&=present;
    }
    if (anyPresent) {
      if (allPresent || hasType!=hadStrongestType)
        increment(target);
      hadStrongestType=hasType;
    }
    result[source]=target;
  }

  return result;
}

std::vector<TurnState::TypeToType> TurnState::typeRemappings(const PieceCounts& pieceCounts)
{
  const auto typeToRanks_=typeToRanks(pieceCounts);
  TypeToType currentRemapping;
  fill(currentRemapping,FIRST_PIECE_TYPE);
  const auto result=typeRemappings(pieceCounts,typeToRanks_,SECOND_PIECE_TYPE,currentRemapping);
  assert(!result.empty());
  return result;
}

std::vector<TurnState::TypeToType> TurnState::typeRemappings(const PieceCounts& pieceCounts,const TypeToType& typeToRanks,const PieceType source,TypeToType& currentRemapping)
{
  if (source==NUM_PIECE_TYPES) {
    assert(currentRemapping[FIRST_PIECE_TYPE]==FIRST_PIECE_TYPE);
    return {currentRemapping};
  }
  else {
    const auto nextSource=PieceType(source+1);

    unsigned int maxPieceCount=0;
    for (Side side=FIRST_SIDE;side<NUM_SIDES;increment(side))
      maxPieceCount=std::max(maxPieceCount,pieceCounts[toPieceTypeAndSide(source,side)]);

    if (maxPieceCount==0)
      return typeRemappings(pieceCounts,typeToRanks,nextSource,currentRemapping);
    else {
      std::vector<TypeToType> result;

      const auto rank=typeToRanks[source];
      PieceType target=FIRST_PIECE_TYPE;
      for (PieceType weakerSource=SECOND_PIECE_TYPE;typeToRanks[weakerSource]<rank;increment(weakerSource))
        target=std::max(target,currentRemapping[weakerSource]);

      for (increment(target);target<NUM_PIECE_TYPES;increment(target)) {
        if (maxPieceCount<=numStartingPiecesPerType[target] && !found(currentRemapping,target)) {
          currentRemapping[source]=target;
          append(result,typeRemappings(pieceCounts,typeToRanks,nextSource,currentRemapping));
          currentRemapping[source]=FIRST_PIECE_TYPE;
        }
      }
      return result;
    }
  }
}
