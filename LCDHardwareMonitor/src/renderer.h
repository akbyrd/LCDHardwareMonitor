struct RendererState;

b32       Renderer_Initialize   (RendererState*, v2i renderSize);
void      Renderer_Teardown     (RendererState*);
DrawCall* Renderer_PushDrawCall (RendererState*);
b32       Renderer_Render       (RendererState*);
