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
  explicit Board(Globals& globals_,NodePtr currentNode_,const bool explore_,const Side viewpoint,const bool soundOn,const std::array<bool,NUM_SIDES>& controllableSides_={true,true},QWidget* const parent=nullptr,const Qt::WindowFlags f=Qt::WindowFlags());
  bool customSetup() const;
  bool setupPhase() const;
  bool setupPlacementPhase() const;
  Side sideToMove() const;
  const GameState& gameState() const;
  const GameState& displayedGameState() const;
  Placements currentPlacements() const;
  std::string tentativeMoveString() const;
  bool gameEnd() const;
  bool playable() const;

  bool setNode(NodePtr newNode,const bool sound=false,bool keepState=false);
  void playMoveSounds(const Node& node);
  void proposeMove(const Node& child,const unsigned int playedOutSteps);
  void proposeSetup(GameState gameState);
  void doSteps(const ExtendedSteps& potentialMove,const bool sound,const int undoneSteps=0);
  void undoSteps(const bool all);
  void redoSteps(const bool all);
  void rotate();
  void setViewpoint(const Side side);
  void setAutoRotate(const bool on);
  void toggleSound(const bool soundOn_);
  void setStepMode(const bool newStepMode);
  void setIconSet(const PieceIcons::Set newIconSet);
  void setAnimate(const bool newAnimate);
  void setAnimationDelay(const int newAnimationDelay);
  void setConfirm(const bool newConfirm);
  void playSound(const QString& soundFile);
  void setExploration(const bool on);
  void setControllable(const std::array<bool,NUM_SIDES>& controllableSides_);

  readonly<Board,bool> explore,southIsUp,stepMode,soundOn,animate,confirm;
  readonly<Board,PieceIcons::Set> iconSet;
  readonly<Board,NodePtr> currentNode;
  readonly<Board,int> animationDelay;
  readonly<Board,PotentialMove> potentialMove;
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
  void initSetup();
  void nextSetupPiece();
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

  virtual void mousePressEvent(QMouseEvent* event) override;
  virtual void mouseMoveEvent(QMouseEvent* event) override;
  virtual void mouseReleaseEvent(QMouseEvent* event) override;
  virtual void mouseDoubleClickEvent(QMouseEvent* event) override;
  virtual void wheelEvent(QWheelEvent* event) override;
  virtual void focusOutEvent(QFocusEvent*) override;
  virtual void paintEvent(QPaintEvent*) override;

  void animateMove(const bool showStart);
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
  std::array<unsigned int,NUM_PIECE_TYPES-1> numSetupPieces;
  int currentSetupPiece;
  std::array<bool,NUM_SIDES> controllableSides;
  bool autoRotate;
  std::array<SquareIndex,2> drag;
  ExtendedSteps dragSteps;
  std::array<SquareIndex,2> highlighted;
  const QColor neutralColor;
signals:
  void gameStarted();
  void boardChanged();
  void boardRotated(const bool southIsUp);
  void sendNodeChange(const NodePtr& newNode,const NodePtr& oldNode);

  friend class Popup;
  friend class OffBoard;
};

#endif // BOARD_HPP
