#include "gamestate.hpp"

GameState::GameState() :
  sideToMove(FIRST_SIDE),
  stepsAvailable(MAX_STEPS_PER_MOVE),
  inPush(false),
  followupDestination(NO_SQUARE)
{
  fill(currentPieces,NO_PIECE);
}

bool GameState::operator==(const GameState& rhs) const
{
  return sideToMove==rhs.sideToMove &&
         currentPieces==rhs.currentPieces &&
         stepsAvailable==rhs.stepsAvailable &&
         inPush==rhs.inPush &&
         followupDestination==rhs.followupDestination &&
         followupOrigins==rhs.followupOrigins;
}

bool GameState::empty() const
{
  for (SquareIndex square=FIRST_SQUARE;square<NUM_SQUARES;increment(square))
    if (currentPieces[square]!=NO_PIECE)
      return false;
  return true;
}

Placement GameState::placement(const Side side) const
{
  Placement result;
  for (SquareIndex square=FIRST_SQUARE;square<NUM_SQUARES;increment(square)) {
    const PieceTypeAndSide pieceType=currentPieces[square];
    if (isSide(pieceType,side))
      result.emplace(square,pieceType);
  }
  return result;
}

bool GameState::isSupported(const SquareIndex square,const Side side) const
{
  for (const auto adjacentSquare:adjacentSquares(square))
    if (isSide(currentPieces[adjacentSquare],side))
      return true;
  return false;
}

bool GameState::isFrozen(const SquareIndex square) const
{
  const PieceTypeAndSide piece=currentPieces[square];
  const Side side=toSide(piece);
  bool dominatingNeighbor=false;
  for (const auto adjacentSquare:adjacentSquares(square)) {
    const PieceTypeAndSide neighbor=currentPieces[adjacentSquare];
    if (isSide(neighbor,side))
      return false;
    else
      dominatingNeighbor|=dominates(neighbor,piece);
  }
  return dominatingNeighbor;
}

bool GameState::floatingPiece(const SquareIndex square) const
{
  if (isTrap(square)) {
    const PieceTypeAndSide pieceTypeAndSide=currentPieces[square];
    if (pieceTypeAndSide!=NO_PIECE) {
      const Side pieceSide=toSide(pieceTypeAndSide);
      for (const auto adjacentSquare:adjacentSquares(square))
        if (isSide(currentPieces[adjacentSquare],pieceSide))
          return false;
      return true;
    }
  }
  return false;
}

bool GameState::legalPosition() const
{
  for (SquareIndex square=FIRST_SQUARE;square<NUM_SQUARES;increment(square))
    if (floatingPiece(square))
      return false;
  return true;
}

Squares GameState::legalDestinations(const SquareIndex origin) const
{
  Squares result;
  for (const auto adjacentSquare:adjacentSquares(origin))
    if (legalStep(origin,adjacentSquare))
      result.push_back(adjacentSquare);
  return result;
}

bool GameState::legalOrigin(const SquareIndex square) const
{
  return !legalDestinations(square).empty();
}

bool GameState::legalStep(const SquareIndex origin,const SquareIndex destination) const
{
  if (stepsAvailable==0 || !isAdjacent(origin,destination) || currentPieces[destination]!=NO_PIECE)
    return false;
  const PieceTypeAndSide piece=currentPieces[origin];
  if (piece==NO_PIECE)
    return false;
  else if (toSide(piece)==sideToMove)
    return !isFrozen(origin) &&
           (!inPush || (destination==followupDestination && found(followupOrigins,origin))) &&
           (toPieceType(piece)!=RESTRICTED_PIECE_TYPE || !restrictedDirection(sideToMove,origin,destination));
  else {
    if (inPush)
      return false;
    else if (destination==followupDestination && found(followupOrigins,origin)) // pull
      return true;
    else if (stepsAvailable<=1)
      return false;
    else {
      for (const auto adjacentSquare:adjacentSquares(origin))
        if (dominates(currentPieces[adjacentSquare],piece) && !isFrozen(adjacentSquare)) // start of push
          return true;
      return false;
    }
  }
}

std::vector<ExtendedSteps> GameState::legalRoutes(const SquareIndex origin,const SquareIndex destination) const
{
  if (origin==destination)
    return std::vector<ExtendedSteps>(1);
  std::vector<ExtendedSteps> result;
  for (const auto adjacentSquare:adjacentSquares(origin))
    if (legalStep(origin,adjacentSquare)) {
      GameState changedState(*this);
      const PieceTypeAndSide trappedPiece=changedState.takeStep(origin,adjacentSquare);
      const ExtendedStep step(origin,adjacentSquare,trappedPiece,changedState);
      for (auto movement:changedState.legalRoutes(adjacentSquare,destination)) {
        movement.insert(movement.begin(),step);
        result.push_back(movement);
      }
    }
  return result;
}

