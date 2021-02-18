vertex
vec4 vertexPosition(attr vec4 vertexPositionAttr)
{
	vertexPosition = vertexPositionAttr;
}

vertex
vec3 vertexNormal(attr vec3 vertexNormalAttr)
{
	vertexNormal = vertexNormalAttr;
}

vertex
vec2 vertexUv0(attr vec2 vertexUv0Attr)
{
	vertexUv0 = vertexUv0Attr;
}

vertex
vec3 vertexColor(attr vec3 vertexColorAttr)
{
	vertexColor = vertexColorAttr;
}

vertex
vec3 normal(mat4 modelMatrix, vec3 vertexNormal)
{
	normal = normalize((modelMatrix * vec4(vertexNormal, 0.0)).xyz);
}

vertex
mat4 modelViewMatrix(mat4 viewMatrix, mat4 modelMatrix)
{
	modelViewMatrix = viewMatrix * modelMatrix;
}

vertex
out vec4 gl_Position(mat4 modelViewMatrix, mat4 projectionMatrix, vec4 vertexPosition)
{
	gl_Position = projectionMatrix * modelViewMatrix * vertexPosition;
}

vertex
out vec4 gl_Position(mat4 modelViewMatrix, mat4 projectionMatrix, attr vec4 skyVertexPositionAttr)
{
	gl_Position = (projectionMatrix * modelViewMatrix * skyVertexPositionAttr).xyww;
}

vertex
out vec4 gl_Position(vec2 vertexUv0)
{
	/* Texture space */
	gl_Position = vec4(vertexUv0 * vec2(2.0, 2.0) + vec2(-1.0, -1.0), 0.0, 1.0);
}




fragment
vec3 selectedColor(vec3 vertexColor, sampler2D diffuseTexture, vec2 vertexUv0)
{
	selectedColor = vertexColor * texture(diffuseTexture, vertexUv0).rgb;
}

fragment
vec3 selectedColor(sampler2D diffuseTexture, vec2 vertexUv0)
{
	vec4 texOut = texture(diffuseTexture, vertexUv0);
	if(texOut.a <= 0.1)
		discard;
	selectedColor = texOut.rgb;
}

fragment
vec3 selectedColor(vec3 vertexColor)
{
	selectedColor = vertexColor;
}

fragment
vec3 selectedColor(vec3 diffuseColor)
{
	selectedColor = diffuseColor;
}

fragment
out vec4 fragmentColor(sampler2D signedDistanceFieldTexture, vec2 vertexUv0, vec3 selectedColor)
{
	float dist = texture(signedDistanceFieldTexture, vertexUv0).r;
	float width = fwidth(dist);
	float alpha = smoothstep(0.5-width, 0.5+width, dist);
	if(alpha < 0.5)
		discard;
	fragmentColor = vec4(selectedColor, alpha);
}

fragment
out vec4 fragmentColor(vec3 diffuseLighting, vec3 selectedColor, float opacity)
{
	fragmentColor = vec4(diffuseLighting * selectedColor, opacity);
}

fragment
out vec4 fragmentColor(vec3 selectedColor, float opacity)
{
	fragmentColor = vec4(selectedColor, opacity);
}

fragment
out vec4 fragmentColor(vec3 emissionColor, float opacity)
{
	fragmentColor = vec4(emissionColor, opacity);
}

fragment
out vec4 fragmentColor()
{
	fragmentColor = vec4(0.5f, 0.5f, 0.5f, 1.0f);
}

fragment
float opacity(sampler2D diffuseAndOpacityTexture, vec2 vertexUv0)
{
	opacity = texture(diffuseAndOpacityTexture, vertexUv0).a;
}

fragment
float opacity()
{
	opacity = 1.0;
}

fragment
float diffuseTerm(vec3 normal, vec3 lightDirection0)
{
	diffuseTerm = max(dot(normal, -lightDirection0), 0.0);
}

fragment
out vec4 fragmentColor(float diffuseTerm, vec3 selectedColor, vec3 ambientColor, float opacity)
{
	vec3 outColor = selectedColor * diffuseTerm;
	outColor += ambientColor;
	fragmentColor =  vec4(outColor, opacity);
}

fragment
float ambientTerm()
{
	ambientTerm = 0.05;
}

fragment
vec3 ambientColor(vec3 selectedColor, float ambientTerm)
{
	ambientColor = selectedColor * ambientTerm;
}

geometry in_prim=lines out_prim=line_strip max_vert=10
int testFunction()
{
	testFunction=50;
}

