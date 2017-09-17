﻿

class sampler1D;
class sampler2D;
class sampler3D;
class samplerCUBE;


class texture;
class sampler_state;

class SamplerState;

class Texture1D;
class Texture2D;
class Texture3D;

template< class T >
struct TVec2
{
	float x, y;
	float r, g;
	float s, t;
};

template< class T >
struct TVec3 : TVec2<T>
{
	float2 xy, st;
	float2 xz, yz;
	float  z, b, p;
};

template< class T >
struct TVec4 : TVec3<T>
{
	float3 xyz, rgb, stp;
	float  w, a, q;

	float2 zw;
	float3 rga;
};

typedef TVec4<float> float4;
typedef TVec3<float> float3;
typedef TVec2<float> float2;

typedef TVec4<uint> uint4;
typedef TVec3<uint> uint3;
typedef TVec2<uint> uint2;

typedef TVec4<int> int4;
typedef TVec3<int> int3;
typedef TVec2<int> int2;

class float3x3
{
	float3 operator[](int);
};

class float4x3
{
	float3 operator[](int);
};

class float3x4
{
	float4 operator[](int);
};

struct float4x4
{
	float4 operator[](int);
};


template< class T >
class Buffer
{
};

float4   tex2D(sampler2D, float2);
float4   texCUBE(samplerCUBE, float3);


float    dot(float4, float4);
vec3     cross(vec3, vec3);

template< class T >
float    length(T);

float4x4 mul(float4x4, float4x4);
float4   mul(float4, float4x4);
float3   mul(float3, float3x3);

float    saturate(float);
float3   reflect(float3, float3);
float3   normalize(float3);

float4   lerp(float4, float4, float);
float4x4 transpose(float4x4);
float4x4 inverse(float4x4);

template< class T >
T        mix(T a, T b, float f);

typedef float  half;
typedef float2 half2;
typedef float3 half3;
typedef float4 half4;
typedef float3x3 half3x3;
typedef float4x4 half4x4;
typedef float4x3 half4x3;

#define uniform
#define attribute
#define varying
#define discard
#define layout

enum
{
	depth_unchanged,

};


class sampler1D;
class sampler2D;
class sampler3D;
class samplerCube;


typedef float2 vec2;
typedef float3 vec3;
typedef float4 vec4;
typedef uint2 uvec2;
typedef uint3 uvec3;
typedef uint4 uvec4;
typedef int2 ivec2;
typedef int3 ivec3;
typedef int4 ivec4;
typedef float3x3 mat3;
typedef float4x4 mat4;

vec4 texture1D(sampler1D, float);
vec4 texture2D(sampler2D, vec2);
vec4 texture3D(sampler3D, vec3);

vec4 texture(sampler1D, float);
vec4 texture(sampler2D, vec2);
vec4 texture(sampler3D, vec3);
vec4 texture(samplerCube, vec3);


int textureSize(sampler1D, int);
template< class T >
T dFdx(T);
template< class T >
T dFdy(T);


//BUILT-IN CONSTANTS (7.4 p44) 
const int gl_MaxVertexUniformComponents;
const int gl_MaxFragmentUniformComponents;
const int gl_MaxVertexAttribs;
const int gl_MaxVaryingFloats;
const int gl_MaxDrawBuffers;
const int gl_MaxTextureCoords;
const int gl_MaxTextureUnits;
const int gl_MaxTextureImageUnits;
const int gl_MaxVertexTextureImageUnits;
const int gl_MaxCombinedTextureImageUnits;
const int gl_MaxLights;
const int gl_MaxClipPlanes;

//VERTEX SHADER VARIABLES 

vec4 ftransform();


//Attribute Inputs (7.3 p44) access=RO
attribute vec4 gl_Vertex;
attribute vec3 gl_Normal;
attribute vec4 gl_Color;
attribute vec4 gl_SecondaryColor;
attribute vec4 gl_MultiTexCoord0;
attribute vec4 gl_MultiTexCoord1;
attribute vec4 gl_MultiTexCoord2;
attribute vec4 gl_MultiTexCoord3;
attribute vec4 gl_MultiTexCoord4;
attribute vec4 gl_MultiTexCoord5;
attribute vec4 gl_MultiTexCoord6;
attribute vec4 gl_MultiTexCoord7;
attribute vec4 gl_FogCoord;

//Special Output Variables (7.1 p42) access=RW 
vec4  gl_Position;  //shader must write 
float gl_PointSize;  //enable GL_VERTEX_PROGRAM_POINT_SIZE 
vec4  gl_ClipVertex;
//Varying Outputs (7.6 p48) access=RW
varying vec4  gl_FrontColor;
varying vec4  gl_BackColor; //enable GL_VERTEX_PROGRAM_TWO_SIDE 
varying vec4  gl_FrontSecondaryColor;
varying vec4  gl_BackSecondaryColor;
varying vec4  gl_TexCoord[gl_MaxTextureCoords];
varying float gl_FogFragCoord;

//FRAGMENT SHADER VARIABLES 

//Varying Inputs (7.6 p48) access=RO 
varying vec4  gl_Color;
varying vec4  gl_SecondaryColor;
varying vec4  gl_TexCoord[gl_MaxTextureCoords];
varying float gl_FogFragCoord;
//Special Input Variables (7.2 p43) access=RO 
vec4 gl_FragCoord; //pixel coordinates 
bool gl_FrontFacing;
//Special Output Variables (7.2 p43) access=RW 
vec4  gl_FragColor;
vec4  gl_FragData[gl_MaxDrawBuffers];
float gl_FragDepth; //DEFAULT=glFragCoord.z


					//BUILT-IN UNIFORMs (7.5 p45) access=RO 
