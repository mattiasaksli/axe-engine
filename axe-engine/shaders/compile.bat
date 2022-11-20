@echo off

for %%f in (*.vert *.tesc *.tese *.geom *.frag *.comp) do (
	echo Compiling %%f
	%VULKAN_SDK%\Bin\glslc %%f -o %%f.spv)

echo:
echo Finished
echo:

pause