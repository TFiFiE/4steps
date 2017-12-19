#ifndef DEF_HPP
#define DEF_HPP

#include <vector>
#include <set>
#include <tuple>
#include <list>
#include <cassert>
#include <QString>

enum Side {
  FIRST_SIDE=0,
  SECOND_SIDE,
  NUM_SIDES,
  NO_SIDE=NUM_SIDES
};

enum PieceType {
  FIRST_PIECE_TYPE=0,
  NUM_PIECE_TYPES=6,
  RESTRICTED_PIECE_TYPE=FIRST_PIECE_TYPE,
  WINNING_PIECE_TYPE=FIRST_PIECE_TYPE
};

enum {
  NUM_RANKS=8,
  NUM_FILES=8,
  NUM_SETUP_RANKS=2,
  NUM_PIECE_SIDE_COMBINATIONS=NUM_PIECE_TYPES*NUM_SIDES,
  MAX_ALLOWED_REPETITIONS=1,
  MAX_STEPS_PER_MOVE=4
};

enum SquareIndex {
  NO_SQUARE=-1,
  FIRST_SQUARE=0,
  NUM_SQUARES=NUM_RANKS*NUM_FILES
};

enum PieceTypeAndSide {
  NO_PIECE=-1
};

enum Direction {
  SOUTH,
  WEST,
  EAST,
  NORTH
};

enum MoveLegality {
  LEGAL,
  ILLEGAL_PUSH_INCOMPLETION,
  ILLEGAL_PASS,
  ILLEGAL_REPETITION
};

enum EndCondition {
  NO_END,
  GOAL,
  IMMOBILIZATION,
  ELIMINATION,
  TIME_OUT,
  RESIGNATION,
  ILLEGAL_MOVE,
  FORFEIT,
  ABANDONMENT,
  SCORE,
  REPETITION,
  MUTUAL_ELIMINATION
};

struct Result {
  Side winner;
  EndCondition endCondition;
  bool operator==(const Result& rhs) const {return winner==rhs.winner && endCondition==rhs.endCondition;}
  bool operator!=(const Result& rhs) const {return !(*this==rhs);}
};

typedef std::vector<SquareIndex> Squares;
typedef std::set<std::pair<SquareIndex,PieceTypeAndSide> > Placement;

class GameState;
typedef std::tuple<SquareIndex,SquareIndex,PieceTypeAndSide,GameState> ExtendedStep;
enum {ORIGIN,DESTINATION,TRAPPED_PIECE,RESULTING_STATE};
typedef std::vector<ExtendedStep> ExtendedSteps;

typedef std::tuple<SquareIndex,SquareIndex,PieceTypeAndSide,PieceTypeAndSide> PieceStep;
enum {STEPPING_PIECE=RESULTING_STATE};
typedef std::vector<PieceStep> PieceSteps;

struct MoveTree;
typedef std::list<MoveTree> MoveTrees;
typedef std::list<std::pair<Placement,MoveTrees> > GameTree;

const unsigned int numStartingPiecesPerType[]={8,2,2,2,1,1};

template<class T>
inline void increment(T& t)
{
  t=static_cast<T>(t+1);
}

template<class Container,class Element>
inline bool found(const Container& container,const Element element)
{
  return std::find(container.begin(),container.end(),element)!=container.end();
}

template<int tupleIndex,class Container,class Element>
inline bool found(const Container& container,const Element element)
{
  return find_if(container.begin(),container.end(),[&](const typename Container::value_type& tuple){return std::get<tupleIndex>(tuple)==element;})!=container.end();
}

template<class Container,class Element>
inline void fill(Container& container,const Element element)
{
  std::fill(container.begin(),container.end(),element);
}

template<class Container>
inline void append(Container& target,const Container& extension)
{
  target.insert(target.end(),extension.begin(),extension.end());
}

inline PieceTypeAndSide toPieceTypeAndSide(const PieceType pieceType,const Side side)
{
  return static_cast<PieceTypeAndSide>(side+pieceType*NUM_SIDES);
}

inline PieceType toPieceType(const PieceTypeAndSide pieceTypeAndSide)
{
  assert(pieceTypeAndSide!=NO_PIECE);
  return static_cast<PieceType>(pieceTypeAndSide/NUM_SIDES);
}

inline Side toSide(const PieceTypeAndSide pieceTypeAndSide)
{
  if (pieceTypeAndSide==NO_PIECE)
    return NO_SIDE;
  return static_cast<Side>(pieceTypeAndSide%NUM_SIDES);
}

inline Side otherSide(const Side side)
{
  return static_cast<Side>(!side);
}

