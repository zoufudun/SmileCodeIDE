#ifndef ROOMMANAGERDIALOG_H
#define ROOMMANAGERDIALOG_H

#include <QColor>
#include <QDialog>
#include <QList>
#include <QStringList>

#include "devicemonitorpanel.h"

class QTableWidget;
class QPushButton;

class RoomManagerDialog : public QDialog {
  Q_OBJECT
public:
  explicit RoomManagerDialog(const QList<RoomRegion> &rooms,
                             const QStringList &viewNames,
                             QWidget *parent = nullptr);

  QList<RoomRegion> rooms() const;

private slots:
  void onAddRoom();
  void onDeleteRoomRow();
  void onSelectColor(int row);
  void onToggleSelectAll(bool select);

private:
  void populateTable();

  QList<RoomRegion> m_rooms;
  QStringList m_viewNames;
  QTableWidget *m_table;
  QPushButton *m_btnAdd;
  QPushButton *m_btnSelectAll;
  QPushButton *m_btnUnselectAll;
};

#endif // ROOMMANAGERDIALOG_H
