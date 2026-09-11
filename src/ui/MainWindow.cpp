#include "ui/MainWindow.h"

#include <QAction>
#include <QBrush>
#include <QColor>
#include <QFont>
#include <QLabel>
#include <QList>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QPen>
#include <QPixmap>
#include <QSplitter>
#include <QStringList>
#include <QToolBar>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>
#include <QWidget>

#include "sample/SampleTree.h"

namespace
{
    /// Whether the database tree shows elements marked as deleted.
    constexpr bool kShowDeletedElementsInDbTree = true;

    /// The foreground color of a cache tree item for a `TrackingState::Added` element.
    constexpr QColor kAddedElementColor(0x0d, 0x47, 0xa1);

    /// The foreground color of a cache tree item for a `TrackingState::Deleted` element.
    const QColor kDeletedElementColor = Qt::gray;

    /// The width and height, in pixels, of each cache tree badge icon.
    constexpr int kIconSize = 12;

    /// @brief A container widget with a tree view.
    struct Pane
    {
        /// The container widget.
        QWidget* widget;

        /// The tree view.
        QTreeWidget* tree;
    };

    /// @brief Creates a titled pane containing a header-less tree view.
    /// @param title The pane's caption.
    /// @return The pane's container widget and its tree view.
    Pane createPane(const QString& title)
    {
        auto* widget = new QWidget;
        auto* layout = new QVBoxLayout(widget);
        layout->setContentsMargins(0, 0, 0, 0);

        auto* label = new QLabel(title);
        label->setContentsMargins(6, 4, 6, 4);

        auto* tree = new QTreeWidget;
        tree->setHeaderHidden(true);

        layout->addWidget(label);
        layout->addWidget(tree);

        return Pane{.widget = widget, .tree = tree};
    }

    /// @brief Draws a small gray plus centered in the icon.
    /// @param painter The painter to draw with, already targeting the icon's surface.
    void drawPlus(QPainter& painter)
    {
        QPen pen(QColor(0x90, 0x90, 0x90));
        pen.setWidth(2);
        pen.setCapStyle(Qt::RoundCap);
        painter.setPen(pen);

        constexpr int center = kIconSize / 2;
        constexpr int margin = 2;
        painter.drawLine(center, margin, center, kIconSize - margin);
        painter.drawLine(margin, center, kIconSize - margin, center);
    }

    /// @brief Creates the icon shown on elements that have children in the database that are not loaded.
    /// @remarks A plain gray plus.
    /// @return The icon.
    QIcon createUnloadedChildrenIcon()
    {
        QPixmap pixmap(kIconSize, kIconSize);
        pixmap.fill(Qt::transparent);

        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing, true);

        drawPlus(painter);

