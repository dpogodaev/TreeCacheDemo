#pragma once

#include <QString>
#include <QVector>

#include "domain/Id.h"

/// @brief The set of pending changes to apply to the database.
struct ChangeSet
{
    /// @brief A new element to insert.
    struct Added
    {
        /// The local temporary ID of the element.
        Id localId;

        /// The ID of the parent.
        Id parentId;

        /// The text of the element.
        QString text;
    };

    /// @brief An element to update.
    struct Modified
    {
        /// The ID of the element to update.
        Id id;

        /// The new text of the element.
        QString text;
    };

    /// @brief An element to delete.
    struct Deleted
    {
        /// The ID of the element to delete.
        Id id;
    };

    /// The elements to insert, ordered so that every parent precedes its children.
    QVector<Added> added;

    /// The text changes to apply.
    QVector<Modified> modified;

    /// The elements to delete.
    QVector<Deleted> deleted;
};
