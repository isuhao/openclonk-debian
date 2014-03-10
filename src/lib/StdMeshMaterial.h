/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2009-2013, The OpenClonk Team and contributors
 *
 * Distributed under the terms of the ISC license; see accompanying file
 * "COPYING" for details.
 *
 * "Clonk" is a registered trademark of Matthes Bender, used with permission.
 * See accompanying file "TRADEMARK" for details.
 *
 * To redistribute this file separately, substitute the full license texts
 * for the above references.
 */

#ifndef INC_StdMeshMaterial
#define INC_StdMeshMaterial

#include <StdBuf.h>
#include <C4Surface.h>

#include <vector>
#include <map>

// TODO: Support more features of OGRE material scripts
// Refer to http://www.ogre3d.org/docs/manual/manual_14.html

class StdMeshMaterialParserCtx;

class StdMeshMaterialError: public std::exception
{
public:
	StdMeshMaterialError(const StdStrBuf& message, const char* file, unsigned int line);
	virtual ~StdMeshMaterialError() throw() {}

	virtual const char* what() const throw() { return Buf.getData(); }

protected:
	StdCopyStrBuf Buf;
};

// Interface to load textures. Given a texture filename occuring in the
// material script, this should load the texture from wherever the material
// script is actually loaded, for example from a C4Group.
class StdMeshMaterialTextureLoader
{
public:
	virtual C4Surface* LoadTexture(const char* filename) = 0;
	virtual ~StdMeshMaterialTextureLoader() {}
};

class StdMeshMaterialTextureUnit
{
public:
	enum TexAddressModeType
	{
		AM_Wrap,
		AM_Clamp,
		AM_Mirror,
		AM_Border
	};

	enum FilteringType
	{
		F_None,
		F_Point,
		F_Linear,
		F_Anisotropic
	};

	enum BlendOpType
	{
		BO_Replace,
		BO_Add,
		BO_Modulate,
		BO_AlphaBlend
	};

	enum BlendOpExType
	{
		BOX_Source1,
		BOX_Source2,
		BOX_Modulate,
		BOX_ModulateX2,
		BOX_ModulateX4,
		BOX_Add,
		BOX_AddSigned,
		BOX_AddSmooth,
		BOX_Subtract,
		BOX_BlendDiffuseAlpha,
		BOX_BlendTextureAlpha,
		BOX_BlendCurrentAlpha,
		BOX_BlendManual,
		BOX_Dotproduct,
		BOX_BlendDiffuseColor
	};

	enum BlendOpSourceType
	{
		BOS_Current,
		BOS_Texture,
		BOS_Diffuse,
		BOS_Specular,
		BOS_PlayerColor, // not specified in ogre, added in OpenClonk
		BOS_Manual
	};

	struct Transformation
	{
		enum Type
		{
			T_SCROLL,
			T_SCROLL_ANIM,
			T_ROTATE,
			T_ROTATE_ANIM,
			T_SCALE,
			T_TRANSFORM,
			T_WAVE_XFORM
		};

		enum XFormType
		{
			XF_SCROLL_X,
			XF_SCROLL_Y,
			XF_ROTATE,
			XF_SCALE_X,
			XF_SCALE_Y
		};

		enum WaveType
		{
			W_SINE,
			W_TRIANGLE,
			W_SQUARE,
			W_SAWTOOTH,
			W_INVERSE_SAWTOOTH
		};

		Type TransformType;

		union
		{
			struct { float X; float Y; } Scroll;
			struct { float XSpeed; float YSpeed; } ScrollAnim;
			struct { float Angle; } Rotate;
			struct { float RevsPerSec; } RotateAnim;
			struct { float X; float Y; } Scale;
			struct { float M[16]; } Transform;
			struct { XFormType XForm; WaveType Wave; float Base; float Frequency; float Phase; float Amplitude; } WaveXForm;
		};

