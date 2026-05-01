#include "MainWindow.h"
#include "TodoItem.h"

#include <QMenu>
#include <QInputDialog>
#include <QColorDialog>
#include <QVBoxLayout>
#include <QDateTime>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

// ---------------- JSON Helpers ----------------

QJsonObject todoToJson(const TodoItem &t)
{
    QJsonObject obj;
    obj["title"] = t.title;
    obj["created"] = t.created.toString(Qt::ISODate);
    obj["completed"] = t.completed.toString(Qt::ISODate);
    obj["notes"] = t.notes;
    obj["done"] = t.done;
    obj["color"] = t.color.name();
    return obj;
}

TodoItem jsonToTodo(const QJsonObject &obj)
{
    TodoItem t;
    t.title = obj["title"].toString();
    t.created = QDateTime::fromString(obj["created"].toString(), Qt::ISODate);
    t.completed = QDateTime::fromString(obj["completed"].toString(), Qt::ISODate);
    t.notes = obj["notes"].toString();
    t.done = obj["done"].toBool();
    t.color = QColor(obj["color"].toString());
    return t;
}

QJsonObject itemToJson(QTreeWidgetItem *item)
{
    QJsonObject obj;

    TodoItem data = item->data(0, Qt::UserRole).value<TodoItem>();
    obj["data"] = todoToJson(data);

    QJsonArray children;
    for (int i = 0; i < item->childCount(); ++i)
        children.append(itemToJson(item->child(i)));

    obj["children"] = children;
    return obj;
}

QTreeWidgetItem* jsonToItem(const QJsonObject &obj)
{
    TodoItem data = jsonToTodo(obj["data"].toObject());

    QTreeWidgetItem *item = new QTreeWidgetItem;
    item->setText(0, data.title);
    item->setText(1, data.created.toString());
    item->setText(2, data.completed.toString());
    item->setData(0, Qt::UserRole, QVariant::fromValue(data));

    item->setCheckState(0, data.done ? Qt::Checked : Qt::Unchecked);

    if (data.color.isValid())
        item->setBackground(0, data.color);

    QFont f = item->font(0);
    f.setStrikeOut(data.done);
    item->setFont(0, f);

    QJsonArray children = obj["children"].toArray();
    for (auto childVal : children)
        item->addChild(jsonToItem(childVal.toObject()));

    return item;
}

// ---------------- MainWindow ----------------

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUI();
    loadFromFile();

    if (tabWidget->count() == 0)
        addTab();
}

void MainWindow::setupUI()
{
    tabWidget = new QTabWidget;
    tabWidget->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(tabWidget, &QTabWidget::customContextMenuRequested,
            this, &MainWindow::showTabContextMenu);

    notesEditor = new QTextEdit;
    connect(notesEditor, &QTextEdit::textChanged,
            this, &MainWindow::saveNotes);

    QSplitter *splitter = new QSplitter;
    splitter->addWidget(tabWidget);
    splitter->addWidget(notesEditor);
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 1);

    setCentralWidget(splitter);
}

QTreeWidget* MainWindow::createTodoTree()
{
    QTreeWidget *tree = new QTreeWidget;
    tree->setHeaderLabels({"Task", "Created", "Completed"});

    tree->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(tree, &QTreeWidget::customContextMenuRequested,
            this, &MainWindow::showItemContextMenu);

    connect(tree, &QTreeWidget::itemSelectionChanged,
            this, &MainWindow::onItemSelectionChanged);

    // Checkbox + completion logic
    connect(tree, &QTreeWidget::itemChanged, this, [=](QTreeWidgetItem *item){
        TodoItem data = item->data(0, Qt::UserRole).value<TodoItem>();

        bool done = item->checkState(0) == Qt::Checked;
        data.done = done;

        if (done)
            data.completed = QDateTime::currentDateTime();

        item->setText(2, data.completed.toString());

        QFont f = item->font(0);
        f.setStrikeOut(done);
        item->setFont(0, f);

        item->setData(0, Qt::UserRole, QVariant::fromValue(data));
        saveToFile();
    });

    // Drag & drop
    tree->setDragEnabled(true);
    tree->setAcceptDrops(true);
    tree->setDropIndicatorShown(true);
    tree->setDragDropMode(QAbstractItemView::InternalMove);

    return tree;
}

// ---------------- Tabs ----------------

void MainWindow::addTab()
{
    QString name = QInputDialog::getText(this, "New List", "List name:");
    if (name.isEmpty()) return;

    tabWidget->addTab(createTodoTree(), name);
    saveToFile();
}

void MainWindow::removeCurrentTab()
{
    int index = tabWidget->currentIndex();
    if (index >= 0)
        tabWidget->removeTab(index);

    saveToFile();
}

void MainWindow::renameCurrentTab()
{
    int index = tabWidget->currentIndex();
    if (index < 0) return;

    QString name = QInputDialog::getText(this, "Rename List", "New name:");
    if (!name.isEmpty())
        tabWidget->setTabText(index, name);

    saveToFile();
}

void MainWindow::showTabContextMenu(const QPoint &pos)
{
    QMenu menu;
    menu.addAction("Add Tab", this, &MainWindow::addTab);
    menu.addAction("Rename Tab", this, &MainWindow::renameCurrentTab);
    menu.addAction("Delete Tab", this, &MainWindow::removeCurrentTab);
    menu.exec(tabWidget->mapToGlobal(pos));
}

