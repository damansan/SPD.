#pragma once
#include <imgui_node_editor.h>

namespace ed = ax::NodeEditor;

class NodeEditorCanvas
{
public:
    NodeEditorCanvas() = default;
    ~NodeEditorCanvas();
    NodeEditorCanvas(const NodeEditorCanvas&) = delete;
    NodeEditorCanvas& operator=(const NodeEditorCanvas&) = delete;

    void Initialize();
    void Begin(const char* canvasId);
    void End();

private:
    ed::EditorContext* editorContext = nullptr;
};