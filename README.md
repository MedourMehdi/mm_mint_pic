# mm_mint_pic
mm_mint_pic is an images viewer/re-scaler and can export them to different file formats

## Description

![IMAGE MMPIC ScreenShot][mmpic-image-screenshot]
![VIDEO MMPIC ScreenShot][mmpic-video-screenshot]
![AUDIO MMPIC ScreenShot][mmpic-audio-screenshot]
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

## Supported formats

### Read & Write
* Degas PI1/PI3
* JPEG 
* PNG
* TIFF
* TARGA
* BMP
* WEBP
* HEIF/HEIC
* PSD (RGB 24/32bpp)

### Read
* PDF (Password not managed yet)
* SVG
* GIF (Images & Video)
* FLIC (Video)
* PSD (RGB 24/32bpp)

### Write
* MFD (Screen memory RAW format)

### Audio/Video with FFMPEG
* All formats listed [here](https://github.com/MedourMehdi/mm_mint_pic/blob/main/vid_ffmpeg/vid_ffmpeg.cpp#L29)

Notice that it's actually in testing, i.e. there's no A/V Sync support actually implemented.

## Getting Started

### Build Dependencies

* GEMLib
* Pth-2.0.7
* PNGLib
* ZLib
* LibYuv
* LibHeif
* Libde265 HEVC
* x265 HEVC Encoder
* LibWebp
* LibJpeg
* GifLib
* [Xpdf](https://github.com/MedourMehdi/xpdf)
* [Psd_Sdk](https://github.com/MolecularMatters/psd_sdk)

For audio/video build you'll need ffmpeg package.

You should found most of these libraries here: https://tho-otto.de/crossmint.php or you can read https://www.atari-forum.com/viewforum.php?f=70 if you want to rebuild them.

### Installing

* "make" command will produce bin/mm_pic.prg
* Edit the Makefile if you want to enable/disable features
    * video : WITH_FFMPEG 
    * audio : WITH_FFMPEG & WITH_FFMPEG_SOUND
* This repository contains sample icons but you should replace them with yours (32bpp / 24px or just adjust your struct_st_ico_png_list array for other sizes)

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
* 0.9.1
    * MMPic can now read [Recoil's supported formats](https://recoil.sourceforge.net/formats.html)
* 0.9
    * Audio player support via ffmpeg libraries (LibAV) 
* 0.8
    * Video player support via ffmpeg libraries (LibAV) 
* 0.7.5.3
    * Read and write support for PSD pictures (Libpsd 0.9 replaced by Psd_Sdk)
    * Grayscaling for control bar icons in 4bpp planar mode
* 0.7
    * Added support for PSD files (It uses LibPsd 0.9)
    * Support icons Drag&Drop
    * Friendly app name set in the desktop menu
    * Respond correctly to AP_TERM & AP_RESCHG messages    
* 0.6
    * Video engine implemented
    * Multiple processes logic implemented (via pseudo pthread functions)
    * New formats supported (read):
        * GIF
        * FLIC
        * WEBP (Animated)
* 0.5
    * New format supported (read):
        * PDF
* 0.4
    * New format supported (read):
        * SVG
* 0.3
    * New formats supported:
        * Degas PI1
        * Degas PI3
* 0.1
    * Formats supported:
        * HEIF
        * TIFF
        * PNG
        * JPEG
        * WEBP
        * MFD

## License

This project is licensed under the GNU GENERAL PUBLIC LICENSE License - see the LICENSE.md file for details

## Acknowledgments

Inspiration, code snippets, etc.
* [AtariForum](https://www.atari-forum.com)
* [T.Otto](https://tho-otto.de/crossmint.php)
* [L.Pursell](https://atari.gfabasic.net/htm/imgview.htm)
* [Myaes](http://myaes.lutece.net/)
* [FreeMint](https://freemint.github.io/)

[mmpic-image-screenshot]: screenshot.png
[mmpic-video-screenshot]: screenshot_video.png
[mmpic-audio-screenshot]: screenshot_audio.png