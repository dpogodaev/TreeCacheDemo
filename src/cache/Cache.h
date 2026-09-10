#pragma once

#include <optional>

#include <QHash>
#include <QString>
#include <QVector>

#include "cache/TreeCacheElement.h"
#include "db/ChangeSet.h"
#include "db/CommitResult.h"
#include "domain/Id.h"
#include "domain/TreeElement.h"

/// @brief The local change-tracking cache of tree elements.
/// @remarks
/// The cache never calls the database.
class Cache
{
public:
    /// @brief Removes all elements from the cache.
    void clear();

    /// @brief Returns whether an element with the specified ID is in the cache.
    /// @param id The ID of the element.
    /// @return `true` if the element is in the cache; otherwise, `false`.
    [[nodiscard]] bool contains(Id id) const;

    /// @brief Loads an element into the cache.
    /// @remarks
    /// The element is tracked as `TrackingState::Unchanged`.
    /// A child loaded before its parent is attached to it when the parent is loaded.
    /// @param element The element to load.
    void load(const TreeElement& element);

    /// @brief Adds a new local child under a parent in the cache.
    /// @remarks The child is tracked as `TrackingState::Added`.
    /// @pre The parent is in the cache and is not marked as `TrackingState::Deleted`.
    /// @param parentId The ID of the parent.
    /// @return The new local ID, or `std::nullopt` if the precondition is not met.
    std::optional<Id> addChild(Id parentId);

    /// @brief Changes an element's text.
    /// @remarks
    /// A `TrackingState::Unchanged` element becomes `TrackingState::Modified`.<br/>
    /// A `TrackingState::Added` element stays `TrackingState::Added`.<br/>
    /// Empty text and `TrackingState::Deleted` elements are ignored.
    /// @param id The ID of the element to change.
    /// @param text The new text.
    void editText(Id id, const QString& text);

    /// @brief Deletes an element with its subtree.
    /// @remarks
    /// An added element and its whole subtree are discarded.<br/>
    /// A loaded element and its loaded subtree become `TrackingState::Deleted`;
    /// any added element in that subtree is discarded too.
    /// The rest of the subtree — elements never loaded — is deleted when the change set is applied.<br/>
    /// An element that is missing or already `TrackingState::Deleted` is ignored.
    /// @param id The ID of the element to delete.
    void remove(Id id);

    /// @brief Returns whether the cache has pending changes.
    /// @remarks A pending change is an element in any state other than `TrackingState::Unchanged`.
    /// @return `true` if there is a pending change; otherwise, `false`.
    [[nodiscard]] bool hasPendingChanges() const;

    /// @brief Builds the set of pending changes.
    /// @return The pending changes. Added elements are ordered so that every parent precedes its children.
    [[nodiscard]] ChangeSet buildChangeSet() const;

    /// @brief Updates the cache after a commit.
    /// @pre The commit succeeded.
    /// @remarks
    /// Elements marked as `TrackingState::Deleted` are removed from the cache.<br/>
    /// Local IDs are replaced with the assigned IDs.<br/>
    /// Every remaining element is set to `TrackingState::Unchanged`.
    /// @param result The commit result to apply.
    void acceptCommit(const CommitResult& result);

    /// @brief Returns the IDs of the root elements, in load order.
    /// @return The IDs of the loaded elements whose parent is not loaded.
    [[nodiscard]] QVector<Id> roots() const;

    /// @brief Returns an element's loaded children.
    /// @param id The ID of the parent.
    /// @return The child IDs, in load order; empty if the element has no loaded children.
    [[nodiscard]] QVector<Id> childrenOf(Id id) const;

    /// @brief Returns the element with the specified ID.
    /// @param id The ID of the element.
    /// @return The element with the specified ID, or `std::nullopt` if not found.
    [[nodiscard]] std::optional<TreeCacheElement> element(Id id) const;

private:
    /// @brief The state of the cache.
    struct Storage
    {
        /// The elements, keyed by ID.
        QHash<Id, TreeCacheElement> elements;

