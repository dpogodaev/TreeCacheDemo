#pragma once

/// @brief The change-tracking state of an element stored in the cache.
enum class TrackingState
{
    Unchanged, ///< Identical to the element in the database.
    Added,     ///< Created locally; no such element exists in the database yet.
    Modified,  ///< Changed locally; the element exists in the database.
    Deleted,   ///< Marked for deletion locally; the element exists in the database.
};
