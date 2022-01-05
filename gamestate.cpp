#include "gamestate.hpp"

GameState::GameState(const TurnState& turnState) :
  TurnState(turnState),
  stepsAvailable(MAX_STEPS_PER_MOVE),
  inPush(false),
  followupDestination(NO_SQUARE)
{
}

bool GameState::operator==(const GameState& rhs) const
{
  return sideToMove==rhs.sideToMove &&
         squarePieces==rhs.squarePieces &&
         stepsAvailable==rhs.stepsAvailable &&
         inPush==rhs.inPush &&
         followupDestination==rhs.followupDestination &&
         followupOrigins==rhs.followupOrigins;
}

bool GameState::operator!=(const GameState& rhs) const
{
  return !(*this==rhs);
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
  if (stepsAvailable==0 || !isAdjacent(origin,destination) || squarePieces[destination]!=NO_PIECE)
    return false;
  const PieceTypeAndSide piece=squarePieces[origin];
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
        if (dominates(squarePieces[adjacentSquare],piece) && !isFrozen(adjacentSquare)) // start of push
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
      const ExtendedStep step=changedState.takeExtendedStep(origin,adjacentSquare);
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

PieceTypeAndSide GameState::takeStep(const SquareIndex origin,const SquareIndex destination)
{
  runtime_assert(legalStep(origin,destination),"Not a legal step.");
  const PieceTypeAndSide piece=squarePieces[origin];
  const Side movingSide=toSide(piece);
  squarePieces[destination]=piece;
  squarePieces[origin]=NO_PIECE;
  PieceTypeAndSide victim=NO_PIECE;
  for (const auto possibleTrapSquare:adjacentSquares(origin))
    if (isTrap(possibleTrapSquare)) {
      PieceTypeAndSide& possibleVictim=squarePieces[possibleTrapSquare];
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
      const PieceTypeAndSide adjacentPiece=squarePieces[adjacentSquare];
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
      if (dominates(squarePieces[adjacentSquare],piece) && !isFrozen(adjacentSquare))
        followupOrigins.insert(adjacentSquare);
  }
  --stepsAvailable;
  return victim;
}

ExtendedStep GameState::takeExtendedStep(const SquareIndex origin,const SquareIndex destination)
{
  const PieceTypeAndSide trappedPiece=takeStep(origin,destination);
  return ExtendedStep(origin,destination,trappedPiece,*this);
}

ExtendedSteps GameState::takeSteps(const Steps& steps)
{
  ExtendedSteps result;
  result.reserve(steps.size());
  for (const auto& step:steps)
    result.emplace_back(takeExtendedStep(step.first,step.second));
  return result;
}

ExtendedSteps GameState::takePieceSteps(const PieceSteps& pieceSteps)
{
  ExtendedSteps result;
  result.reserve(pieceSteps.size());
  for (const auto& pieceStep:pieceSteps) {
    const SquareIndex origin=std::get<ORIGIN>(pieceStep);
    const SquareIndex destination=std::get<DESTINATION>(pieceStep);
    runtime_assert(squarePieces[origin]==std::get<STEPPING_PIECE>(pieceStep),"Origin does not have given stepping piece.");
    const PieceTypeAndSide trappedPiece=takeStep(origin,destination);
    runtime_assert(trappedPiece==std::get<TRAPPED_PIECE>(pieceStep),"No trapped piece as given.");
    result.emplace_back(origin,destination,trappedPiece,*this);
  }
  return result;
}

void GameState::switchTurn()
{
  TurnState::switchTurn();
  assert(!inPush);
  stepsAvailable=MAX_STEPS_PER_MOVE;
  followupDestination=NO_SQUARE;
  followupOrigins.clear();
}

void GameState::flipSides()
{
  TurnState::flipSides();
  transformExtra(invert);
}

void GameState::mirror()
{
  TurnState::mirror();
  transformExtra(::mirror);
}

template<class Function>
void GameState::transformExtra(Function function)
{
  followupDestination=function(followupDestination);
  std::set<SquareIndex> newFollowupOrigins;
  for (const auto& followupOrigin:followupOrigins)
    newFollowupOrigins.emplace(function(followupOrigin));
  followupOrigins=std::move(newFollowupOrigins);
}
