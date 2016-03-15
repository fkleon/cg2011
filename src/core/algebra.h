#ifndef __INCLUDE_GUARD_B416C75E_5006_4FC1_9E52_06D4CC2331DF
#define __INCLUDE_GUARD_B416C75E_5006_4FC1_9E52_06D4CC2331DF
#ifdef _MSC_VER
	#pragma once
#endif

#include "defs.h"

#pragma region Structures on 4 components (float4 and int4)

#define _DEF_BIN_OP4(_OP)                                                      \
	const t_this operator _OP (const t_this &_v) const                         \
	{                                                                          \
		return t_this(x _OP _v.x, y _OP _v.y, z _OP _v.z, w _OP _v.w);         \
	}                                                                          \
	t_this& operator _OP##= (const t_this &_v)                                 \
	{                                                                          \
		x _OP##= _v.x; y _OP##= _v.y; z _OP##= _v.z; w _OP##= _v.w;            \
		return *this;                                                          \
	}

#define _DEF_UNARY_MINUS4                                                      \
	const t_this operator- () const                                            \
	{                                                                          \
		return t_this(-x, -y, -z, -w);                                         \
	}                                                                          \

#define _DEF_CONSTR_AND_ACCESSORS4(_NAME)                                      \
	t_scalar x, y, z, w;	                                                   \
	_NAME() {}                                                                 \
	_NAME(t_scalar _x, t_scalar _y,                                            \
		t_scalar _z, t_scalar _w)                                              \
		: x(_x), y(_y), z(_z), w(_w)                                           \
		{}                                                                     \
	t_scalar& operator[] (int _index)                                          \
	{                                                                          \
		return (reinterpret_cast<t_scalar*>(this))[_index];                    \
	}                                                                          \
	const t_scalar& operator[] (int _index) const                              \
	{                                                                          \
		return (reinterpret_cast<const t_scalar*>(this))[_index];              \
	}                                                                          \
	template<int _i1, int _i2, int _i3, int _i4>                               \
	const t_this shuffle() const                                               \
	{                                                                          \
		return t_this((*this)[_i1], (*this)[_i2], (*this)[_i3], (*this)[_i4]); \
	}

#define _DEF_REP4                                                              \
	static t_this rep(t_scalar _v) { return t_this(_v, _v, _v, _v); }

#define _DEF_CMPOP(_OP)                                                        \
	const t_cmpResult operator _OP (const t_this& _val) const                  \
	{                                                                          \
		return t_cmpResult(                                                    \
			x _OP _val.x ? (t_cmpResult::t_scalar)-1 : 0,                      \
			y _OP _val.y ? (t_cmpResult::t_scalar)-1 : 0,                      \
			z _OP _val.z ? (t_cmpResult::t_scalar)-1 : 0,                      \
			w _OP _val.w ? (t_cmpResult::t_scalar)-1 : 0                       \
		);                                                                     \
	}

#define _DEF_LOGOP(_OP, _TARG)                                                 \
t_this& operator _OP##= (const _TARG &_v)                                      \
{                                                                              \
	*(uint *)&x _OP##= *(uint*)&_v.x;                                          \
	*(uint *)&y _OP##= *(uint*)&_v.y;                                          \
	*(uint *)&z _OP##= *(uint*)&_v.z;                                          \
	*(uint *)&w _OP##= *(uint*)&_v.w;                                          \
                                                                               \
	return *this;                                                              \
}                                                                              \
const t_this operator _OP (const _TARG &_v) const                              \
{                                                                              \
	t_this ret = *this;                                                        \
	return ret _OP##= _v;                                                      \
}

#define _DEF_LOGOP_SYM(_OP, _TARG)	  										   \
friend t_this operator _OP (const _TARG &_v, const t_this &_t)                 \
{                                                                              \
	t_this ret = _t;                                                           \
	return ret _OP##= _v;                                                      \
}

struct Point;
struct Vector;

//A 4 component int32 value
struct int4
{
	typedef int4 t_this;
	typedef int t_scalar;
	typedef int4 t_cmpResult;