        return QIcon(pixmap);
    }

    /// @brief Creates the icon shown on elements whose children are all loaded.
    /// @remarks A plus on a gray square.
    /// @return The icon.
    QIcon createAllChildrenLoadedIcon()
    {
        QPixmap pixmap(kIconSize, kIconSize);
        pixmap.fill(Qt::transparent);

        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(0xd8, 0xd8, 0xd8));
        painter.drawRect(0, 0, kIconSize, kIconSize);

        drawPlus(painter);

        return QIcon(pixmap);
    }

    /// @brief Creates a transparent icon.
    /// @remarks Placed on items without a badge so their text stays aligned with items that have one.
    /// @return The icon.
    QIcon createBlankIcon()
    {
        QPixmap pixmap(kIconSize, kIconSize);
        pixmap.fill(Qt::transparent);
        return QIcon(pixmap);
    }

    /// @brief Returns the message shown to the user for a failed commit.
    /// @param status The result of the commit.
    /// @return The message text.
    QString commitFailureMessage(const CommitStatus status)
    {
        switch (status)
        {
            case CommitStatus::UnresolvedParent:
                return MainWindow::tr("A new element refers to a parent that is not saved yet.");
            case CommitStatus::UnknownParent:
                return MainWindow::tr("A new element refers to a parent that no longer exists.");
            case CommitStatus::UnknownElement:
                return MainWindow::tr("An element no longer exists in the database.");
            case CommitStatus::ElementDeleted:
                return MainWindow::tr("An element was deleted in the database by someone else.");
            case CommitStatus::Success:
                break;
        }
        return MainWindow::tr("The database rejected the changes.");
    }

    /// @brief Applies the font, color and editability of a cache tree item for the element's tracking state.
    /// @param item The tree item to style.
    /// @param element The cache element it represents.
    void styleCacheItem(QTreeWidgetItem* item, const TreeCacheElement& element)
    {
        Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
        if (element.state != TrackingState::Deleted)
        {
            flags |= Qt::ItemIsEditable;
        }
        item->setFlags(flags);

        QFont font = item->font(0);
        font.setItalic(element.state == TrackingState::Modified);
        font.setStrikeOut(element.state == TrackingState::Deleted);
        item->setFont(0, font);

        if (element.state == TrackingState::Added)
        {
            item->setForeground(0, QBrush(kAddedElementColor));
        } else if (element.state == TrackingState::Deleted)
        {
            item->setForeground(0, QBrush(kDeletedElementColor));
        } else
        {
            item->setForeground(0, QBrush());
        }
    }

    /// @brief Returns the element ID stored on a tree item.
    Id idOf(const QTreeWidgetItem* item)
    {
        return item->data(0, Qt::UserRole).toLongLong();
    }

    /// @brief Stores an element ID on a tree item.
    void setItemId(QTreeWidgetItem* item, const Id id)
    {
        item->setData(0, Qt::UserRole, id);
    }
} // namespace

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), m_db(buildSampleTree())
{
    setWindowTitle(tr("TreeCacheDemo"));
    resize(900, 500);

    auto* toolbar = addToolBar(tr("Actions"));
    toolbar->setMovable(false);
    m_resetAction = toolbar->addAction(tr("Reset"));
    toolbar->addSeparator();
    m_loadAction = toolbar->addAction(tr("Load into cache"));
    toolbar->addSeparator();
    m_addChildAction = toolbar->addAction(tr("Add child"));
    m_deleteAction = toolbar->addAction(tr("Delete"));
    m_saveAction = toolbar->addAction(tr("Save to DB"));

    const Pane dbPane = createPane(tr("Database"));
    const Pane cachePane = createPane(tr("Cache"));
    m_dbTree = dbPane.tree;
    m_cacheTree = cachePane.tree;

    auto* splitter = new QSplitter(this);
    splitter->addWidget(dbPane.widget);
    splitter->addWidget(cachePane.widget);
    splitter->setSizes({450, 450});
    setCentralWidget(splitter);

    m_unloadedChildrenIcon = createUnloadedChildrenIcon();
    m_allChildrenLoadedIcon = createAllChildrenLoadedIcon();
    m_blankIcon = createBlankIcon();

    connect(m_loadAction, &QAction::triggered, this, &MainWindow::loadIntoCache);
    connect(m_addChildAction, &QAction::triggered, this, &MainWindow::addChild);
    connect(m_deleteAction, &QAction::triggered, this, &MainWindow::deleteSelected);
    connect(m_saveAction, &QAction::triggered, this, &MainWindow::saveToDatabase);
    connect(m_resetAction, &QAction::triggered, this, &MainWindow::resetAll);

    connect(m_dbTree, &QTreeWidget::itemSelectionChanged, this, &MainWindow::onDbSelectionChanged);
    connect(m_cacheTree, &QTreeWidget::itemSelectionChanged, this, &MainWindow::onCacheSelectionChanged);
    connect(m_cacheTree, &QTreeWidget::itemChanged, this, &MainWindow::onCacheItemChanged);

    m_dbTree->setContextMenuPolicy(Qt::CustomContextMenu);
    m_cacheTree->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_dbTree, &QWidget::customContextMenuRequested, this, &MainWindow::showDbContextMenu);
    connect(m_cacheTree, &QWidget::customContextMenuRequested, this, &MainWindow::showCacheContextMenu);

    rebuildDbTree();
    rebuildCacheTree();
    updateActionStates();
}

void MainWindow::loadIntoCache()
{
    const Id id = selectedId(m_dbTree);
    if (id == UndefinedId) return;

    if (!m_cache.contains(id))
    {
        if (const auto element = m_db.get(id))
        {
            m_cache.load(*element);
        }
    }

    rebuildCacheTree();
    selectInCacheTree(id);
    updateActionStates();
}

