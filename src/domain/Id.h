#pragma once

#include <limits>

#include <QtGlobal>

/// @brief ID of a tree element.
/// @remarks
/// A value greater than zero identifies a persisted element.<br/>
/// A negative value is a temporary ID for an element that has not been committed.<br/>
/// @ref RootId and @ref UndefinedId are reserved.
using Id = qint64;

/// @brief Returns whether the ID identifies an element stored in the database.
constexpr bool isPersistent(const Id id) { return id > 0; }

/// @brief Returns whether the ID is a temporary ID of an element that has not been committed.
constexpr bool isTemporary(const Id id) { return id < 0; }

/// @brief The parent ID of a root element.
inline constexpr Id RootId = std::numeric_limits<Id>::max();

/// @brief An unset element ID.
inline constexpr Id UndefinedId = 0;
