#pragma once

#include <QHash>

#include "domain/Id.h"

/// @brief The status of a commit.
enum class CommitStatus
{
    Success,          ///< Every item was applied.
    UnresolvedParent, ///< The parent is added by the same change set but has no ID yet.
    UnknownParent,    ///< The parent does not exist in the database.
    UnknownElement,   ///< The element to modify or delete does not exist.
    ElementDeleted,   ///< The element to modify is already deleted.
};

/// @brief The result of a commit.
struct CommitResult
{
    /// The status of the commit.
    CommitStatus status = CommitStatus::Success;

    /// The mapping from each element's local temporary ID to its persisted ID.
    QHash<Id, Id> idMap = {};

    /// @brief Returns whether the commit succeeded.
    bool isSuccess() const { return status == CommitStatus::Success; }
};
