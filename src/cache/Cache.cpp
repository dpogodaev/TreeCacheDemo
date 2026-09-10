#include "cache/Cache.h"

#include <algorithm>
#include <utility>

#include <QString>
#include <QtGlobal>

namespace
{
    const QString kNewElementText = QStringLiteral("New element");
} // namespace

void Cache::clear()
{
    m_storage = {};
}

bool Cache::contains(const Id id) const
{
    return m_storage.elements.contains(id);
}

void Cache::load(const TreeElement& element)
{
    if (m_storage.elements.contains(element.id)) return;

    const TreeCacheElement cacheElement{
        .id = element.id,
        .parentId = element.parentId,
        .text = element.text,
        .hasChildren = element.hasChildren,
        .state = TrackingState::Unchanged,
    };
    m_storage.elements.insert(cacheElement.id, cacheElement);

    linkIntoTree(cacheElement);
    linkWaitingChildren(cacheElement.id);
}

std::optional<Id> Cache::addChild(const Id parentId)
{
    const bool usable = isUsableParent(parentId);
    Q_ASSERT(usable);
    if (!usable) return std::nullopt;

    const Id localId = generateLocalId();
    const TreeCacheElement element{
        .id = localId,
        .parentId = parentId,
        .text = kNewElementText,
        .hasChildren = false,
        .state = TrackingState::Added,
    };

    m_storage.elements.insert(localId, element);
    m_storage.childrenOf[parentId].append(localId);

    return localId;
}

void Cache::editText(const Id id, const QString& text)
{
    const auto element = m_storage.elements.find(id);
    if (element == m_storage.elements.end() || element->state == TrackingState::Deleted) return;

    if (text.isEmpty() || text == element->text) return;

    element->text = text;

    if (element->state == TrackingState::Unchanged)
    {
        element->state = TrackingState::Modified;
    }
}

void Cache::remove(const Id id)
{
    const auto element = m_storage.elements.find(id);
    if (element == m_storage.elements.end() || element->state == TrackingState::Deleted) return;

    if (element->state == TrackingState::Added)
    {
        discardSubtree(id);
    } else
    {
        markSubtreeAsDeleted(id);
    }
}

bool Cache::hasPendingChanges() const
{
    for (const TreeCacheElement& element: m_storage.elements)
    {
        if (element.state != TrackingState::Unchanged) return true;
    }

    return false;
}

ChangeSet Cache::buildChangeSet() const
{
    ChangeSet changes;

    for (const TreeCacheElement& element: m_storage.elements)
    {
        switch (element.state)
        {
            case TrackingState::Added:
                changes.added.append(
                    ChangeSet::Added{.localId = element.id, .parentId = element.parentId, .text = element.text});
                break;
            case TrackingState::Modified:
                changes.modified.append(ChangeSet::Modified{.id = element.id, .text = element.text});
                break;
            case TrackingState::Deleted:
                changes.deleted.append(ChangeSet::Deleted{.id = element.id});
                break;
            case TrackingState::Unchanged:
                break;
        }
    }

    sortAddedParentsFirst(changes.added);

    return changes;
}

void Cache::acceptCommit(const CommitResult& result)
{
    Q_ASSERT(result.isSuccess());

    removeElementsMarkedAsDeleted();
    replaceLocalIds(result.idMap);
    markAllElementsAsUnchanged();
}

QVector<Id> Cache::roots() const
{
    return m_storage.roots;
}

QVector<Id> Cache::childrenOf(const Id id) const
{
    const auto childrenIds = m_storage.childrenOf.constFind(id);
    return childrenIds != m_storage.childrenOf.constEnd() ? childrenIds.value() : QVector<Id>{};
}

std::optional<TreeCacheElement> Cache::element(const Id id) const
{
    const auto element = m_storage.elements.constFind(id);

    if (element == m_storage.elements.constEnd())
    {
        return std::nullopt;
    }

    return element.value();
}

void Cache::linkIntoTree(const TreeCacheElement& element)
{
    if (element.parentId != RootId && m_storage.elements.contains(element.parentId))
    {
        m_storage.childrenOf[element.parentId].append(element.id);
    } else
    {
        m_storage.roots.append(element.id);
    }
}

bool Cache::isUsableParent(const Id id) const
{
    const auto element = m_storage.elements.constFind(id);
    return element != m_storage.elements.constEnd() && element->state != TrackingState::Deleted;
}

Id Cache::generateLocalId()
{
    return m_storage.nextLocalId--;
}

void Cache::sortAddedParentsFirst(QVector<ChangeSet::Added>& added)
{
    std::ranges::sort(added, std::ranges::greater{}, &ChangeSet::Added::localId);
}