        /// The child IDs of each parent, each list in load order.
        QHash<Id, QVector<Id>> childrenOf;

        /// The IDs of the loaded elements whose parent is not loaded.
        QVector<Id> roots;

        /// The next local ID to assign to an added element.
        /// Negative and decreasing, so it never matches with a persisted ID.
        Id nextLocalId = -1;
    };

    /// @brief Links the element to its parent, or makes it a root if it has no parent or the parent is not loaded.
    /// @param element The element to link.
    void linkIntoTree(const TreeCacheElement& element);

    /// @brief Moves any root that was waiting for the specified parent into that parent's child list, in load order.
    /// @param parentId The ID of the parent — a just-loaded element.
    void linkWaitingChildren(Id parentId);

    /// @brief Returns whether the element can be a parent for a new child.
    /// @param id The ID of the element.
    /// @return `true` if the element is in the cache and is not marked as `TrackingState::Deleted`; otherwise, `false`.
    [[nodiscard]] bool isUsableParent(Id id) const;

    /// @brief Returns the next available local ID.
    /// @return The local ID.
    Id generateLocalId();

    /// @brief Sorts added elements so that every parent comes before its children.
    /// @remarks A child is always added after its parent, so its local ID is more negative.
    /// Sorting by descending local ID therefore places every parent before its children.
    /// @param added The added elements to sort.
    static void sortAddedParentsFirst(QVector<ChangeSet::Added>& added);

    /// @brief Removes every element marked as `TrackingState::Deleted` and unlinks it from the tree.
    void removeElementsMarkedAsDeleted();

    /// @brief Replaces every local ID in the cache with its assigned ID.
    /// @param idMap The mapping from local ID to assigned ID.
    void replaceLocalIds(const QHash<Id, Id>& idMap);

    /// @brief Marks all elements as `TrackingState::Unchanged`.
    void markAllElementsAsUnchanged();

    /// @brief Returns the assigned ID for the specified local ID, or the same ID if the map has no entry for it.
    /// @param idMap The mapping from local ID to assigned ID.
    /// @param id The ID to map.
    /// @return The assigned ID, or the same ID.
    static Id assignedId(const QHash<Id, Id>& idMap, Id id);

    /// @brief Returns a copy of the elements with every ID replaced by its assigned ID.
    /// @param elements The elements to remap.
    /// @param idMap The mapping from local ID to assigned ID.
    /// @return The elements, keyed by assigned ID.
    static QHash<Id, TreeCacheElement> remap(const QHash<Id, TreeCacheElement>& elements, const QHash<Id, Id>& idMap);

    /// @brief Returns a copy of the child lists with every ID replaced by its assigned ID.
    /// @param childrenOf The child lists to remap, keyed by parent ID.
    /// @param idMap The mapping from local ID to assigned ID.
    /// @return The child lists, keyed by assigned parent ID.
    static QHash<Id, QVector<Id>> remap(const QHash<Id, QVector<Id>>& childrenOf, const QHash<Id, Id>& idMap);

    /// @brief Returns a copy of the root IDs with every ID replaced by its assigned ID.
    /// @param roots The root IDs to remap.
    /// @param idMap The mapping from local ID to assigned ID.
    /// @return The root IDs.
    static QVector<Id> remap(const QVector<Id>& roots, const QHash<Id, Id>& idMap);

    /// @brief Returns all IDs in the element's subtree, including the root.
    /// @param id The ID of the subtree root.
    /// @return The list of IDs.
    [[nodiscard]] QVector<Id> collectSubtree(Id id) const;

    /// @brief Unlinks the element from its parent's child list, or from the roots.
    /// @param id The ID of the element.
    void unlinkFromTree(Id id);

    /// @brief Discards an added element and its subtree.
    /// @param id The ID of the element.
    void discardSubtree(Id id);

    /// @brief Marks the element and its loaded subtree as `TrackingState::Deleted`.
    /// @remarks An added element in the subtree is discarded instead — it was never persisted.
    /// @param id The ID of the element.
    void markSubtreeAsDeleted(Id id);

    /// The current state of the cache.
    Storage m_storage;
};
