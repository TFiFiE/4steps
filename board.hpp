#ifndef BOARD_HPP
#define BOARD_HPP

#include <QWidget>
#include <QMediaPlayer>
#include <QMediaPlaylist>
class QSvgRenderer;
#include "tree.hpp"

class Board : public QWidget {
  Q_OBJECT
public:
  explicit Board(QSvgRenderer pieceIcons_[NUM_PIECE_SIDE_COMBINATIONS],const Side viewpoint,const std::array<bool,NUM_SIDES>& controllableSides_={true,true},QWidget* const parent=nullptr,const Qt::WindowFlags f=Qt::WindowFlags());
  bool setupPhase() const;
  Side sideToMove() const;
  MoveTree& currentMoveNode() const;
  std::vector<GameState> history() const;
  const GameState& gameState() const;
  bool playable() const;
  void receiveSetup(const Placement& placement,const bool sound);
  void receiveMove(const PieceSteps& pieceSteps,const bool sound);
  void receiveGameTree(const GameTreeNode& gameTreeNode,const bool sound);
  void rotate();
  void setViewpoint(const Side side);
  void setAutoRotate(const bool on);
  void toggleSound(const bool soundOn_);
  void setStepMode(const bool newStepMode);
  void setSizeHint(const QSize newSizeHint);
  void playSound(const QString& soundFile);
  void setControllable(const std::array<bool,NUM_SIDES>& sides);
  std::array<bool,NUM_SIDES> controllableSides() const {return controllableSides_;}
  bool southIsUp() const {return southIsUp_;}
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
  bool validDrop() const;

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

  void mousePressEvent(QMouseEvent* event) override;
  void mouseMoveEvent(QMouseEvent* event) override;
  void mouseReleaseEvent(QMouseEvent* event) override;
  void mouseDoubleClickEvent(QMouseEvent* event) override;
  void keyPressEvent(QKeyEvent* event) override;
  void focusOutEvent(QFocusEvent*) override;
  void paintEvent(QPaintEvent*) override;
  QSize sizeHint() const override;

  void playStepSounds(const ExtendedSteps& steps,const bool emphasize);

  QSvgRenderer* const pieceIcons;
  QMediaPlayer qMediaPlayer;
  QMediaPlaylist qMediaPlaylist;
  QSize sizeHint_;

  GameTreeNode currentNode;
  GameState potentialSetup;
  ExtendedSteps potentialMove;
  ExtendedSteps::const_iterator afterCurrentStep;
  std::array<unsigned int,NUM_PIECE_TYPES-1> numSetupPieces;
  int currentSetupPiece;
  std::array<bool,NUM_SIDES> controllableSides_;
  bool southIsUp_,autoRotate,soundOn;
  std::array<SquareIndex,2> drag;
  ExtendedSteps dragSteps;
  bool stepMode;
  std::array<SquareIndex,2> highlighted;
signals:
  void gameStarted();
  void boardChanged();
  void boardRotated(const bool southIsUp);
  void sendSetup(const Placement& placement,const Side side);
  void sendMove(const ExtendedSteps& move,const Side side);
};

#endif // BOARD_HPP
