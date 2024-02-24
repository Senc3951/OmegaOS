#pragma once

#include <fs/vfs.h>

#define stdin   0
#define stdout  1
#define stderr  2

/// @brief Create a stdin vfs node.
/// @return Node, NULL, if failed.
VfsNode_t *createStdinNode();

/// @brief Create a stdout vfs node.
/// @return Node, NULL, if failed.
VfsNode_t *createStdoutNode();

/// @brief Create a stderr vfs node.
/// @return Node, NULL, if failed.
VfsNode_t *createStderrNode();