void MainWindow::addChild()
{
    const Id parentId = selectedId(m_cacheTree);
    if (parentId == UndefinedId) return;

    const auto child = m_cache.addChild(parentId);
    if (!child) return;

    rebuildCacheTree();
    selectInCacheTree(*child, true);
    updateActionStates();
}

void MainWindow::deleteSelected()
{
    const Id id = selectedId(m_cacheTree);
    if (id == UndefinedId) return;

    const auto element = m_cache.element(id);
    if (!element || element->state == TrackingState::Deleted) return;

    if (hasUnloadedChildrenInSubtree(id))
    {
        const auto answer = QMessageBox::warning(
            this, tr("Delete element"),
            tr("\"%1\" has children in the database that aren't loaded here.\nThey will be deleted too.")
            .arg(element->text),
            QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Cancel);

        if (answer != QMessageBox::Ok) return;
    }

    m_cache.remove(id);
    rebuildCacheTree();
    updateActionStates();
}

void MainWindow::saveToDatabase()
{
    if (!m_cache.hasPendingChanges()) return;

    const ChangeSet changes = m_cache.buildChangeSet();
    if (const CommitResult result = m_db.commit(changes);
        result.isSuccess())
    {
        m_cache.acceptCommit(result);
    } else
    {
        QMessageBox::warning(this, tr("Save failed"), commitFailureMessage(result.status));
    }

    rebuildDbTree();
    rebuildCacheTree();
    updateActionStates();
}

void MainWindow::resetAll()
{
    if (m_cache.hasPendingChanges())
    {
        const auto answer = QMessageBox::question(
            this, tr("Reset"),
            tr("Discard all pending changes and reset to the initial state?"),
            QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Cancel);

        if (answer != QMessageBox::Ok) return;
    }

    m_db.reset();
    m_cache.clear();

    rebuildDbTree();
    rebuildCacheTree();
    updateActionStates();
}

void MainWindow::onCacheItemChanged(const QTreeWidgetItem* item, const int column)
{
    if (m_rebuilding || column != 0 || item == nullptr) return;

    const Id id = idOf(item);
    if (!m_cache.contains(id)) return;

    m_cache.editText(id, item->text(0));

    rebuildCacheTree();
    selectInCacheTree(id);
    updateActionStates();
}

void MainWindow::onDbSelectionChanged() const
{
    if (m_rebuilding) return;

    if (!m_dbTree->selectedItems().isEmpty())
    {
        m_cacheTree->clearSelection();
    }

    updateActionStates();
}

void MainWindow::onCacheSelectionChanged() const
{
    if (m_rebuilding) return;

    if (!m_cacheTree->selectedItems().isEmpty())
    {
        m_dbTree->clearSelection();
    }

    updateActionStates();
}

void MainWindow::showDbContextMenu(const QPoint& pos)
{
    if (QTreeWidgetItem* item = m_dbTree->itemAt(pos))
    {
        m_dbTree->setCurrentItem(item);
    }

    updateActionStates();

    QMenu menu(this);
    menu.addAction(m_loadAction);
    menu.exec(m_dbTree->viewport()->mapToGlobal(pos));
}

void MainWindow::showCacheContextMenu(const QPoint& pos)
{
    if (QTreeWidgetItem* item = m_cacheTree->itemAt(pos))
    {
        m_cacheTree->setCurrentItem(item);
    }

    updateActionStates();

    QMenu menu(this);
    menu.addAction(m_addChildAction);
    menu.addAction(m_deleteAction);
    menu.exec(m_cacheTree->viewport()->mapToGlobal(pos));
}

void MainWindow::updateActionStates() const
{
    const Id dbId = selectedId(m_dbTree);
    m_loadAction->setEnabled(dbId != UndefinedId);

    const Id cacheId = selectedId(m_cacheTree);
    const auto cacheElement = m_cache.element(cacheId);
    const bool editableCacheElement = cacheElement && cacheElement->state != TrackingState::Deleted;
    m_addChildAction->setEnabled(editableCacheElement);
    m_deleteAction->setEnabled(editableCacheElement);

    m_saveAction->setEnabled(m_cache.hasPendingChanges());
}

