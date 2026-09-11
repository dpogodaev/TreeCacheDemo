#pragma once

#include <optional>
#include <vector>

#include <QHash>
#include <QVector>

#include "db/ChangeSet.h"
#include "db/CommitResult.h"
#include "db/TreeDbElement.h"
#include "domain/Id.h"
#include "domain/TreeElement.h"

/// @brief A simulated in-memory database containing a tree of elements.
class Database
{
public:
    /// @brief Constructs the database from the specified elements.
    /// @param initialElements The elements of the initial tree, parents before children.
    explicit Database(std::vector<TreeDbElement> initialElements);

    /// @brief Restores the initial tree.
    void reset();

    /// @brief Returns an element of the tree by ID.
    /// @param id The ID of the element to retrieve.
    /// @return The tree element, or `std::nullopt` if not found.
    std::optional<TreeElement> get(Id id) const;

    /// @brief Applies a change set to the database.
    /// @remarks
    /// All or nothing: if any item is invalid, nothing is applied and the database stays unchanged.<br/>
    /// Deleting an element also deletes its whole subtree, including elements that were never loaded.
    /// @param changes The pending changes to apply.
    /// @return The outcome of the commit, and on success the IDs assigned to the inserted elements.
    CommitResult commit(const ChangeSet& changes);

    /// @brief Returns all elements of the tree.
    /// @param includeDeleted  Whether elements marked as deleted are included.
    /// @return The elements, parents before children.
    std::vector<TreeDbElement> getAll(bool includeDeleted = true) const;

private:
    /// @brief The state of the database.
    struct Storage
    {
        /// The elements, keyed by ID.
        QHash<Id, TreeDbElement> elements;

        /// The child IDs of each parent, each list in insertion order.
        QHash<Id, QVector<Id> > childrenOf;

        /// The next ID to assign to a new element.
        Id nextId = 1;

        /// @brief Inserts the specified elements and assigns them IDs.
        /// @param added The elements to insert, parents before children.
        /// @return The result, with the IDs assigned to the inserted elements on success.
        CommitResult applyAdded(const QVector<ChangeSet::Added>& added);

        /// @brief Updates the specified elements.
        /// @param modified The elements to update.
        /// @return `CommitStatus::Success`, or the reason an element could not be updated.
        CommitStatus applyModified(const QVector<ChangeSet::Modified>& modified);

        /// @brief Deletes the specified elements with their subtrees.
        /// @param deleted The elements to delete.
        /// @return `CommitStatus::Success`, or the reason an element could not be deleted.
        CommitStatus applyDeleted(const QVector<ChangeSet::Deleted>& deleted);
    };

    /// @brief Replaces the entire element tree.
    /// @param elements The elements, parents before children.
    void seed(const std::vector<TreeDbElement>& elements);

    /// @brief Returns the number of direct children of the element that are not deleted.
    /// @param id The ID of the element.
    /// @return The number of children.
    int childCount(Id id) const;

    /// @brief Appends an element and its whole subtree, parents before children.
    /// @param id The ID of the element.
    /// @param includeDeleted Whether deleted elements and their subtrees will be added.
    /// @param treeElements The tree elements to append to.
    void appendSubtree(Id id, bool includeDeleted, std::vector<TreeDbElement>& treeElements) const;

    /// The elements of the initial tree.
    std::vector<TreeDbElement> m_initialElements;

    /// The current state of the database.
    Storage m_storage;
};