	//Constructors: default and int4(x, y, z, w)
	//Can also access the object using the [] operator
	_DEF_CONSTR_AND_ACCESSORS4(int4)

	//Bitwise operations with results from comparison. Very convenient for implementing
	//	per-component conditionals in the form (a _OP_ b ? a : b).
	_DEF_LOGOP(&, int4);
	_DEF_LOGOP(|, int4);
	_DEF_LOGOP(^, int4);

	const t_this operator~() const
	{
		int4 ret = *this;
		ret.x = ~ret.x;
		ret.y = ~ret.y;
		ret.z = ~ret.z;
		ret.w = ~ret.w;

		return ret;
	}

	//Returns a mask containing the sign bit of each component
	int getMask() const
	{
		return (((uint)x >> 31) << 3) | (((uint)y >> 31) << 2) | (((uint)z >> 31) << 1) | ((uint)w >> 31);
	}
};

//The float4 class can be used for color and vector calculations
struct float4
{
	typedef float4 t_this;
	typedef float t_scalar;
	typedef int4 t_cmpResult;

	//Constructors: default and float4(x, y, z, w)
	//Can also access the object using the [] operator
	_DEF_CONSTR_AND_ACCESSORS4(float4)

	//Creates a float4 from a point, by setting the .w component to 1
	float4(const Point&);

	//Creates a float4 from a vector, by setting the .w component to 0
	float4(const Vector&);

	//float4 supports per-component +, -, * and /. Thus float4(1, 2, 3, 4) * float4(2, 2, 2, 2) gives float4(2, 4, 6, 8)
	_DEF_BIN_OP4(+);
	_DEF_BIN_OP4(-);
	_DEF_BIN_OP4(*);
	_DEF_BIN_OP4(/);

	//Per-component comparison operations. Return int4. For each component, the return value
	//	is 0 if the condition holds and -1 otherwise. You can use the getMask to see the result
	//	in a more compact form
	_DEF_CMPOP(<);
	_DEF_CMPOP(<=);
	_DEF_CMPOP(>);
	_DEF_CMPOP(>=);
	_DEF_CMPOP(==);
	_DEF_CMPOP(!=);

	//Bitwise operations with results from comparison. Very convenient for implementing
	//	per-component conditionals in the form (a _OP_ b ? a : b).
	_DEF_LOGOP(&, int4);
	_DEF_LOGOP_SYM(&, int4);
	_DEF_LOGOP(&, float4);

	_DEF_LOGOP(|, int4);
	_DEF_LOGOP_SYM(|, int4);
	_DEF_LOGOP(|, float4);

	_DEF_LOGOP(^, int4);
	_DEF_LOGOP_SYM(^, int4);
	_DEF_LOGOP(^, float4);


	//An unary minus
	_DEF_UNARY_MINUS4;

	//A replication function. You can use float4::rep(4.f) as a shortcut to float4(4.f, 4.f, 4.f, 4.f)
	_DEF_REP4;

	//4 component dot product
	float dot(const float4& _v)
	{
		return x * _v.x + y * _v.y + z * _v.z + w * _v.w;
	}

	//Cross product on the first three components. The .w component of the result has no meaning
	const float4 cross(const float4& _v) const
	{
		return shuffle<1, 2, 0, 3>() * _v.shuffle<2, 0, 1, 3>()
			- shuffle<2, 0, 1, 3>() * _v.shuffle<1, 2, 0, 3>();
	}

	//A component-wise minimum between two float4s
	static const float4 min(const float4 & _v1, const float4 & _v2)
	{
		return float4(
			std::min(_v1.x, _v2.x), std::min(_v1.y, _v2.y),
			std::min(_v1.z, _v2.z), std::min(_v1.w, _v2.w)
			);
	}

	//A component-wise maximum between two float4s
	static const float4 max(const float4 & _v1, const float4 & _v2)
	{
		return float4(
			std::max(_v1.x, _v2.x), std::max(_v1.y, _v2.y),
			std::max(_v1.z, _v2.z), std::max(_v1.w, _v2.w)
			);
	}
};

#pragma endregion


