#ifndef TREEMODEL_HPP
#define TREEMODEL_HPP

#include <unordered_map>
#include <QAbstractItemModel>
class QModelIndex;
class QVariant;
#include "def.hpp"

class TreeModel : public QAbstractItemModel {
  Q_OBJECT
public:
  TreeModel(NodePtr root,QObject* const parent=nullptr);

  std::vector<QPersistentModelIndex> indices(const NodePtr& node) const;
  QPersistentModelIndex index(const NodePtr& node,const int column) const;
  QPersistentModelIndex lastIndex(const NodePtr& node) const;
  NodePtr getItem(const QModelIndex& index) const;
  QModelIndex createIndex(const int row,const int column,const NodePtr& node) const;
  int columnCount(const int numMoves) const;

  virtual bool hasChildren(const QModelIndex& parent=QModelIndex()) const override;
  virtual int rowCount(const QModelIndex& parent=QModelIndex()) const override;
  virtual int columnCount(const QModelIndex& index=QModelIndex()) const override;
  virtual QModelIndex index(const int row,const int column,const QModelIndex& parent=QModelIndex()) const override;
  virtual QModelIndex parent(const QModelIndex& index) const override;
  virtual QVariant data(const QModelIndex& index,const int role) const override;
  virtual Qt::ItemFlags flags(const QModelIndex& index) const override;

  NodePtr root;
  QPersistentModelIndex exclusive;
private:
  mutable std::unordered_map<Node*,std::weak_ptr<Node> > rawToWeak;
  mutable std::unordered_map<Node*,std::vector<QPersistentModelIndex> > rawToIndices;
};

#endif // TREEMODEL_HPP
