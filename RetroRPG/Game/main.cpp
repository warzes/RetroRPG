#include "Window.h"
#include "DrawingContext.h"
#include "Renderer.h"
#include "Rasterizer.h"
#include "ImageCache.h"
#include "MeshCache.h"
#include "Timing.h"
#include "Light.h"
#include "BmpFile.h"
//https://github.com/iikare/picoPNG
//https://github.com/skywind3000/mini3d/blob/master/mini3d.c
//https://github.com/search?l=C&q=software+renderer&type=Repositories
//https://github.com/glouw/gel
//https://github.com/kondrak/dos3d
//
//https://www.tk421.net/wizardry/wiz1manual.shtml
//https://gamefaqs.gamespot.com/nes/563479-wizardry-proving-grounds-of-the-mad-overlord/faqs/39644
//https://gamefaqs.gamespot.com/nes/563479-wizardry-proving-grounds-of-the-mad-overlord/faqs/51998
//https://gamefaqs.gamespot.com/nes/563479-wizardry-proving-grounds-of-the-mad-overlord/faqs/42047
//стиль
//https ://github.com/TheFakeMontyOnTheRun/dungeons-of-noudar
//https://menacingmecha.itch.io/isle-of-dreamers
//-----------------------------------------------------------------------------
#pragma comment( lib, "winmm.lib" )
//-----------------------------------------------------------------------------
int main(
	[[maybe_unused]] int   argc,
	[[maybe_unused]] char* argv[])
{
	Window window("Game", 1024, 768);
	Draw draw(window.GetContext(), 1024, 768);
	Renderer renderer(1024, 768);
	Rasterizer rasterizer(1024, 768);

	// Load the assets (textures then 3D models)
	gImageCache.LoadDirectory("../assets");
	meshCache.LoadDirectory("../assets");

	Mesh* crate = meshCache.GetMeshFromName("crate.obj");

	//Create three lights
	Light light1(LIGHT_DIRECTIONAL, Color::RGB(0xFF4040));
	Light light2(LIGHT_DIRECTIONAL, Color::RGB(0x4040FF));
	Light light3(LIGHT_AMBIENT, Color::RGB(0x404040));

	light1.axis = Axis(Vertex(), Vertex(1.0f, 0.0f, -1.0f));
	light2.axis = Axis(Vertex(), Vertex(-1.0f, 0.0f, -1.0f));

	/** Set the renderer properties */
	renderer.SetFog(true);
	renderer.SetViewPosition(Vertex(0.0f, 0.0f, 4.0f));
	renderer.UpdateViewMatrix();
	rasterizer.background = Color::RGB(0xC0C0FF);

	/** Initialize the timing */
	timing.Setup(60);
	//timing.Setup(1200);
	timing.FirstFrame();

	// Program main loop
	while( window.Running && window.Visible )
	{
		window.Update();

		/** Wait for next frame */
		timing.WaitNextFrame();

		/** Copy render frame to window context */
		draw.SetPixels(rasterizer.GetPixels());

		/** Update model transforms */
		crate->angle += Vertex(0.1f, 2.0f, 0.0f);
		crate->UpdateMatrix();

		/** Light model */
		Light::Black(crate);
		light1.Shine(crate);
		light2.Shine(crate);
		light3.Shine(crate);

		/** Render the 3D model */
		renderer.Render(crate);

		/** Draw the triangles */
		rasterizer.Flush();
		rasterizer.RasterList(renderer.GetTriangleList());
		renderer.Flush();
	}

	//BmpFile screenshot("screenshot.bmp");
	//screenshot.Save(&rasterizer.frame);

	timing.LastFrame();
}
//-----------------------------------------------------------------------------