struct Widget
{
	__declspec(align(16))
	struct VSConstants
	{

	};

	__declspec(align(16))
	struct PSConstants
	{
		//TODO: How do we get this? (Do the UV calculation in CPU, I think)
		V2i   res;

		V4    borderColor = { 1.0f, 0.0f, 0.0f, 1.0f };
		float borderSize  = 20.0f;
		float borderBlur  = 20.0f;

		V4    fillColor  = { 1.0f, 1.0f, 1.0f, 0.5f };
		float fillAmount = 0.5f;
		float fillBlur   = 10.0f;

		V4    backgroundColor = { 0.0f, 0.0f, 0.0f, 1.0f };
	};

	//TODO: This pointer will break when the list resizes (or sensors are added/removed)
	//Eventually we'll want to store something more indirect than a pointer.
	//Maybe a plugin id and sensor id pair? Maybe ids are never reused or invalid
	//This will tie closely into how we save layouts between sessions
	//Plugin* plugin;
	Sensor* sensor;

	V2i position;
	V2i size;

	VSConstants vsConstants;
	PSConstants psConstants;

	Mesh    mesh;
	//TODO: Maybe string and range overrides?
	//TODO: drawFn? Widget type to function map?
};

void
DrawWidget_FilledBar(Widget& w, RendererState* renderer)
{
	//Set Input Layout
	//Set Vertex Shader
	//Set VS constant buffer
	//Set Pixel Shader
	//Set PS constant buffer
}

//typedef FilledBarConstants ConstantBufferType;