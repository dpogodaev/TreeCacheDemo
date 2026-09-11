#include "db/Database.h"

#include <algorithm>
#include <utility>

Database::Database(std::vector<TreeDbElement> initialElements)
    : m_initialElements(std::move(initialElements))
{
    seed(m_initialElements);
}

void Database::reset()
{
    seed(m_initialElements);
}

void Database::seed(const std::vector<TreeDbElement>& elements)
{
    Storage storage;

    for (const TreeDbElement& element: elements)
    {
        storage.elements.insert(element.id, element);

        if (element.parentId != RootId)
        {
            storage.childrenOf[element.parentId].append(element.id);
        }

        storage.nextId = std::max(storage.nextId, element.id + 1);
    }

    m_storage = std::move(storage);
}

std::optional<TreeElement> Database::get(const Id id) const
{
    const auto searchResult = m_storage.elements.constFind(id);

    if (searchResult == m_storage.elements.constEnd() || searchResult->isDeleted)
    {
        return std::nullopt;
    }

    const TreeDbElement& element = searchResult.value();

    return TreeElement{
        .id = element.id,
        .parentId = element.parentId,
        .text = element.text,
        .childCount = childCount(id)
    };
}

int Database::childCount(const Id id) const
{
    const auto searchResult = m_storage.childrenOf.constFind(id);

    if (searchResult == m_storage.childrenOf.constEnd())
    {
        return 0;
    }

    int count = 0;
    for (const Id childId: searchResult.value())
    {
        const auto child = m_storage.elements.constFind(childId);

        if (child != m_storage.elements.constEnd() && !child->isDeleted)
        {
            ++count;
        }
    }

    return count;
}

CommitResult Database::Storage::applyAdded(const QVector<ChangeSet::Added>& added)
{
    CommitResult commitResult;

    for (const ChangeSet::Added& item: added)
    {
        Id parentId = item.parentId;

        if (isTemporary(parentId))
        {
            const auto mapped = commitResult.idMap.constFind(parentId);

            if (mapped == commitResult.idMap.constEnd())
            {
                return CommitResult{.status = CommitStatus::UnresolvedParent};
            }

            parentId = mapped.value();
        }

        if (parentId != RootId && !elements.contains(parentId))
        {
            return CommitResult{.status = CommitStatus::UnknownParent};
        }

        const Id newId = nextId++;
        const auto newElement = TreeDbElement{
            .id = newId,
            .parentId = parentId,
            .text = item.text, .isDeleted = false
        };

        elements.insert(newId, newElement);
        childrenOf[parentId].append(newId);
        commitResult.idMap.insert(item.localId, newId);
    }

    return commitResult;
}

CommitStatus Database::Storage::applyModified(const QVector<ChangeSet::Modified>& modified)
{
    for (const ChangeSet::Modified& item: modified)
    {
        const auto searchResult = elements.find(item.id);

        if (searchResult == elements.end())
        {
            return CommitStatus::UnknownElement;
        }

        if (searchResult->isDeleted)
        {
            return CommitStatus::ElementDeleted;
        }

        searchResult->text = item.text;
    }

    return CommitStatus::Success;
}

CommitStatus Database::Storage::applyDeleted(const QVector<ChangeSet::Deleted>& deleted)
{
    for (const ChangeSet::Deleted& item: deleted)
    {
        const auto searchResult = elements.find(item.id);

        if (searchResult == elements.end())
        {
            return CommitStatus::UnknownElement;
        }

        if (searchResult->isDeleted) continue;

        QVector<Id> toDelete{item.id};
        while (!toDelete.isEmpty())
        {
            const Id id = toDelete.takeLast();
            elements[id].isDeleted = true;

            const auto children = childrenOf.constFind(id);
            if (children != childrenOf.constEnd())
            {
                toDelete += children.value();
            }
        }
    }

    return CommitStatus::Success;
}

CommitResult Database::commit(const ChangeSet& changes)
{
    Storage storage = m_storage;

    CommitResult commitResult = storage.applyAdded(changes.added);
    if (!commitResult.isSuccess())
    {
        return commitResult;
    }

    const CommitStatus modifiedStatus = storage.applyModified(changes.modified);
    if (modifiedStatus != CommitStatus::Success)
    {
        return CommitResult{.status = modifiedStatus};
    }

    const CommitStatus deletedStatus = storage.applyDeleted(changes.deleted);
    if (deletedStatus != CommitStatus::Success)
    {
        return CommitResult{.status = deletedStatus};
    }

    m_storage = std::move(storage);

    return commitResult;
}

std::vector<TreeDbElement> Database::getAll(const bool includeDeleted) const
{
    std::vector<Id> rootIds;

    for (auto it = m_storage.elements.constBegin(); it != m_storage.elements.constEnd(); ++it)
    {
        if (it.value().parentId == RootId)
        {
            rootIds.push_back(it.key());
        }
    }

    std::ranges::sort(rootIds);

    std::vector<TreeDbElement> treeElements;
    treeElements.reserve(static_cast<std::size_t>(m_storage.elements.size()));

    for (const Id rootId: rootIds)
    {
        appendSubtree(rootId, includeDeleted, treeElements);
    }

    return treeElements;
}

void Database::appendSubtree(
    const Id id,
    const bool includeDeleted,
    std::vector<TreeDbElement>& treeElements) const
{
    const auto searchResult = m_storage.elements.constFind(id);

    if (searchResult == m_storage.elements.constEnd()) return;

    const TreeDbElement& element = searchResult.value();

    if (element.isDeleted && !includeDeleted) return;

    treeElements.push_back(element);

    const auto children = m_storage.childrenOf.constFind(id);

    if (children == m_storage.childrenOf.constEnd()) return;

    for (const Id childId: children.value())
    {
        appendSubtree(childId, includeDeleted, treeElements);
    }
}