// ---------------- Tasks ----------------

void MainWindow::addTask()
{
    auto tree = static_cast<QTreeWidget*>(tabWidget->currentWidget());
    if (!tree) return;

    QString text = QInputDialog::getText(this, "New Task", "Task:");
    if (text.isEmpty()) return;

    QTreeWidgetItem *item = new QTreeWidgetItem(tree);
    item->setText(0, text);
    item->setCheckState(0, Qt::Unchecked);

    TodoItem data;
    data.title = text;
    data.created = QDateTime::currentDateTime();

    item->setText(1, data.created.toString());
    item->setData(0, Qt::UserRole, QVariant::fromValue(data));

    tree->setCurrentItem(item);
    saveToFile();
}

void MainWindow::addSubTask()
{
    auto tree = static_cast<QTreeWidget*>(tabWidget->currentWidget());
    auto parent = tree->currentItem();
    if (!parent) return;

    QString text = QInputDialog::getText(this, "New Subtask", "Task:");
    if (text.isEmpty()) return;

    QTreeWidgetItem *child = new QTreeWidgetItem(parent);
    child->setText(0, text);
    child->setCheckState(0, Qt::Unchecked);

    TodoItem data;
    data.title = text;
    data.created = QDateTime::currentDateTime();

    child->setText(1, data.created.toString());
    child->setData(0, Qt::UserRole, QVariant::fromValue(data));

    parent->setExpanded(true);
    tree->setCurrentItem(child);

    saveToFile();
}

void MainWindow::deleteTask()
{
    auto tree = static_cast<QTreeWidget*>(tabWidget->currentWidget());
    auto item = tree->currentItem();
    if (!item) return;

    delete item;
    saveToFile();
}

void MainWindow::setColor()
{
    auto tree = static_cast<QTreeWidget*>(tabWidget->currentWidget());
    auto item = tree->currentItem();
    if (!item) return;

    QColor color = QColorDialog::getColor();
    if (!color.isValid()) return;

    item->setBackground(0, color);

    TodoItem data = item->data(0, Qt::UserRole).value<TodoItem>();
    data.color = color;
    item->setData(0, Qt::UserRole, QVariant::fromValue(data));

    saveToFile();
}

void MainWindow::showItemContextMenu(const QPoint &pos)
{
    auto tree = static_cast<QTreeWidget*>(tabWidget->currentWidget());
    auto item = tree->itemAt(pos);

    QMenu menu;
    menu.addAction("Add Task", this, &MainWindow::addTask);

    if (item) {
        menu.addAction("Add Subtask", this, &MainWindow::addSubTask);
        menu.addAction("Delete", this, &MainWindow::deleteTask);
        menu.addAction("Set Color", this, &MainWindow::setColor);
    }

    menu.exec(tree->mapToGlobal(pos));
}

// ---------------- Notes ----------------

void MainWindow::onItemSelectionChanged()
{
    auto tree = static_cast<QTreeWidget*>(tabWidget->currentWidget());
    auto item = tree->currentItem();
    if (!item) {
        notesEditor->clear();
        return;
    }

    TodoItem data = item->data(0, Qt::UserRole).value<TodoItem>();
    notesEditor->blockSignals(true);
    notesEditor->setPlainText(data.notes);
    notesEditor->blockSignals(false);
}

void MainWindow::saveNotes()
{
    auto tree = static_cast<QTreeWidget*>(tabWidget->currentWidget());
    auto item = tree->currentItem();
    if (!item) return;

    TodoItem data = item->data(0, Qt::UserRole).value<TodoItem>();
    data.notes = notesEditor->toPlainText();
    item->setData(0, Qt::UserRole, QVariant::fromValue(data));

    saveToFile();
}

// ---------------- Save / Load ----------------

void MainWindow::saveToFile()
{
    QJsonArray tabs;

    for (int i = 0; i < tabWidget->count(); ++i) {
        QTreeWidget *tree = static_cast<QTreeWidget*>(tabWidget->widget(i));

        QJsonObject tabObj;
        tabObj["name"] = tabWidget->tabText(i);

        QJsonArray items;
        for (int j = 0; j < tree->topLevelItemCount(); ++j)
            items.append(itemToJson(tree->topLevelItem(j)));

        tabObj["items"] = items;
        tabs.append(tabObj);
    }

    QFile file("todos.json");
    if (file.open(QIODevice::WriteOnly))
        file.write(QJsonDocument(tabs).toJson());
}

void MainWindow::loadFromFile()
{
    QFile file("todos.json");
    if (!file.open(QIODevice::ReadOnly)) return;

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    QJsonArray tabs = doc.array();

    tabWidget->clear();

    for (auto tabVal : tabs) {
        QJsonObject tabObj = tabVal.toObject();

        QTreeWidget *tree = createTodoTree();
        tabWidget->addTab(tree, tabObj["name"].toString());

        QJsonArray items = tabObj["items"].toArray();
        for (auto itemVal : items)
            tree->addTopLevelItem(jsonToItem(itemVal.toObject()));
    }
}

// ---------------- Close ----------------

void MainWindow::closeEvent(QCloseEvent *event)
{
    saveToFile();
    QMainWindow::closeEvent(event);
}