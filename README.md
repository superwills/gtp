gtp
===

Graphics Test Project

## Vector occluders technique
<img src="http://i.imgur.com/FyJetdS.png" width="320" height="240" /> <img src="http://i.imgur.com/vFpWgoz.png" width="320" height="240" />

## Raytracing
<img src="http://i.imgur.com/EwGS8wU.png" width="320" height="240" /> <img src="http://i.imgur.com/Fgd32aR.png" width="320" height="240" />

## Radiosity
<img src="http://i.imgur.com/wbedFBJ.png" width="320" height="240" /> <img src="http://i.imgur.com/pXIE4Gy.png" width="320" height="240" />

## Spherical harmonics
<img src="http://i.imgur.com/ndBG7tU.png" width="320" height="240" /> <img src="http://i.imgur.com/h3BBvb2.png" width="320" height="240" />

# SETUP

In order to compile, you will need to perform the following steps.

# [D3DX11](https://www.microsoft.com/en-ca/download/details.aspx?id=6812)

To set up the D3DX11 (`<D3DX11.h>`, `d3dx11.lib` files), you have to download the ^ linked file, and install it.

  * Open Visual Studio, and right click the `gtp` project name. Go to __Project properties / VC++ Directories / Include Directories__: `C:\Program Files %28x86%29\Microsoft DirectX SDK %28June 2010%29\Include`
  * Go to __Project properties / VC++ Directories / Library Directories__: `C:\Program Files %28x86%29\Microsoft DirectX SDK %28June 2010%29\Lib\x64`
    * NOTE: If you're building x86, then you have to link the x86 folder, not x64

# [Eigen](https://gitlab.com/libeigen/eigen/)
  * Download the ^ linked zip file, then extract it in `C:\Libraries` or wherever you'd like to park it on your machine
  * Then, in Visual Studio go to __Project properties / VC++ Directories / Include Directories__: Add `C:\Libraries\eigen-X.X.X` (where eigen-X.X.X is the version you downloaded!) to the list

# [RapidJson](https://github.com/Tencent/rapidjson)
  * Download the ^ linked zip file, then extract it in C:\Libraries or something like that
  * Then, in Visual Studio go to __Project properties / VC++ Directories / Include Directories__: Add `C:\Libraries\rapidjson-master\include` to the list
  
* Remember to maintain your Debug/Release configs separately -- they are different!



