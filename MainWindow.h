#pragma once

#include <QMainWindow>
#include <QTabWidget>
#include <QTreeWidget>
#include <QTextEdit>
#include <QSplitter>
#include <QCloseEvent>

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    QTabWidget *tabWidget;
    QTextEdit *notesEditor;

    QTreeWidget* createTodoTree();
    void setupUI();

    void saveToFile();
    void loadFromFile();

private slots:
    void addTab();
    void removeCurrentTab();
    void renameCurrentTab();

    void showTabContextMenu(const QPoint &pos);
    void showItemContextMenu(const QPoint &pos);

    void addTask();
    void addSubTask();
    void deleteTask();
    void setColor();

    void onItemSelectionChanged();
    void saveNotes();
};