		double GetScrollX(double t) const { assert(TransformType == T_SCROLL_ANIM); return ScrollAnim.XSpeed * t; }
		double GetScrollY(double t) const { assert(TransformType == T_SCROLL_ANIM); return ScrollAnim.YSpeed * t; }
		double GetRotate(double t) const { assert(TransformType == T_ROTATE_ANIM); return fmod(RotateAnim.RevsPerSec * t, 1.0) * 360.0; }
		double GetWaveXForm(double t) const;
	};

	// Ref-counted texture. When a meterial inherits from one which contains
	// a TextureUnit, then they will share the same C4TexRef.
	class Tex
	{
	public:
		Tex(C4Surface* Surface); // Takes ownership
		~Tex();

		unsigned int RefCount;

		// TODO: Note this cannot be C4Surface here, because C4Surface
		// does not have a virtual destructor, so we couldn't delete it
		// properly in that case. I am a bit annoyed that this
		// currently requires a cross-ref to lib/texture. I think
		// C4Surface should go away and the file loading/saving
		// should be free functions instead. I also think the file
		// loading/saving should be decoupled from the surfaces, so we
		// can skip the surface here and simply use a C4TexRef. armin.
		C4Surface* Surf;
		C4TexRef& Texture;
	};

	// Simple wrapper which handles refcounting of Tex
	class TexPtr
	{
	public:
		TexPtr(C4Surface* Surface);
		TexPtr(const TexPtr& other);
		~TexPtr();
		
		TexPtr& operator=(const TexPtr& other);
		
		Tex* pTex;
	};

	StdMeshMaterialTextureUnit();

	void LoadTexture(StdMeshMaterialParserCtx& ctx, const char* texname);
	void Load(StdMeshMaterialParserCtx& ctx);

	bool HasTexture() const { return !Textures.empty(); }
	size_t GetNumTextures() const { return Textures.size(); }
	const C4TexRef& GetTexture(unsigned int i) const { return Textures[i].pTex->Texture; }
	bool HasFrameAnimation() const { return Duration > 0; }
	bool HasTexCoordAnimation() const { return !Transformations.empty(); }

	StdCopyStrBuf Name;
	float Duration; // Duration of texture animation, if any.

	TexAddressModeType TexAddressMode;
	float TexBorderColor[4];
	FilteringType Filtering[3]; // min, max, mipmap

	BlendOpExType ColorOpEx;
	BlendOpSourceType ColorOpSources[2];
	float ColorOpManualFactor;
	float ColorOpManualColor1[3];
	float ColorOpManualColor2[3];

	BlendOpExType AlphaOpEx;
	BlendOpSourceType AlphaOpSources[2];
	float AlphaOpManualFactor;
	float AlphaOpManualAlpha1;
	float AlphaOpManualAlpha2;

	// Transformations to be applied to texture coordinates in order
	std::vector<Transformation> Transformations;

private:
	std::vector<TexPtr> Textures;
};

class StdMeshMaterialPass
{
public:
	enum CullHardwareType
	{
		CH_Clockwise,
		CH_CounterClockwise,
		CH_None
	};

	enum SceneBlendType
	{
		SB_One,
		SB_Zero,
		SB_DestColor,
		SB_SrcColor,
		SB_OneMinusDestColor,
		SB_OneMinusSrcColor,
		SB_DestAlpha,
		SB_SrcAlpha,
		SB_OneMinusDestAlpha,
		SB_OneMinusSrcAlpha
	};

	StdMeshMaterialPass();
	void Load(StdMeshMaterialParserCtx& ctx);

	bool IsOpaque() const { return SceneBlendFactors[1] == SB_Zero; }

	StdCopyStrBuf Name;
	std::vector<StdMeshMaterialTextureUnit> TextureUnits;

	float Ambient[4];
	float Diffuse[4];
	float Specular[4];
	float Emissive[4];
	float Shininess;

	bool DepthCheck;
	bool DepthWrite;

	CullHardwareType CullHardware;
	SceneBlendType SceneBlendFactors[2];
	bool AlphaToCoverage;

	// An abstract shader class. This is supposed to be implemented by the
	// GFX implementation, such as C4DrawGL.
	class Shader { public: virtual ~Shader() {} };