void MainWindow::rebuildDbTree()
{
    m_rebuilding = true;

    const Id keep = selectedId(m_dbTree);
    m_dbTree->clear();

    QHash<Id, QTreeWidgetItem*> items;
    for (const TreeDbElement& element: m_db.getAll(kShowDeletedElementsInDbTree))
    {
        auto* item = new QTreeWidgetItem(QStringList{element.text});
        setItemId(item, element.id);

        if (element.isDeleted)
        {
            QFont font = item->font(0);
            font.setStrikeOut(true);
            item->setFont(0, font);
            item->setForeground(0, QBrush(Qt::gray));
            item->setFlags(item->flags() & ~Qt::ItemIsSelectable);
        }

        if (element.parentId != RootId && items.contains(element.parentId))
        {
            items.value(element.parentId)->addChild(item);
        } else
        {
            m_dbTree->addTopLevelItem(item);
        }

        items.insert(element.id, item);
    }

    m_dbTree->expandAll();
    if (keep != UndefinedId && items.contains(keep))
    {
        m_dbTree->setCurrentItem(items.value(keep));
    }

    m_rebuilding = false;
}

void MainWindow::rebuildCacheTree()
{
    m_rebuilding = true;
    m_cacheTree->clear();
    m_cacheItems.clear();

    for (const Id rootId: m_cache.roots())
    {
        appendCacheSubtree(rootId, nullptr);
    }

    m_cacheTree->expandAll();
    m_rebuilding = false;
}

void MainWindow::appendCacheSubtree(const Id id, QTreeWidgetItem* parentItem)
{
    const auto found = m_cache.element(id);
    Q_ASSERT(found);
    const TreeCacheElement& element = *found;

    auto* item = new QTreeWidgetItem(QStringList{element.text});
    setItemId(item, id);
    styleCacheItem(item, element);

    if (element.hasChildren)
    {
        const bool allLoaded = allDirectChildrenLoaded(id);
        item->setIcon(0, allLoaded ? m_allChildrenLoadedIcon : m_unloadedChildrenIcon);
        item->setToolTip(0, allLoaded
                                ? tr("All children are loaded")
                                : tr("Has children in the database that aren't loaded here"));
    } else
    {
        item->setIcon(0, m_blankIcon);
    }

    if (parentItem != nullptr)
    {
        parentItem->addChild(item);
    } else
    {
        m_cacheTree->addTopLevelItem(item);
    }

    m_cacheItems.insert(id, item);

    for (const Id childId: m_cache.childrenOf(id))
    {
        appendCacheSubtree(childId, item);
    }
}

bool MainWindow::allDirectChildrenLoaded(const Id id) const
{
    if (!isPersistent(id)) return true;

    const auto dbElement = m_db.get(id);
    if (!dbElement) return true;

    int loadedChildCount = 0;
    for (const Id childId: m_cache.childrenOf(id))
    {
        if (const auto child = m_cache.element(childId);
            child && child->state != TrackingState::Added)
        {
            ++loadedChildCount;
        }
    }

    return dbElement->childCount <= loadedChildCount;
}

bool MainWindow::hasUnloadedChildrenInSubtree(const Id id) const
{
    if (!allDirectChildrenLoaded(id)) return true;

    for (const Id childId: m_cache.childrenOf(id))
    {
        if (hasUnloadedChildrenInSubtree(childId)) return true;
    }

    return false;
}

Id MainWindow::selectedId(const QTreeWidget* tree)
{
    const QList<QTreeWidgetItem*> selected = tree->selectedItems();
    if (selected.isEmpty()) return UndefinedId;
    return idOf(selected.first());
}

void MainWindow::selectInCacheTree(const Id id, const bool startEditing) const
{
    const auto found = m_cacheItems.constFind(id);
    if (found == m_cacheItems.constEnd()) return;

    QTreeWidgetItem* const item = found.value();

    m_cacheTree->setCurrentItem(item);
    m_cacheTree->scrollToItem(item);

    if (startEditing)
    {
        m_cacheTree->editItem(item, 0);
    }
}