inline SquareIndex toSquare(const unsigned int file,const unsigned int rank)
{
  if (file>=NUM_FILES || rank>=NUM_RANKS)
    return NO_SQUARE;
  return static_cast<SquareIndex>(file+rank*NUM_FILES);
}

inline int toFile(const SquareIndex square)
{
  assert(square!=NO_SQUARE);
  return square%NUM_FILES;
}

inline int toRank(const SquareIndex square)
{
  assert(square!=NO_SQUARE);
  return square/NUM_FILES;
}

inline bool isTrap(const unsigned int file,const unsigned int rank)
{
  return (file==2 || file==5) && (rank==2 || rank==5);
}

inline bool isTrap(const SquareIndex square)
{
  return isTrap(toFile(square),toRank(square));
}

inline bool isGoal(const SquareIndex square,const Side side)
{
  return toRank(square)==(side==FIRST_SIDE ? NUM_RANKS-1 : 0);
}

inline int distance(const SquareIndex square1,const SquareIndex square2)
{
  if (square1==NO_SQUARE || square2==NO_SQUARE)
    return -1;
  return std::abs(toFile(square1)-
                  toFile(square2))+
         std::abs(toRank(square1)-
                  toRank(square2));
}

inline bool isAdjacent(const SquareIndex square1,const SquareIndex square2)
{
  return distance(square1,square2)==1;
}

inline bool isSetupRank(const Side side,const unsigned int rank)
{
  return side==0 ? rank<NUM_SETUP_RANKS : rank>=NUM_RANKS-NUM_SETUP_RANKS;
}

inline bool isSetupSquare(const Side side,const SquareIndex square)
{
  return square!=NO_SQUARE && isSetupRank(side,toRank(square));
}

inline bool isSide(const PieceTypeAndSide pieceTypeAndSide,const Side side)
{
  return pieceTypeAndSide!=NO_PIECE && toSide(pieceTypeAndSide)==side;
}

inline bool dominates(const PieceTypeAndSide dominator,const PieceTypeAndSide victim)
{
  assert(victim!=NO_PIECE);
  return dominator!=NO_PIECE && toSide(dominator)!=toSide(victim) && toPieceType(dominator)>toPieceType(victim);
}

inline bool restrictedDirection(const Side side,const SquareIndex origin,const SquareIndex destination)
{
  return destination-origin==(side==FIRST_SIDE ? -NUM_FILES : NUM_FILES);
}

inline Squares adjacentSquares(const SquareIndex square)
{
  if (square==NO_SQUARE)
    return Squares();
  const unsigned int file=toFile(square);
  const unsigned int rank=toRank(square);
  Squares result;
  if (file>0)
    result.push_back(toSquare(file-1,rank));
  if (file<NUM_FILES-1)
    result.push_back(toSquare(file+1,rank));
  if (rank>0)
    result.push_back(toSquare(file,rank-1));
  if (rank<NUM_RANKS-1)
    result.push_back(toSquare(file,rank+1));
  return result;
}

inline SquareIndex adjacentTrap(const SquareIndex trapNeighbor)
{
  for (const auto adjacentSquare:adjacentSquares(trapNeighbor))
    if (isTrap(adjacentSquare))
      return adjacentSquare;
  return NO_SQUARE;
}

inline SquareIndex toDestination(const SquareIndex origin,const Direction direction)
{
  const int directionToOffset[]={-NUM_FILES,-1,1,NUM_FILES};
  return static_cast<SquareIndex>(origin+directionToOffset[direction]);
}

inline std::string replaceString(std::string subject,const std::string& search,const std::string& replace)
{
  size_t pos=0;
  while ((pos=subject.find(search,pos))!=std::string::npos) {
    subject.replace(pos,search.length(),replace);
    pos+=replace.length();
  }
  return subject;
}

inline bool startsWith(const std::string& string,const std::string& prefix)
{
  return string.compare(0,prefix.size(),prefix)==0;
}

template<class Widget>
inline int textWidth(const Widget& widget)
{
  return widget.fontMetrics().boundingRect(widget.text()).width()+10;
}

template<class String>
inline void runtime_assert(const bool condition,const String& message)
{
  if (!condition)
    throw std::runtime_error(message);
}

inline void runtime_assert(const bool condition,const QString& message)
{
  runtime_assert(condition,message.toStdString());
}

#if __cplusplus <= 201103L
#include <memory>

template<typename T,typename... Args>
std::unique_ptr<T> make_unique(Args&&... args)
{
  return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}
#endif

#endif // DEF_HPP
