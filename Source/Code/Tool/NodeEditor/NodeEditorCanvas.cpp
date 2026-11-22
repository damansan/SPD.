#include "NodeEditorCanvas.h"

NodeEditorCanvas::~NodeEditorCanvas()
{
    if (editorContext)
    {
        ed::DestroyEditor(editorContext);
        editorContext = nullptr;
    }
}

void NodeEditorCanvas::Initialize()
{
    ed::Config config;
    config.SettingsFile = nullptr;
    editorContext = ed::CreateEditor(&config);
}

void NodeEditorCanvas::Begin(const char* canvasId)
{
    ed::SetCurrentEditor(editorContext);
    ed::Begin(canvasId);
}

void NodeEditorCanvas::End()
{
    ed::End();
    ed::SetCurrentEditor(nullptr);
}