void Cache::removeElementsMarkedAsDeleted()
{
    QVector<Id> idsToRemove;

    for (auto element = m_storage.elements.constBegin(); element != m_storage.elements.constEnd(); ++element)
    {
        if (element->state == TrackingState::Deleted)
        {
            idsToRemove.append(element.key());
        }
    }

    for (const Id id: idsToRemove)
    {
        unlinkFromTree(id);
        m_storage.elements.remove(id);
        m_storage.childrenOf.remove(id);
    }
}

void Cache::replaceLocalIds(const QHash<Id, Id>& idMap)
{
    m_storage.elements = remap(m_storage.elements, idMap);
    m_storage.childrenOf = remap(m_storage.childrenOf, idMap);
    m_storage.roots = remap(m_storage.roots, idMap);
}

void Cache::markAllElementsAsUnchanged()
{
    for (TreeCacheElement& element: m_storage.elements)
    {
        element.state = TrackingState::Unchanged;
    }
}

Id Cache::assignedId(const QHash<Id, Id>& idMap, const Id id)
{
    const auto mapping = idMap.constFind(id);
    return mapping != idMap.constEnd() ? mapping.value() : id;
}

QHash<Id, TreeCacheElement> Cache::remap(const QHash<Id, TreeCacheElement>& elements, const QHash<Id, Id>& idMap)
{
    QHash<Id, TreeCacheElement> result;
    result.reserve(elements.size());

    for (TreeCacheElement element: elements)
    {
        element.id = assignedId(idMap, element.id);
        element.parentId = assignedId(idMap, element.parentId);

        result.insert(element.id, element);
    }

    return result;
}

QHash<Id, QVector<Id>> Cache::remap(const QHash<Id, QVector<Id>>& childrenOf, const QHash<Id, Id>& idMap)
{
    QHash<Id, QVector<Id>> result;
    result.reserve(childrenOf.size());

    for (auto it = childrenOf.constBegin(); it != childrenOf.constEnd(); ++it)
    {
        const Id parentId = it.key();
        const QVector<Id>& childrenIds = it.value();

        QVector<Id> remappedChildrenIds;
        remappedChildrenIds.reserve(childrenIds.size());

        for (const Id childId: childrenIds)
        {
            remappedChildrenIds.append(assignedId(idMap, childId));
        }

        result.insert(assignedId(idMap, parentId), remappedChildrenIds);
    }

    return result;
}

QVector<Id> Cache::remap(const QVector<Id>& roots, const QHash<Id, Id>& idMap)
{
    QVector<Id> result;
    result.reserve(roots.size());

    for (const Id rootId: roots)
    {
        result.append(assignedId(idMap, rootId));
    }

    return result;
}

void Cache::linkWaitingChildren(const Id parentId)
{
    QVector<Id> waitingChildren;

    for (const Id rootId: m_storage.roots)
    {
        if (rootId == parentId) continue;

        if (const auto element = m_storage.elements.constFind(rootId);
            element != m_storage.elements.constEnd() && element->parentId == parentId)
        {
            waitingChildren.append(rootId);
        }
    }

    for (const Id childId: waitingChildren)
    {
        m_storage.roots.removeOne(childId);
        m_storage.childrenOf[parentId].append(childId);
    }
}

void Cache::markSubtreeAsDeleted(const Id id)
{
    QVector<Id> toVisit{id};

    while (!toVisit.isEmpty())
    {
        const Id currentId = toVisit.takeLast();

        if (m_storage.elements.value(currentId).state == TrackingState::Added)
        {
            discardSubtree(currentId);
            continue;
        }

        m_storage.elements[currentId].state = TrackingState::Deleted;

        toVisit += m_storage.childrenOf.value(currentId);
    }
}

void Cache::discardSubtree(const Id id)
{
    const QVector<Id> subtree = collectSubtree(id);

    unlinkFromTree(id);

    for (const Id elementId: subtree)
    {
        m_storage.elements.remove(elementId);
        m_storage.childrenOf.remove(elementId);
    }
}

QVector<Id> Cache::collectSubtree(const Id id) const
{
    QVector<Id> subtree;
    QVector<Id> toVisit{id};

    while (!toVisit.isEmpty())
    {
        const Id currentId = toVisit.takeLast();
        subtree.append(currentId);

        if (const auto children = m_storage.childrenOf.constFind(currentId);
            children != m_storage.childrenOf.constEnd())
        {
            toVisit += children.value();
        }
    }

    return subtree;
}

void Cache::unlinkFromTree(const Id id)
{
    const auto element = m_storage.elements.constFind(id);
    if (element == m_storage.elements.constEnd()) return;

    if (const auto children = m_storage.childrenOf.find(element->parentId);
        children != m_storage.childrenOf.end())
    {
        children->removeOne(id);
    } else
    {
        m_storage.roots.removeOne(id);
    }
}
