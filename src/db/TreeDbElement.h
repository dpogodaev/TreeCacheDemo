#pragma once

#include <QString>

#include "domain/Id.h"

/// @brief A persistent element of the tree.
struct TreeDbElement
{
    /// The ID of the element.
    Id id = UndefinedId;

    /// The ID of the parent.
    Id parentId = UndefinedId;

    /// The text of the element.
    QString text;

    /// Whether the element is marked as deleted.
    bool isDeleted = false;
};
