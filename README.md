# mm_mint_pic
mm_mint_pic is an images viewer/re-scaler and can export them to different file formats

## Description

![Product Name Screen Shot][product-screenshot]
<div style="text-align: justify">
mm_mint_pic is designed to display modern formats images in GEM windows.
It lets you transform images by scaling or rotating them.
It works in 1bpp, 4bpp (no screen cropping yet) and 8bpp planar modes, as well as 16bpp, 24bpp and 32bpp for large screen configurations.
Note that the best performance is achieved in 32bpp mode.
TIFF and HEIF containers are supported.
You can export the current image as raw data (Xaaes MFD format) so that the current image size and screen format are preserved.
Another feature is the ability to crop the part of the desktop you want by simply drawing a box on the screen.
The control bar is rebuilt on the image respecting the alpha channel of png icons according to window size and scrolling.
You can change the icons to suit your needs by simply replacing the files under the icon directory.
No shared library or module is required to run it.
</div>

## Getting Started

### Build Dependencies

* GEMLib
* Pth-2.0.7
* PNGLib
* ZLib
* LibYuv
* LibHeif (min v1.15)
* Libde265 HEVC
* x265 HEVC Encoder
* LibWebp
* LibJpeg

You should found most of these libraries here: https://tho-otto.de/crossmint.php or you can read https://www.atari-forum.com/viewforum.php?f=70&sid=3d5a35722383f636d46de67fa33c5b57 if you want to rebuild them.

### Installing

* "make" command will produce bin/mm_pic.prg.
* This repository contains sample icons but you should replace them with yours (32bpp / 24px or just adjust your struct_st_ico_png_list array for other sizes).

### Executing program

* Drag&Drop images file on the binary generated
* Right click in the window will show/hide the icons who contains classic functions like
    * Open
    * Save as
    * 1:1 or automatic resizing
    * Reload
    * Resize
    * Crop desktop
    * Rotate (by 90°/180°/270°)
    * Zoom In
    * Zoom Out

## Help

Notice that the ico and rsc folders must be present in the bin directory.
Due to the lack of performance on Atari Planar mode some features like alpha transparency are disabled by defaut on planar mode.
Best performances are obtained in 32bpp screen mode.

## Authors

Medour Mehdi
[@M.Medour](www.linkedin.com/in/mehdi-medour-2968b3b2)

## Version History

* 0.1
    * Initial Release

## License

This project is licensed under the GNU GENERAL PUBLIC LICENSE License - see the LICENSE.md file for details

## Acknowledgments

Inspiration, code snippets, etc.
* [AtariForum](https://www.atari-forum.com)
* [T.Otto](https://tho-otto.de/crossmint.php)
* [Myaes](http://myaes.lutece.net/)
* [FreeMint](https://freemint.github.io/)

[product-screenshot]: screenshot.png
