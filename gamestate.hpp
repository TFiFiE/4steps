#ifndef GAMESTATE_HPP
#define GAMESTATE_HPP

#include <array>
#include "turnstate.hpp"

class GameState : public TurnState {
public:
  typedef std::array<PieceTypeAndSide,NUM_SQUARES> Board;

  explicit GameState(const TurnState& turnState=TurnState());

  bool operator==(const GameState& rhs) const;
  Squares legalDestinations(const SquareIndex origin) const;
  bool legalOrigin(const SquareIndex square) const;
  bool legalStep(const SquareIndex origin,const SquareIndex destination) const;
  std::vector<std::vector<ExtendedStep> > legalRoutes(const SquareIndex origin,const SquareIndex destination) const;
  ExtendedSteps preferredRoute(const SquareIndex origin,const SquareIndex destination,const ExtendedSteps& preference=ExtendedSteps()) const;

  PieceTypeAndSide takeStep(const SquareIndex origin,const SquareIndex destination);
  ExtendedStep takeExtendedStep(const SquareIndex origin,const SquareIndex destination);
  ExtendedSteps takePieceSteps(const PieceSteps& pieceSteps);
  virtual void switchTurn() override;

  int stepsAvailable;
  bool inPush;
  SquareIndex followupDestination;
  std::set<SquareIndex> followupOrigins;
};

#endif // GAMESTATE_HPP