	// This is a simple reference to a shader. It is allowed to be copied as long
	// as the shader is not set.
	class ShaderRef
	{
	public:
		ShaderRef(): Program(NULL) {}
		ShaderRef(const ShaderRef& other) { Program = NULL; /* don't copy the program pointer */ }
		~ShaderRef() { delete Program; }

		ShaderRef& operator=(Shader* NewProgram) { assert(Program == NULL); Program = NewProgram; return *this; }
		ShaderRef& operator=(const ShaderRef& other) { assert(Program == NULL); assert(other.Program == NULL); Program = NULL; return *this; }

		const Shader* operator->() const { return Program; }
		const Shader& operator*() const { return *Program; }
		operator const Shader*() const { return Program; }

		Shader* Program;
	};

	// A compiled shader which applies the blending between the texture units,
	// and also applies color modulation and MOD2.
	// The actual compilation is being done in PrepareMaterial() of the C4Draw.
	ShaderRef Program;
};

class StdMeshMaterialTechnique
{
public:
	StdMeshMaterialTechnique();

	void Load(StdMeshMaterialParserCtx& ctx);

	bool IsOpaque() const;

	StdCopyStrBuf Name;
	std::vector<StdMeshMaterialPass> Passes;

	// Filled in by gfx implementation: Whether this technique is available on
	// the hardware and gfx engine (DX/GL) we are running on
	bool Available;
};

class StdMeshMaterial
{
public:
	StdMeshMaterial();
	void Load(StdMeshMaterialParserCtx& ctx);

	bool IsOpaque() const { assert(BestTechniqueIndex >= 0); return Techniques[BestTechniqueIndex].IsOpaque(); }

	// Location the Material was loaded from
	StdCopyStrBuf FileName;
	unsigned int Line;

	// Material name
	StdCopyStrBuf Name;

	// Not currently used in Clonk, but don't fail when we see this in a
	// Material script:
	bool ReceiveShadows;

	// Available techniques
	std::vector<StdMeshMaterialTechnique> Techniques;

	// Filled in by gfx implementation: Best technique to use
	int BestTechniqueIndex; // Don't use a pointer into the Technique vector to save us from implementing a copyctor
};

class StdMeshMatManager
{
	friend class StdMeshMaterialUpdate;
private:
	typedef std::map<StdCopyStrBuf, StdMeshMaterial> MaterialMap;

public:
	class Iterator
	{
		friend class StdMeshMatManager;
	public:
		Iterator(const MaterialMap::iterator& iter): iter_(iter) {}
		Iterator(const Iterator& iter): iter_(iter.iter_) {}

		Iterator operator=(const Iterator& iter) { iter_ = iter.iter_; return *this; }
		Iterator& operator++() { ++iter_; return *this; }
		bool operator==(const Iterator& other) const { return iter_ == other.iter_; }
		bool operator!=(const Iterator& other) const { return iter_ != other.iter_; }

		const StdMeshMaterial& operator*() const { return iter_->second; }
		const StdMeshMaterial* operator->() const { return &iter_->second; }
	private:
		MaterialMap::iterator iter_;
	};

	// Remove all materials from manager. Make sure there is no StdMesh
	// referencing any out there before calling this.
	void Clear();

	// Parse a material script file, and add the materials to the manager.
	// filename may be NULL if the source is not a file. It will only be used
	// for error messages.
	// Throws StdMeshMaterialError.
	void Parse(const char* mat_script, const char* filename, StdMeshMaterialTextureLoader& tex_loader);

	// Get material by name. NULL if there is no such material with this name.
	const StdMeshMaterial* GetMaterial(const char* material_name) const;

	Iterator Begin() { return Iterator(Materials.begin()); }
	Iterator End() { return Iterator(Materials.end()); }
	Iterator Remove(const Iterator& iter, class StdMeshMaterialUpdate* update);

private:
	MaterialMap Materials;
};

extern StdMeshMatManager MeshMaterialManager;

#endif