uniform mat4 gl_ModelViewMatrix;
uniform mat4 gl_ModelViewProjectionMatrix;
uniform mat4 gl_ProjectionMatrix;
uniform mat4 gl_TextureMatrix[gl_MaxTextureCoords];

uniform mat4 gl_ModelViewMatrixInverse;
uniform mat4 gl_ModelViewProjectionMatrixInverse;
uniform mat4 gl_ProjectionMatrixInverse;
uniform mat4 gl_TextureMatrixInverse[gl_MaxTextureCoords];

uniform mat4 gl_ModelViewMatrixTranspose;
uniform mat4 gl_ModelViewProjectionMatrixTranspose;
uniform mat4 gl_ProjectionMatrixTranspose;
uniform mat4 gl_TextureMatrixTranspose[gl_MaxTextureCoords];

uniform mat4 gl_ModelViewMatrixInverseTranspose;
uniform mat4 gl_ModelViewProjectionMatrixInverseTranspose;
uniform mat4 gl_ProjectionMatrixInverseTranspose;
uniform mat4 gl_TextureMatrixInverseTranspose[gl_MaxTextureCoords];

uniform mat3  gl_NormalMatrix;
uniform float gl_NormalScale;

struct gl_DepthRangeParameters
{
	float near;
	float far;
	float diff;
};
uniform gl_DepthRangeParameters gl_DepthRange;

struct gl_FogParameters
{
	vec4 color;
	float density;
	float start;
	float end;
	float scale;
};
uniform gl_FogParameters gl_Fog;

struct gl_LightSourceParameters
{
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
	vec4 position;
	vec4 halfVector;
	vec3 spotDirection;
	float spotExponent;
	float spotCutoff;
	float spotCosCutoff;
	float constantAttenuation;
	float linearAttenuation;
	float quadraticAttenuation;
};
uniform gl_LightSourceParameters gl_LightSource[gl_MaxLights];


struct gl_LightModelProductParams
{
	vec4 sceneColor; // Derived. Ecm + Acm * Acs  
};


uniform gl_LightModelProductParams gl_FrontLightModelProduct;
uniform gl_LightModelProductParams gl_BackLightModelProduct;

struct gl_LightProductParams
{
	vec4 ambient;    // Acm * Acli    
	vec4 diffuse;    // Dcm * Dcli   
	vec4 specular;   // Scm * Scli  
};


uniform gl_LightProductParams gl_FrontLightProduct[gl_MaxLights];
uniform gl_LightProductParams gl_BackLightProduct[gl_MaxLights];

//Geometry Shader

in gl_PerVertex
{
	vec4 gl_Position;
float gl_PointSize;
float gl_ClipDistance[];
} gl_in[];


//layout(points, invocations = 1) in;
//layout(line_strip, max_vertices = 2) out;

int max_vertices;
int invocations;

in int gl_PrimitiveIDIn;
in int gl_InvocationID;  //Requires GLSL 4.0 or ARB_gpu_shader5

						 //Layered rendering
out int gl_Layer;
out int gl_ViewportIndex; //Requires GL 4.1 or ARB_viewport_array.

enum
{
	points,
	lines,
	lines_adjacency,
	triangles,
	triangles_adjacency,
};

enum
{
	points,
	line_strip,
	triangle_strip,
};

#define shared

void EmitVertex();
void EndPrimitive();

//Compute Shader

//layout(local_size_x = X​, local_size_y = Y​, local_size_z = Z​) in;
int local_size_x;
int local_size_y;
int local_size_z;

in uvec3 gl_NumWorkGroups;
in uvec3 gl_WorkGroupID;
in uvec3 gl_WorkGroupSize;
in uvec3 gl_LocalInvocationID;
in uvec3 gl_GlobalInvocationID;
in uint  gl_LocalInvocationIndex;



//
class image1D;
class image2D;
class image3D;
class iimage1D;
class iimage2D;
class iimage3D;
class uimage1D;
class uimage2D;
class uimage3D;

enum
{
	rgba16f ,
	r32i ,
	rg32i,
	rgb32i,
	rgba32i,
	r32ui,
	rg32ui,
	rgb32ui,
	rgba32ui,
};

#define coherent
#define volatile
#define restrict
#define readonly
#define writeonly

template< class gimage >
ivec2 imageSize(gimage image​);
template< class gimage, class CoordType >
vec4 imageLoad( gimage image​, CoordType);
template< class gimage, class CoordType >
void imageStore(gimage image​, CoordType, vec4 data​);

template< class gimage >
uint imageAtomicAdd(gimage image​, uint data);
template< class gimage >
uint imageAtomicMin(gimage image​, uint data);
template< class gimage >
uint imageAtomicMax(gimage image​, uint data);
template< class gimage >
uint imageAtomicAnd(gimage image​, uint data);
template< class gimage >
uint imageAtomicOr(gimage image​, uint data);
template< class gimage >
uint imageAtomicXor(gimage image​, uint data);
template< class gimage >
uint imageAtomicExchange(gimage image​, uint data);
template< class gimage >
uint imageAtomicCompSwap(gimage image​, uint compare, uint data);

//Atomic Counter

uint atomicCounter(atomic_uint c​);
uint atomicCounterIncrement(atomic_uint c​);
uint atomicCounterDecrement(atomic_uint c​);