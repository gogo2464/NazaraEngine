/********************Entrant********************/
in mat4 InstanceData0;
in vec4 VertexColor;
in vec3 VertexPosition;
in vec2 VertexTexCoord;

/********************Sortant********************/
out vec4 vColor;
out vec2 vTexCoord;

/********************Uniformes********************/
uniform float VertexDepth;
uniform mat4 ViewProjMatrix;
uniform mat4 WorldViewProjMatrix;

/********************Fonctions********************/
void main()
{
#if FLAG_VERTEXCOLOR
	vec4 color = VertexColor;
#else
	vec4 color = vec4(1.0);
#endif

#if FLAG_INSTANCING
	#if TRANSFORM
	gl_Position = ViewProjMatrix * InstanceData0 * vec4(VertexPosition, 1.0);
	#else
		#if UNIFORM_VERTEX_DEPTH
		gl_Position = InstanceData0 * vec4(VertexPosition.xy, VertexDepth, 1.0);
		#else
		gl_Position = InstanceData0 * vec4(VertexPosition, 1.0);
		#endif
	#endif
#else
	#if TRANSFORM
	gl_Position = WorldViewProjMatrix * vec4(VertexPosition, 1.0);
	#else
		#if UNIFORM_VERTEX_DEPTH
		gl_Position = vec4(VertexPosition.xy, VertexDepth, 1.0);
		#else
		gl_Position = vec4(VertexPosition, 1.0);
		#endif
	#endif
#endif

	vColor = color;
#if TEXTURE_MAPPING
	vTexCoord = vec2(VertexTexCoord);
#endif
}
