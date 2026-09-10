#pragma once

#include <QString>

#include "cache/TrackingState.h"
#include "domain/Id.h"

/// @brief An element tracked by the cache.
struct TreeCacheElement
{
    /// The ID of the element.
    Id id = UndefinedId;

    /// The ID of the parent.
    Id parentId = UndefinedId;

    /// The text of the element.
    QString text;

    /// Whether the element has children in the database.
    bool hasChildren = false;

    /// The change-tracking state of the element.
    TrackingState state = TrackingState::Unchanged;
};