#pragma region Geometry primitives (Point and Vector)
#define _DEF_BIN_OP3(_OP, _T_ARG)                                              \
	const t_this operator _OP (const _T_ARG &_v) const                         \
	{                                                                          \
		return t_this(x _OP _v.x, y _OP _v.y, z _OP _v.z);                     \
	}                                                                          \
                                                                               \
	t_this& operator _OP##= (const _T_ARG &_v)                                 \
	{                                                                          \
		x _OP##= _v.x; y _OP##= _v.y; z _OP##= _v.z;                           \
		return *this;                                                          \
	}

#define _DEF_BIN_OP3_SYM(_OP, _T_ARG)                                          \
	friend const t_this operator _OP (const _T_ARG &_v1,                       \
		const t_this &_v2)                                                     \
	{                                                                          \
		return                                                                 \
			t_this(_v1.x _OP _v2.x, _v1.y _OP _v2.y, _v1.z _OP _v2.z);         \
	}

#define _DEF_SCALAR_OP3(_OP, _TARG)                                            \
	const t_this operator _OP (_TARG _v) const                                 \
	{                                                                          \
		return t_this(x _OP _v, y _OP _v, z _OP _v);                           \
	}                                                                          \
	t_this& operator _OP##= (const _TARG _v)                                   \
	{                                                                          \
		x _OP##= _v; y _OP##= _v; z _OP##= _v;                                 \
		return *this;                                                          \
	}

#define _DEF_SCALAR_OP3_SYM(_OP, _TARG)                                        \
	friend const t_this operator _OP (_TARG _v1, t_this _v2)                   \
	{                                                                          \
		return                                                                 \
			t_this(_v1 _OP _v2.x, _v1 _OP _v2.y, _v1 _OP _v2.z);               \
	}                                                                          \


#define _DEF_CONSTR_AND_ACCESSORS3(_NAME)                                      \
	t_scalar x, y, z;	                                                       \
	_NAME() {}                                                                 \
	_NAME(t_scalar _x, t_scalar _y, t_scalar _z)                               \
		: x(_x), y(_y), z(_z)                                                  \
		{}                                                                     \
	t_scalar& operator[] (int _index)                                          \
	{                                                                          \
		return (reinterpret_cast<t_scalar*>(this))[_index];                    \
	}                                                                          \
	const t_scalar& operator[] (int _index) const                              \
	{                                                                          \
		return (reinterpret_cast<const t_scalar*>(this))[_index];              \
	}                                                                          \

#define _DEF_UNARY_MINUS3                                                      \
	const t_this operator- () const                                            \
	{                                                                          \
		return t_this(-x, -y, -z);                                             \
	}                                                                          \

//A class representing a 3D vector in space
struct Vector
{
	typedef Vector t_this;
	typedef float t_scalar;

	//Constructors: default and Vector(x, y, z)
	//Can also access the Vector using the [] operator
	_DEF_CONSTR_AND_ACCESSORS3(Vector);

	_DEF_BIN_OP3(+, Vector); //Vector + Vector -> Vector and Vector += Vector
	_DEF_BIN_OP3(-, Vector); //Vector - Vector -> Vector and Vector -= Vector


	_DEF_SCALAR_OP3(*, float); //Vector * scalar -> Vector and Vector *= scalar
	_DEF_SCALAR_OP3_SYM(*, float); //scalar * Vector -> Vector
	_DEF_SCALAR_OP3(/, float); //Vector / scalar and Vector /= scalar

	//Unary minus
	_DEF_UNARY_MINUS3;

	//Dot product
	float operator* (const Vector& _v) const
	{
		return x * _v.x + y * _v.y + z * _v.z;
	}

	//Cross product
	const Vector operator %(const Vector& _v) const
	{
		float4 t = float4(*this).cross(float4(_v));
		return Vector(t.x, t.y, t.z);
	}

	//Length of the vector
	const float len() const
	{
		return sqrtf(*this * *this);
	}

	//A normalized vector: V / |V|
	const Vector operator ~() const
	{
		return *this / len();
	}
};

//A class representing a 3D point in space
struct Point
{
	typedef Point t_this;
	typedef float t_scalar;

