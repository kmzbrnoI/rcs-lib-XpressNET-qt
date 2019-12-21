#ifndef QTREENUMWIDGETITEM_H
#define QTREENUMWIDGETITEM_H

/* Custom FirstNumTreeWidgetItem, which cotains number is first column and
 * is sorted by this number.
 */

#include <QTreeWidgetItem>

class FirstNumTreeWidgetItem : public QTreeWidgetItem {
  public:
  FirstNumTreeWidgetItem(QTreeWidget* parent): QTreeWidgetItem(parent) {}
  private:
  bool operator<(const QTreeWidgetItem &other) const {
     int column = treeWidget()->sortColumn();
     if (column == 0)
         return text(column).toInt() < other.text(column).toInt();
     return text(column) < other.text(column);
  }
};

#endif // QTREENUMWIDGETITEM_H
