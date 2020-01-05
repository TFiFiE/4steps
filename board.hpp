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

class Board : public QWidget {
  Q_OBJECT
public:
  explicit Board(Globals& globals_,const Side viewpoint,const bool soundOn,const std::array<bool,NUM_SIDES>& controllableSides_={true,true},const bool customSetup_=false,QWidget* const parent=nullptr,const Qt::WindowFlags f=Qt::WindowFlags());
  bool setupPhase() const;
  Side sideToMove() const;
  const NodePtr& currentNode() const;
  NodePtr& currentNode();
  const GameState& gameState() const;
  const GameState& displayedGameState() const;
  bool playable() const;
  void receiveGameTree(const GameTree& gameTreeNode,const bool sound);
  void rotate();
  void setViewpoint(const Side side);
  void setAutoRotate(const bool on);
  void toggleSound(const bool soundOn_);
  void setStepMode(const bool newStepMode);
  void setIconSet(const PieceIcons::Set newIconSet);
  void setAnimate(const bool newAnimate);
  void setAnimationDelay(const int newAnimationDelay);
  void playSound(const QString& soundFile);
  void setControllable(const std::array<bool,NUM_SIDES>& controllableSides_);

  readonly<Board,bool> southIsUp,stepMode,soundOn,animate;
  readonly<Board,PieceIcons::Set> iconSet;
  readonly<Board,GameTree> gameTree;
  const NodePtr root;
  readonly<Board,int> animationDelay;
private:
  int squareWidth() const;
  int squareHeight() const;
  void normalizeOrientation(unsigned int& file,unsigned int& rank) const;
  QRect visualRectangle(unsigned int file,unsigned int rank) const;
  SquareIndex orientedSquare(unsigned int file,unsigned int rank) const;
  SquareIndex positionToSquare(const QPoint& position) const;
  static int closestAxisDirection(unsigned int value,const unsigned int size);
  SquareIndex closestAdjacentSquare(const QPoint& position) const;
  bool setupPlacementPhase() const;
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
  void doSteps(const ExtendedSteps& potentialMove);
  void finalizeSetup(const Placement& placement,const bool sound);
  void finalizeMove(const ExtendedSteps& playedMove);
  void endDrag();
  void undoSteps(const bool all);
  void redoSteps(const bool all);

  void mousePressEvent(QMouseEvent* event) override;
  void mouseMoveEvent(QMouseEvent* event) override;
  void mouseReleaseEvent(QMouseEvent* event) override;
  void mouseDoubleClickEvent(QMouseEvent* event) override;
  void wheelEvent(QWheelEvent* event) override;
  void focusOutEvent(QFocusEvent*) override;
  void paintEvent(QPaintEvent*) override;

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
  ExtendedSteps potentialMove;
  ExtendedSteps::const_iterator afterCurrentStep;
  std::array<unsigned int,NUM_PIECE_TYPES-1> numSetupPieces;
  int currentSetupPiece;
  std::array<bool,NUM_SIDES> controllableSides;
  bool customSetup;
  bool autoRotate;
  std::array<SquareIndex,2> drag;
  ExtendedSteps dragSteps;
  std::array<SquareIndex,2> highlighted;
  const QColor neutralColor;
signals:
  void gameStarted();
  void boardChanged();
  void boardRotated(const bool southIsUp);
  void sendSetup(const Placement& placement,const Side side);
  void sendMove(const ExtendedSteps& move,const Side side);

  friend class Popup;
};

#endif // BOARD_HPP
