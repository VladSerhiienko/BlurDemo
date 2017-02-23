$input v_dir

/*
 * Copyright 2014-2016 Dario Manesku. All rights reserved.
 * License: https://github.com/bkaradzic/bgfx#license-bsd-2-clause
 */
 
#include "common.sh"
#include "uniforms.sh"

SAMPLERCUBE(s_texCube, 0);
SAMPLERCUBE(s_texCubeIrr, 1);

void main()
{
	vec3 dir = normalize(v_dir);
	//gl_FragColor = vec4(toFilmic(dir), 1.0);
	//return;

	vec4 color;
	//color = toLinear(textureCube(s_texCubeIrr, dir));
	#if 1
	if (u_bgType == 7.0)
	{
		color = toLinear(textureCube(s_texCubeIrr, dir));
	}
	else
	{
		float lod = u_bgType;
		dir = fixCubeLookup(dir, lod, 256.0);
		color = toLinear(textureCubeLod(s_texCube, dir, lod));
	}
	color *= exp2(u_exposure);
	#endif

	gl_FragColor = toFilmic(color);
}
