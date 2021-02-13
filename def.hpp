#ifndef DEF_HPP
#define DEF_HPP

#include <set>
#include <vector>
#include <tuple>
#include <memory>
#include <deque>
#include <array>
#include <cassert>
#include <QString>
#include <QDialog>
#include <QNetworkRequest>

enum Side {
  FIRST_SIDE=0,
  SECOND_SIDE,
  NUM_SIDES,
  NO_SIDE=NUM_SIDES
};

enum PieceType {
  FIRST_PIECE_TYPE=0,
  SECOND_PIECE_TYPE,
  NUM_PIECE_TYPES=6,
  LAST_PIECE_TYPE=NUM_PIECE_TYPES-1,
  RESTRICTED_PIECE_TYPE=FIRST_PIECE_TYPE,
  WINNING_PIECE_TYPE=FIRST_PIECE_TYPE
};

constexpr auto NUM_RANKS=8;
constexpr auto NUM_FILES=8;
constexpr auto NUM_SETUP_RANKS=2;
constexpr auto MAX_ALLOWED_REPETITIONS=1;
constexpr auto MAX_STEPS_PER_MOVE=4;

enum SquareIndex {
  NO_SQUARE=-1,
  FIRST_SQUARE=0,
  NUM_SQUARES=NUM_RANKS*NUM_FILES
};

enum PieceTypeAndSide {
  NUM_PIECE_SIDE_COMBINATIONS=NUM_PIECE_TYPES*NUM_SIDES,
  NO_PIECE=-1
};

enum Direction {
  SOUTH,
  WEST,
  EAST,
  NORTH,
  NUM_DIRECTIONS
};

