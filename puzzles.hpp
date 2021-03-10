#ifndef PUZZLES_HPP
#define PUZZLES_HPP

#include <deque>
#include <QElapsedTimer>
#include <QSpinBox>
#include "game.hpp"

class Puzzles : public Game {
public:
  Puzzles(Globals& globals_,const std::string& fileName,QWidget* const parent);
private:
  static constexpr auto SQUARE_BIT_SIZE=4;
  static constexpr auto SQUARE_DATA_RANGE=1<<SQUARE_BIT_SIZE;
  static constexpr auto SQUARES_PER_BYTES=CHAR_BIT/SQUARE_BIT_SIZE;
  static constexpr auto BYTES_PER_BOARD=roundedUpDivision(int(NUM_SQUARES),SQUARES_PER_BYTES);
  static constexpr auto PUZZLE_BYTE_SIZE=BYTES_PER_BOARD+MAX_STEPS_PER_MOVE;
  static constexpr unsigned char EMPTY_VALUE=SQUARE_DATA_RANGE-1;
  static constexpr unsigned char PASS=0;
  static constexpr auto DISPLAYED_INDEX_OFFSET=1;

  typedef std::array<unsigned char,PUZZLE_BYTE_SIZE> PuzzleData;
  typedef std::pair<TurnState,Steps> Puzzle;

  void setPuzzleCount();
  void loadPuzzles(const std::string& fileName);
  static Puzzle toPuzzle(const PuzzleData& puzzleData);
  static PuzzleData toData(Puzzle puzzle);
  static PieceTypeAndSide toSquareValue(const unsigned char bits);
  static unsigned char toData(const PieceTypeAndSide squareValue);
  static Step toStep(const unsigned char byte,const bool strict=true);
  static unsigned char toData(const Step& step);
  static void flipSides(Puzzle& puzzle);
  static void mirror(Puzzle& puzzle);
  template<unsigned int numValues,class Number> static std::array<Number,numValues> clock(const std::array<Number,numValues-1>& circles,Number number);
  static QString displayedTime(const qint64 milliseconds);
  void randomize(Puzzle& puzzle) const;
  void setPuzzle(unsigned int newIndex);
  void setPuzzle(Puzzle puzzle);
  void setPuzzle(const NodePtr node,const ExtendedSteps& newSolution);
  void clearHints();
  virtual void receiveNodeChange(const NodePtr& newNode) override;

  static const std::vector<SquareIndex> trapSquares;
  Globals& globals;
  std::deque<PuzzleData> puzzles;
  unsigned int puzzleIndex;
  ExtendedSteps solution;
  std::vector<SquareIndex> differentSquares;
  std::vector<SquareIndex>::iterator pastRevealed;
  PieceType pieceType;
  QElapsedTimer solveTimer;
  bool keepScore;

  class PieceIcon : public QWidget {
    Puzzles& puzzles;
  public:
    PieceIcon(Puzzles& puzzles_);
  private:
    virtual int heightForWidth(int w) const override;
    virtual void paintEvent(QPaintEvent*) override;
  };

  QWidget puzzleDock;
    QVBoxLayout layout;
      QPushButton reshuffle;
      QSpinBox spinBox;
      QCheckBox autoAdvance,autoUndo,sounds;
      PieceIcon pieceIcon;
      QLabel evaluation,solveTime;
      QPushButton resetPuzzle,hint,answer;
};

#endif // PUZZLES_HPP
