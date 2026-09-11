#pragma once

#include <QString>

#include "domain/Id.h"

/// @brief A tree element's data.
struct TreeElement
{
    /// The ID of the element.
    Id id;

    /// The ID of the parent.
    Id parentId;

    /// The text of the element.
    QString text;

    /// The number of direct children of the element that are not deleted.
    int childCount;
};