enum struct MoveLegality {
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

struct Placement {
  SquareIndex location;
  PieceTypeAndSide piece;
  bool operator==(const Placement& rhs) const {return location==rhs.location && piece==rhs.piece;}
  bool operator<(const Placement& rhs) const {return piece!=rhs.piece ? piece>rhs.piece : location<rhs.location;}
  bool isValid() const {return location!=NO_SQUARE && piece!=NO_PIECE;}
  static Placement invalid() {return {NO_SQUARE,NO_PIECE};}
};

typedef std::set<Placement> Placements;
typedef std::vector<SquareIndex> Squares;
typedef std::pair<SquareIndex,SquareIndex> Step;
typedef std::vector<Step> Steps;

class GameState;
typedef std::tuple<SquareIndex,SquareIndex,PieceTypeAndSide,GameState> ExtendedStep;
enum {ORIGIN,DESTINATION,TRAPPED_PIECE,RESULTING_STATE};
typedef std::vector<ExtendedStep> ExtendedSteps;

typedef std::tuple<SquareIndex,SquareIndex,PieceTypeAndSide,PieceTypeAndSide> PieceStep;
enum {STEPPING_PIECE=RESULTING_STATE};
typedef std::vector<PieceStep> PieceSteps;

struct Node;
typedef std::shared_ptr<Node> NodePtr;
typedef std::deque<NodePtr> GameTree;

const std::array<unsigned int,NUM_PIECE_TYPES> numStartingPiecesPerType={8,2,2,2,1,1};
const unsigned int numStartingPieces=std::accumulate(std::begin(numStartingPiecesPerType),std::end(numStartingPiecesPerType),0);

template<class Integer>
inline Integer floorDiv(const Integer dividend,const Integer divisor)
{
  assert(divisor>0);
  return dividend>=0 ? dividend/divisor : -1-(-1-dividend)/divisor;
}

template<class Integer>
inline constexpr Integer roundedUpDivision(const Integer dividend,const Integer divisor)
{
  return (dividend+divisor-1)/divisor;
}

template<class T>
inline void increment(T& t)
{
  t=static_cast<T>(t+1);
}

template<class T>
inline void decrement(T& t)
{
  t=static_cast<T>(t-1);
}

template<class Number>
inline Number clipped(const Number number,const Number min,const Number max)
{
  if (number<min)
    return min;
  else if (number>max)
    return max;
  else
    return number;
}

template<class Container>
typename Container::value_type& safeAt(Container& container,const typename Container::size_type index)
{
  if (index>=container.size())
    container.resize(index+1);
  return container[index];
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
inline unsigned int count(const Container& container,const Element element)
{
  return std::count(container.begin(),container.end(),element);
}

template<class Sequence,class Subsequence>
inline bool startsWith(const Sequence& sequence,const Subsequence& subsequence)
{
  return sequence.size()>=subsequence.size() && std::equal(subsequence.begin(),subsequence.end(),sequence.begin());
}

template<class Container,class Element>
inline void fill(Container& container,const Element element)
{
  std::fill(std::begin(container),std::end(container),element);
}

template<class Container>
inline void append(Container& target,const Container& extension)
{
  target.insert(target.end(),extension.begin(),extension.end());
}

template<class Container>
inline typename Container::value_type join(const Container& container,const typename Container::value_type& separator)
{
  typename Container::value_type result;
  for (const auto& element:container) {
    if (!result.empty())
      append(result,separator);
    append(result,element);
  }
  return result;
}

template<class Iterator>
inline Iterator unsorted_unique(const Iterator begin,const Iterator end)
{
  typedef typename std::remove_reference<decltype(*begin)>::type value_type;
  typedef std::pair<value_type,size_t> Pair;
  std::vector<Pair> pairs;
  pairs.reserve(distance(begin,end));
  transform(make_move_iterator(begin),make_move_iterator(end),back_inserter(pairs),[&pairs](value_type&& value){return Pair(value,pairs.size());});
  sort(pairs.begin(),pairs.end());
  const auto pairs_end=unique(pairs.begin(),pairs.end(),[](const Pair& lhs,const Pair& rhs){return lhs.first==rhs.first;});
  sort(pairs.begin(),pairs_end,[](const Pair& lhs,const Pair& rhs) {return lhs.second<rhs.second;});
  return transform(make_move_iterator(pairs.begin()),make_move_iterator(pairs_end),begin,[](Pair&& pair) {return pair.first;});
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

inline PieceTypeAndSide toOtherSide(const PieceTypeAndSide pieceTypeAndSide)
{
  if (pieceTypeAndSide==NO_PIECE)
    return NO_PIECE;
  else
    return toPieceTypeAndSide(toPieceType(pieceTypeAndSide),otherSide(toSide(pieceTypeAndSide)));
}

inline bool isValidSquare(const unsigned int file,const unsigned int rank)
{
  return file<NUM_FILES && rank<NUM_RANKS;
}

inline SquareIndex toSquare(const unsigned int file,const unsigned int rank)
{
  return isValidSquare(file,rank) ? static_cast<SquareIndex>(file+rank*NUM_FILES) : NO_SQUARE;
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

inline SquareIndex invert(const SquareIndex square)
{
  assert(square!=NO_SQUARE);
  return SquareIndex(NUM_SQUARES-1-square);
}

inline SquareIndex mirror(const SquareIndex square)
{
  assert(square!=NO_SQUARE);
  return toSquare(NUM_FILES-1-toFile(square),toRank(square));
}

inline bool isTrap(const unsigned int file,const unsigned int rank)
{
  return (file==2 || file==5) && (rank==2 || rank==5);
}

inline bool isTrap(const SquareIndex square)
{
  return isTrap(toFile(square),toRank(square));
}

inline std::vector<SquareIndex> getTrapSquares()
{
  std::vector<SquareIndex> result;
  for (SquareIndex square=FIRST_SQUARE;square<NUM_SQUARES;increment(square))
    if (isTrap(square))
      result.emplace_back(square);
  return result;
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
  return side==FIRST_SIDE ? rank<NUM_SETUP_RANKS : rank>=NUM_RANKS-NUM_SETUP_RANKS;
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

template<class Function>
inline void forEachAdjacentSquare(const SquareIndex square,Function function)
{
  assert(square!=NO_SQUARE);
  const unsigned int file=toFile(square);
  const unsigned int rank=toRank(square);
  if (rank>0 && function(toSquare(file,rank-1)))
    return;
  if (file>0 && function(toSquare(file-1,rank)))
    return;
  if (file<NUM_FILES-1 && function(toSquare(file+1,rank)))
    return;
  if (rank<NUM_RANKS-1 && function(toSquare(file,rank+1)))
    return;
}

inline Squares adjacentSquares(const SquareIndex square)
{
  if (square==NO_SQUARE)
    return Squares();
  Squares result;
  forEachAdjacentSquare(square,[&result](const SquareIndex adjacentSquare) {
    result.emplace_back(adjacentSquare);
    return false;
  });
  return result;
}

inline SquareIndex adjacentTrap(const SquareIndex trapNeighbor)
{
  for (const auto adjacentSquare:adjacentSquares(trapNeighbor))
    if (isTrap(adjacentSquare))
      return adjacentSquare;
  return NO_SQUARE;
}

inline Direction toDirection(const SquareIndex origin,const SquareIndex destination)
{
  switch (destination-origin) {
    case -NUM_FILES: return SOUTH;
    case -1        : return WEST;
    case  1        : return EAST;
    case  NUM_FILES: return NORTH;
    default: throw std::runtime_error("Squares not orthogonally adjacent.");
  }
}

inline SquareIndex toDestination(const SquareIndex origin,const Direction direction,const bool strict=true)
{
  unsigned int file=toFile(origin);
  unsigned int rank=toRank(origin);
  switch (direction) {
    case SOUTH: --rank; break;
    case  WEST: --file; break;
    case  EAST: ++file; break;
    case NORTH: ++rank; break;
    default:
      if (strict)
        throw std::runtime_error("Unrecognized direction.");
      else
        return NO_SQUARE;
    break;
  }
  const auto destination=toSquare(file,rank);
  if (destination==NO_SQUARE && strict)
    throw std::runtime_error("Direction is off the edge of the board.");
  return destination;
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

inline QFont monospace()
{
  QFont font("Monospace");
  font.setStyleHint(QFont::TypeWriter);
  return font;
}

inline void openDialog(QDialog* const dialog)
{
  dialog->setAttribute(Qt::WA_DeleteOnClose);
  dialog->show();
}

inline QNetworkRequest getNetworkRequest(const QUrl& url)
{
  QNetworkRequest networkRequest(url);
  networkRequest.setHeader(QNetworkRequest::ContentTypeHeader,"application/x-www-form-urlencoded");
  return networkRequest;
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

#include <memory>
#ifndef __cpp_lib_make_unique
template<typename T,typename... Args>
std::unique_ptr<T> make_unique(Args&&... args)
{
  return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}
#endif

#endif // DEF_HPP
