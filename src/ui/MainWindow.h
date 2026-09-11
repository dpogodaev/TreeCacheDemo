#pragma once

#include <QHash>
#include <QIcon>
#include <QMainWindow>

#include "cache/Cache.h"
#include "db/Database.h"
#include "domain/Id.h"

class QAction;
class QPoint;
class QTreeWidget;
class QTreeWidgetItem;

/// @brief The application window: two tree views and a toolbar managing a @ref Database and a @ref Cache.
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    /// @brief Builds the window and renders the initial state.
    /// @param parent The parent widget, or `nullptr`.
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    /// @brief Loads the element selected in the database tree into the cache.
    void loadIntoCache();

    /// @brief Adds a new local child under the element selected in the cache tree and opens the inline editor.
    void addChild();

    /// @brief Deletes the element selected in the cache tree, warning first if it has unloaded children in the database.
    void deleteSelected();

    /// @brief Commits the pending changes and refreshes both trees, warning if the commit fails.
    void saveToDatabase();

    /// @brief Resets the database, clears the cache and refreshes both trees, confirming first if changes are pending.
    void resetAll();

    /// @brief Handles an inline text edit finished in the cache tree.
    /// @param item The edited item.
    /// @param column The edited column.
    void onCacheItemChanged(const QTreeWidgetItem* item, int column);

    /// @brief Clears the cache tree selection when an element is selected in the database tree.
    void onDbSelectionChanged() const;

    /// @brief Clears the database tree selection when an element is selected in the cache tree.
    void onCacheSelectionChanged() const;

    /// @brief Shows the context menu for the database tree, selecting the element under the cursor first, if any.
    /// @param pos The request position, in viewport coordinates.
    void showDbContextMenu(const QPoint& pos);

    /// @brief Shows the context menu for the cache tree, selecting the element under the cursor first, if any.
    /// @param pos The request position, in viewport coordinates.
    void showCacheContextMenu(const QPoint& pos);

private:
    /// @brief Enables or disables actions based on the current selection and pending changes.
    void updateActionStates() const;

    /// @brief Rebuilds the database tree.
    void rebuildDbTree();

    /// @brief Rebuilds the cache tree.
    void rebuildCacheTree();

    /// @brief Appends a cache element and, recursively, its subtree to the cache tree.
    /// @pre The element with the specified ID is in the cache.
    /// @param id The identifier of the subtree root.
    /// @param parentItem The item to append under, or `nullptr` for a top-level item.
    void appendCacheSubtree(Id id, QTreeWidgetItem* parentItem);

    /// @brief Returns whether every direct child of the element in the database is loaded into the cache.
    /// @param id The identifier of the element.
    /// @return `true` if the element is not persisted, or all of its direct database children are loaded; otherwise, `false`.
    bool allDirectChildrenLoaded(Id id) const;

    /// @brief Returns whether the element's loaded subtree has a child in the database that is not loaded.
    /// @param id The identifier of the subtree root.
    /// @return `true` if some element in the subtree has more children in the database than are loaded; otherwise, `false`.
    bool hasUnloadedChildrenInSubtree(Id id) const;

    /// @brief Returns the id stored on the selected item of a tree.
    /// @param tree The tree to read the selection from.
    /// @return The selected element's ID, or @ref UndefinedId if nothing is selected.
    static Id selectedId(const QTreeWidget* tree);

    /// @brief Selects an element in the cache tree, optionally opening the inline editor.
    /// @param id The identifier of the element to select.
    /// @param startEditing Whether to open the inline editor.
    void selectInCacheTree(Id id, bool startEditing = false) const;

    /// The simulated database.
    Database m_db;

    /// The local change-tracking cache.
    Cache m_cache;

    /// The database tree view.
    QTreeWidget* m_dbTree = nullptr;

    /// The cache tree view.
    QTreeWidget* m_cacheTree = nullptr;

    /// The action that loads the selected database element into the cache.
    QAction* m_loadAction = nullptr;

    /// The action that adds a local child under the selected cache element.
    QAction* m_addChildAction = nullptr;

    /// The action that deletes the selected cache element.
    QAction* m_deleteAction = nullptr;

    /// The action that commits the pending changes.
    QAction* m_saveAction = nullptr;

    /// The action that resets the database and clears the cache.
    QAction* m_resetAction = nullptr;

    /// Maps an element ID to its item in the cache tree.
    QHash<Id, QTreeWidgetItem*> m_cacheItems;

    /// The plus badge shown on cache elements that have children in the database that are not loaded.
    QIcon m_unloadedChildrenIcon;

    /// The plus-on-square badge shown on cache elements whose children are all loaded.
    QIcon m_allChildrenLoadedIcon;

    /// Transparent placeholder kept on items without a badge so their text stays aligned.
    QIcon m_blankIcon;

    /// Indicates that the tree is being rebuilt programmatically.
    /// Signals emitted during the rebuild must not be interpreted as user actions.
    bool m_rebuilding = false;
};
