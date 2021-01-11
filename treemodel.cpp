#include "treemodel.hpp"
#include "node.hpp"
#include "io.hpp"

TreeModel::TreeModel(NodePtr root_,QObject* const parent) :
  QAbstractItemModel(parent),
  root(std::move(root_))
{
}

void TreeModel::reset() const
{
  rawToIndices.clear();
}

std::vector<QPersistentModelIndex> TreeModel::indices(const NodePtr& node) const
{
  if (node==nullptr)
    return std::vector<QPersistentModelIndex>(1);
  const auto& pair=rawToIndices.find(node.get());
  if (pair==rawToIndices.end()) {
    const auto childIndex=node->childIndex();
    const auto numColumns=columnCount(node->move.size());
    std::vector<QPersistentModelIndex> indices;
    indices.reserve(numColumns);
    for (int column=0;column<numColumns;++column)
      indices.emplace_back(createIndex(childIndex,column,node));
    return indices;
  }
  else
    return pair->second;
}

QPersistentModelIndex TreeModel::index(const NodePtr& node,const int column) const
{
  if (node==nullptr)
    return QPersistentModelIndex();
  const auto& pair=rawToIndices.find(node.get());
  if (pair!=rawToIndices.end() && column<int(pair->second.size())) {
    const auto result=pair->second[column];
    if (result.isValid())
      return result;
  }
  return createIndex(node->childIndex(),column,node);
}

QPersistentModelIndex TreeModel::lastIndex(const NodePtr& node) const
{
  return index(node,columnCount(node->move.size())-1);
}

NodePtr TreeModel::getItem(const QModelIndex& index) const
{
  if (index.isValid())
    if (const auto item=rawToWeak[static_cast<Node*>(index.internalPointer())].lock())
      return item;
  return root;
}

inline QModelIndex TreeModel::createIndex(const int row,const int column,const NodePtr& node) const
{
  rawToWeak.emplace(node.get(),std::weak_ptr<Node>(node));
  const auto index=QAbstractItemModel::createIndex(row,column,node.get());
  safeAt(rawToIndices[node.get()],column)=index;
  return index;
}

int TreeModel::columnCount(const int numMoves) const
{
  if (numMoves==0)
    return numStartingPieces;
  else if (numMoves<MAX_STEPS_PER_MOVE)
    return numMoves+2;
  else
    return numMoves+1;
}

bool TreeModel::hasChildren(const QModelIndex& parent) const
{
  const auto parentItem=getItem(parent);
  return parentItem!=nullptr && parentItem->hasChild();
}

int TreeModel::rowCount(const QModelIndex& parent) const
{
  const auto parentItem=getItem(parent);
  return parentItem==nullptr ? 0 : parentItem->numChildren();
}

int TreeModel::columnCount(const QModelIndex& index) const
{
  const auto item=getItem(index);
  if (item==nullptr || !item->hasChild())
    return 0;
  else if (item->inSetup())
    return numStartingPieces;
  else
    return columnCount(index.parent().isValid() ? item->maxChildSteps() : item->maxDescendantSteps());
}

QModelIndex TreeModel::index(const int row,const int column,const QModelIndex& parent) const
{
  if (!parent.isValid() || parent.column()==0) {
    const auto parentItem=getItem(parent);
    if (parentItem!=nullptr)
      if (const auto child=parentItem->child(row))
        if (column<columnCount(child->move.size()))
          return createIndex(row,column,child);
  }
  return QModelIndex();
}

QModelIndex TreeModel::parent(const QModelIndex& index) const
{
  if (index.isValid()) {
    const auto item=getItem(index);
    if (item!=nullptr) {
      const auto parent=item->previousNode;
      if (parent!=nullptr)
        return createIndex(parent->childIndex(),0,parent);
    }
  }
  return QModelIndex();
}

QVariant TreeModel::data(const QModelIndex& index,const int role) const
{
  if (index.isValid()) {
    const auto& node=getItem(index);
    if (node==nullptr)
      return QVariant();
    if (role==Qt::DisplayRole) {
      const unsigned int column=index.column();
      if (column==0) {
        const auto moveNumber=QString::fromStdString(node->toPlyString(*root));
        const auto numChildren=rowCount(index);
        switch (numChildren) {
          case 0: return moveNumber;
          case 1: return moveNumber+" *";
          default: return moveNumber+" ("+QString::number(numChildren)+')';
        }
      }
      const auto& move=node->move;
      const unsigned int moveIndex=column-1;
      if (move.empty()) {
        const auto& placements=node->gameState.playedPlacements();
        assert(placements.size()==numStartingPieces);
        const auto begin=next(placements.cbegin(),moveIndex);
        return QString::fromStdString(toString(begin,next(begin)));
      }
      else {
        assert(moveIndex<MAX_STEPS_PER_MOVE);
        QString step;
        if (moveIndex<move.size()) {
          step=QString::fromStdString(toString(move[moveIndex]));
          if (moveIndex<MAX_STEPS_PER_MOVE-1)
            return step;
        }
        else if (moveIndex==move.size())
          step="pass";

        const auto result=node->result;
        if (node->result.endCondition!=NO_END)
          step+=QString(' ')+(result.winner==node->gameState.sideToMove ? '-' : '+')+toupper(toChar(result.endCondition));
        return step;
      }
    }
    else if (role==Qt::BackgroundRole)
      return node->cumulativeChildIndex()%2==0 ? QPalette().base() : QPalette().alternateBase();
  }
  return QVariant();
}

Qt::ItemFlags TreeModel::flags(const QModelIndex& index) const
{
  if (!index.isValid() || (exclusive.isValid() && exclusive!=index))
    return Qt::NoItemFlags;

  return QAbstractItemModel::flags(index);
}