	//Constructors: default and Point(x, y, z)
	//Can also access the Point using the [] operator
	_DEF_CONSTR_AND_ACCESSORS3(Point);

	_DEF_BIN_OP3(+, Vector); //Point + Vector -> Point and Point += Vector
	_DEF_BIN_OP3_SYM(+, Vector); //Vector + Point -> Point
	_DEF_BIN_OP3(-, Vector); //Point - Vector -> Point and Point -= Vector

	//Unary minus
	_DEF_UNARY_MINUS3;

	//Point - Point -> Vector
	const Vector operator- (const Point& _v) const
	{
		return Vector(x - _v.x, y - _v.y, z - _v.z);
	}

	//Linear interpolation between the points _v1 and _v2: _v2 + (_v1 - _v2) * coef
	static const Point lerp (const Point &_v1, const Point &_v2, float _coef)
	{
		return Point(
			_v1.x * _coef + _v2.x * (1 - _coef),
			_v1.y * _coef + _v2.y * (1 - _coef),
			_v1.z * _coef + _v2.z * (1 - _coef)
			);
	}

	const Point lerp (const Point &_v2, float _coef) const
	{
		return Point::lerp(*this, _v2, _coef);
	}

	static const Point lerp(const Point &_v1, const Point &_v2, const Point &_v3, float _u, float _v)
	{
		return Point(
			_v1.x * _u + _v2.x * _v + _v3.x * (1 - _u - _v),
			_v1.y * _u + _v2.y * _v + _v3.y * (1 - _u - _v),
			_v1.z * _u + _v2.z * _v + _v3.z * (1 - _u - _v)
			);
	}
};

inline float4::float4(const Point& _v) {x = _v.x; y = _v.y; z = _v.z; w = 1;}
inline float4::float4(const Vector& _v) {x = _v.x; y = _v.y; z = _v.z; w = 0;}

#pragma endregion

#define _DEF_SCALAR_OP2(_OP, _TARG)                                            \
	const t_this operator _OP (_TARG _v) const                                 \
{                                                                              \
	return t_this(x _OP _v, y _OP _v);                                         \
}                                                                              \
	t_this& operator _OP##= (const _TARG _v)                                   \
{                                                                              \
	x _OP##= _v; y _OP##= _v;                                                  \
	return *this;                                                              \
}

#define _DEF_SCALAR_OP2_SYM(_OP, _TARG)                                        \
	friend const t_this operator _OP (_TARG _v1, t_this _v2)                   \
{                                                                              \
	return                                                                     \
	t_this(_v1 _OP _v2.x, _v1 _OP _v2.y);                                      \
}                                                                              \

#define _DEF_BIN_OP2(_OP, _T_ARG)                                              \
	const t_this operator _OP (const _T_ARG &_v) const                         \
{                                                                              \
	return t_this(x _OP _v.x, y _OP _v.y);                                     \
}                                                                              \
	                                                                           \
	t_this& operator _OP##= (const _T_ARG &_v)                                 \
{                                                                              \
	x _OP##= _v.x; y _OP##= _v.y;                                              \
	return *this;                                                              \
}

#define _DEF_BIN_OP2_SYM(_OP, _T_ARG)                                          \
	friend const t_this operator _OP (const _T_ARG &_v1,                       \
	const t_this &_v2)                                                         \
{                                                                              \
	return                                                                     \
	t_this(_v1.x _OP _v2.x, _v1.y _OP _v2.y);                                  \
}

struct float2
{
	float x, y;
	float2(){}
	float2(float _x, float _y) : x(_x), y(_y) {}

	typedef float2 t_this;
	typedef float t_scalar;

	_DEF_BIN_OP2(+, float2);
	_DEF_BIN_OP2(-, float2);
	_DEF_BIN_OP2(*, float2);
	_DEF_BIN_OP2(/, float2);

	_DEF_SCALAR_OP2(*, float);
	_DEF_SCALAR_OP2_SYM(*, float);
	_DEF_SCALAR_OP2(/, float);
	_DEF_SCALAR_OP2_SYM(/, float);
};


#endif //__INCLUDE_GUARD_B416C75E_5006_4FC1_9E52_06D4CC2331DF
