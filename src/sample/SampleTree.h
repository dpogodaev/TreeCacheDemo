#pragma once

/// @file
/// @brief The sample tree the application starts with.

#include <vector>

#include "db/TreeDbElement.h"

/// @brief Builds the sample tree.
/// @remarks
/// One company with two departments.
/// Each department has two units.
/// Each unit two teams.
/// Some teams have one or two employees.
/// @return The elements of the tree, parents before children.
std::vector<TreeDbElement> buildSampleTree();
