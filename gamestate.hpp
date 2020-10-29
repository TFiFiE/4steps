#ifndef GAMESTATE_HPP
#define GAMESTATE_HPP

#include <array>
#include "def.hpp"

class GameState {
public:
  typedef std::array<PieceTypeAndSide,NUM_SQUARES> Board;

  explicit GameState();
  bool operator==(const GameState& rhs) const;
  bool empty() const;
  Placements placements(const Side side) const;
  Placements playedPlacements() const;
  std::array<bool,NUM_PIECE_SIDE_COMBINATIONS> piecesAtMax() const;
  bool isSupported(const SquareIndex square,const Side side) const;
  bool isFrozen(const SquareIndex square) const;
  bool floatingPiece(const SquareIndex square) const;
  bool legalPosition() const;
  Squares legalDestinations(const SquareIndex origin) const;
  bool legalOrigin(const SquareIndex square) const;
  std::vector<std::vector<ExtendedStep> > legalRoutes(const SquareIndex origin,const SquareIndex destination) const;
  bool legalStep(const SquareIndex origin,const SquareIndex destination) const;
  ExtendedSteps preferredRoute(const SquareIndex origin,const SquareIndex destination,const ExtendedSteps& preference=ExtendedSteps()) const;
  ExtendedSteps toExtendedSteps(const PieceSteps& pieceSteps) const;

  void add(const Placements& placements);
  PieceTypeAndSide takeStep(const SquareIndex origin,const SquareIndex destination);
  ExtendedStep takeExtendedStep(const SquareIndex origin,const SquareIndex destination);
  ExtendedSteps takePieceSteps(const PieceSteps& pieceSteps);
  void switchTurn();

  Side sideToMove;
  Board currentPieces;
  int stepsAvailable;
  bool inPush;
  SquareIndex followupDestination;
  std::set<SquareIndex> followupOrigins;
};

#endif // GAMESTATE_HPP
