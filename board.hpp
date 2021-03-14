#ifndef BOARD_HPP
#define BOARD_HPP

#include <QWidget>
#include <QTimer>
#include <QMediaPlayer>
#include <QMediaPlaylist>
struct Globals;
#include "readonly.hpp"
#include "pieceicons.hpp"
#include "gamestate.hpp"
#include "potentialmove.hpp"

class Board : public QWidget {
  Q_OBJECT
public:
  enum CoordinateDisplay {
    NONE,
    TRAPS_ONLY,
    ALL
  };
  enum SquareColorIndex {
    REGULAR,
    GOAL,
    TRAP=GOAL+NUM_SIDES,
    HIGHLIGHT=TRAP+NUM_SIDES,
    HIGHLIGHT_MILD=HIGHLIGHT+NUM_SIDES,
    ALTERNATIVE=HIGHLIGHT_MILD+NUM_SIDES,
    NUM_SQUARE_COLORS=ALTERNATIVE*2
  };

  explicit Board(Globals& globals_,NodePtr currentNode_,const bool explore_,const Side viewpoint,const bool soundOn,const std::array<bool,NUM_SIDES>& controllableSides_={true,true},const TurnState* const customSetup_=nullptr,QWidget* const parent=nullptr,const Qt::WindowFlags f=Qt::WindowFlags());
  bool customSetup() const;
  bool setupPhase() const;
  bool setupPlacementPhase() const;
  Side sideToMove() const;
  const GameState& gameState() const;
  const GameState& displayedGameState() const;
  Placements currentPlacements() const;
  std::pair<Placements,ExtendedSteps> tentativeMove() const;
  std::string tentativeMoveString() const;
  bool gameEnd() const;
  bool playable() const;

  bool setNode(NodePtr newNode,const bool silent=true,bool keepState=false);
  void playMoveSounds(const Node& node);
  void proposeMove(const Node& child,const unsigned int playedOutSteps);
  void proposeCustomSetup(const TurnState& turnState);
  bool proposeSetup(const Placements& placements);
  void proposeSetup(const Node& child);
  void doSteps(const ExtendedSteps& potentialMove,const bool sound,const int undoneSteps=0);
  void undoSteps(const bool all);
  void redoSteps(const bool all);
  void redoSteps(const ExtendedSteps& steps);
  void animateMove(const bool showStart);
  void rotate();
  void setViewpoint(const Side side);
  void setAutoRotate(const bool on);
  void toggleSound(const bool soundOn_);
  void setStepMode(const bool newStepMode);
  void setIconSet(const PieceIcons::Set newIconSet);
  void setCoordinateDisplay(const CoordinateDisplay newCoordinateDisplay);
  void setAnimate(const bool newAnimate);
  void setAnimationDelay(const int newAnimationDelay);
  void setVolume(const int newVolume);
  void setConfirm(const bool newConfirm);
  void setColor(const unsigned int colorIndex,const QColor& newColor);
  void playSound(const QString& soundFile,const bool override=false);
  void setExploration(const bool on);
  void setControllable(const std::array<bool,NUM_SIDES>& controllableSides_);

  readonly<Board,bool> explore,southIsUp,stepMode,soundOn,animate,confirm;
  readonly<Board,PieceIcons::Set> iconSet;
  readonly<Board,CoordinateDisplay> coordinateDisplay;
  readonly<Board,NodePtr> currentNode;
  readonly<Board,int> animationDelay,volume;
  readonly<Board,PotentialMove> potentialMove;

  readonly<Board,QColor> colors[NUM_SQUARE_COLORS];
  SquareColorIndex customColors[NUM_SQUARES];
private:
  int squareWidth() const;
  int squareHeight() const;
  void normalizeOrientation(unsigned int& file,unsigned int& rank) const;
  QRect visualRectangle(unsigned int file,unsigned int rank) const;
  SquareIndex orientedSquare(unsigned int file,unsigned int rank) const;
  SquareIndex positionToSquare(const QPoint& position) const;
  static int closestAxisDirection(unsigned int value,const unsigned int size);
  SquareIndex closestAdjacentSquare(const QPoint& position) const;
  bool isSetupSquare(const Side side,const SquareIndex square) const;
  bool validDrop() const;
  bool isAnimating() const;

  template<class Type> bool setSetting(readonly<Board,Type>& currentValue,const Type newValue,const QString& key);
  void clearSetup();
  void initSetup();
  bool nextSetupPiece(const bool finalize=true);
  bool setUpPiece(const SquareIndex destination);
  bool refreshHighlights(const bool clearSelected);
  bool updateStepHighlights();
  bool updateStepHighlights(const QPoint& mousePosition);
  bool singleSquareAction(const SquareIndex square);
  bool doubleSquareAction(const SquareIndex origin,const SquareIndex destination);
  bool doubleSquareSetupAction(const SquareIndex origin,const SquareIndex destination);
  void finalizeSetup(const Placements& placements);
  void finalizeMove(const ExtendedSteps& playedMove);
  void endDrag();
  bool autoFinalize(const bool stepsTaken);
  bool confirmMove();

  virtual bool event(QEvent* event) override;
  virtual void mousePressEvent(QMouseEvent* event) override;
  virtual void mouseMoveEvent(QMouseEvent* event) override;
  virtual void mouseReleaseEvent(QMouseEvent* event) override;
  virtual void mouseDoubleClickEvent(QMouseEvent* event) override;
  virtual void wheelEvent(QWheelEvent* event) override;
  virtual void focusOutEvent(QFocusEvent*) override;
  virtual void paintEvent(QPaintEvent*) override;

  void animateNextStep();
  void disableAnimation();
  void playStepSounds(const ExtendedSteps& steps,const bool emphasize);
  bool playCaptureSound(const ExtendedStep& step);

  Globals& globals;
  QTimer animationTimer;
  ExtendedSteps::const_iterator nextAnimatedStep;
  QMediaPlayer qMediaPlayer;
  QMediaPlaylist qMediaPlaylist;

  GameState potentialSetup;
  PieceType currentSetupPiece;
  std::array<bool,NUM_SIDES> controllableSides;
  bool autoRotate;
  std::array<SquareIndex,2> drag;
  ExtendedSteps dragSteps;
  std::array<SquareIndex,2> highlighted;

  const char* const colorKeys[NUM_SQUARE_COLORS];
signals:
  void gameStarted();
  void boardChanged(const bool refresh=true);
  void boardRotated(const bool southIsUp);
  void sendNodeChange(const NodePtr& newNode,const NodePtr& oldNode);

  friend class Palette;
  friend class OffBoard;
};

#endif // BOARD_HPP