struct PreferredRoute {
  const ExtendedSteps& preference;
  PreferredRoute(const ExtendedSteps& preference_) : preference(preference_) {}
  bool operator()(const ExtendedSteps& lhs,const ExtendedSteps& rhs) const
  {
    if (lhs.size()!=rhs.size())
      return lhs.size()<rhs.size();

    const size_t minSize=std::min(preference.size(),lhs.size());
    for (size_t index=0;index<minSize;++index) {
      const int diff=distance(std::get<DESTINATION>(preference[index]),std::get<DESTINATION>(lhs[index]))-
                     distance(std::get<DESTINATION>(preference[index]),std::get<DESTINATION>(rhs[index]));
      if (diff!=0)
        return diff<0;
    }
    return false;
  }
};

ExtendedSteps GameState::preferredRoute(const SquareIndex origin,const SquareIndex destination,const ExtendedSteps& preference) const
{
  const std::vector<ExtendedSteps> routes=legalRoutes(origin,destination);
  const auto result=min_element(routes.begin(),routes.end(),PreferredRoute(preference));
  return result==routes.end() ? ExtendedSteps() : *result;
}

ExtendedSteps GameState::toExtendedSteps(const PieceSteps& pieceSteps) const
{
  return GameState(*this).takePieceSteps(pieceSteps);
}

void GameState::add(const Placement& placement)
{
  for (const auto& pair:placement) {
    PieceTypeAndSide& pieceOnSquare=currentPieces[pair.first];
    runtime_assert(pieceOnSquare==NO_PIECE,"Square already has a piece.");
    pieceOnSquare=pair.second;
  }
}

PieceTypeAndSide GameState::takeStep(const SquareIndex origin,const SquareIndex destination)
{
  runtime_assert(legalStep(origin,destination),"Not a legal step.");
  const PieceTypeAndSide piece=currentPieces[origin];
  const Side movingSide=toSide(piece);
  currentPieces[destination]=piece;
  currentPieces[origin]=NO_PIECE;
  PieceTypeAndSide victim=NO_PIECE;
  for (const auto possibleTrapSquare:adjacentSquares(origin))
    if (isTrap(possibleTrapSquare)) {
      PieceTypeAndSide& possibleVictim=currentPieces[possibleTrapSquare];
      if (isSide(possibleVictim,movingSide) && !isSupported(possibleTrapSquare,movingSide)) {
        assert(victim==NO_PIECE);
        victim=possibleVictim;
        possibleVictim=NO_PIECE;
      }
    }
  if (inPush) {
    assert(sideToMove==movingSide);
    inPush=false;
    // Disallow pull with push
    followupDestination=NO_SQUARE;
    followupOrigins.clear();
  }
  else if (sideToMove==movingSide) {
    // slide or start of pull
    followupDestination=origin;
    followupOrigins.clear();
    for (const auto adjacentSquare:adjacentSquares(origin)) {
      const PieceTypeAndSide adjacentPiece=currentPieces[adjacentSquare];
      if (adjacentPiece!=NO_PIECE && dominates(piece,adjacentPiece))
        followupOrigins.insert(adjacentSquare);
    }
  }
  else if (destination==followupDestination && found(followupOrigins,origin)) {
    // completion of pull
    followupDestination=NO_SQUARE;
    followupOrigins.clear();
  }
  else {
    // start of push
    inPush=true;
    followupDestination=origin;
    followupOrigins.clear();
    for (const auto adjacentSquare:adjacentSquares(origin))
      if (dominates(currentPieces[adjacentSquare],piece) && !isFrozen(adjacentSquare))
        followupOrigins.insert(adjacentSquare);
  }
  --stepsAvailable;
  return victim;
}

ExtendedSteps GameState::takePieceSteps(const PieceSteps& pieceSteps)
{
  ExtendedSteps result;
  for (const auto& pieceStep:pieceSteps) {
    const SquareIndex origin=std::get<ORIGIN>(pieceStep);
    const SquareIndex destination=std::get<DESTINATION>(pieceStep);
    runtime_assert(currentPieces[origin]==std::get<STEPPING_PIECE>(pieceStep),"Origin does not have given stepping piece.");
    const PieceTypeAndSide trappedPiece=takeStep(origin,destination);
    runtime_assert(trappedPiece==std::get<TRAPPED_PIECE>(pieceStep),"No trapped piece as given.");
    result.emplace_back(origin,destination,trappedPiece,*this);
  }
  return result;
}

void GameState::switchTurn()
{
  assert(!inPush);
  sideToMove=otherSide(sideToMove);
  stepsAvailable=MAX_STEPS_PER_MOVE;
  followupDestination=NO_SQUARE;
  followupOrigins.clear();
}
