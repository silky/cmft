/*
 * Copyright 2014 Dario Manesku. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#include "cmft/image.h"

#include "base/config.h"
#include "base/utils.h"
#include "base/macros.h"
#include "cubemaputils.h"
#include "messages.h"

#include <bx/uint32_t.h>

#include <string.h>
#include <stdarg.h>

namespace cmft
{
    // Texture format string.
    //-----

    static const char* s_textureFormatStr[TextureFormat::Count] =
    {
        "BGR8",      //BGR8
        "RGB8",      //RGB8
        "RGB16",     //RGB16
        "RGB16F",    //RGB16F
        "RGB32F",    //RGB32F
        "RGBE",      //RGBE
        "BGRA8",     //BGRA8
        "RGBA8",     //RGBA8
        "RGBA16",    //RGBA16
        "RGBA16F",   //RGBA16F
        "RGBA32F",   //RGBA32F
        "<unknown>", //Unknown
    };

    const char* getTextureFormatStr(TextureFormat::Enum _format)
    {
        DEBUG_CHECK(_format < TextureFormat::Count, "Reading array out of bounds!");
        return s_textureFormatStr[uint8_t(_format)];
    }

    // Image file type extension.
    //-----

    static const char* s_imageFileTypeExtension[ImageFileType::Count] =
    {
        ".dds", //DDS
        ".ktx", //KTX
        ".tga", //TGA
        ".hdr", //HDR
    };

    const char* getFilenameExtensionStr(ImageFileType::Enum _ft)
    {
        DEBUG_CHECK(_ft < ImageFileType::Count, "Reading array out of bounds!");
        return s_imageFileTypeExtension[uint8_t(_ft)];
    }

    // Image file type name.
    //-----

    static const char* s_imageFileTypeName[ImageFileType::Count] =
    {
        "DDS", //DDS
        "KTX", //KTX
        "TGA", //TGA
        "HDR", //HDR
    };

    const char* getFileTypeStr(ImageFileType::Enum _ft)
    {
        DEBUG_CHECK(_ft < ImageFileType::Count, "Reading array out of bounds!");
        return s_imageFileTypeName[uint8_t(_ft)];
    }

    // Valid formats.
    //-----

#define TEXTURE_FORMAT_NULL TextureFormat::Enum(-1)

    static const TextureFormat::Enum s_ddsValidFormats[] =
    {
        TextureFormat::BGR8,
        TextureFormat::BGRA8,
        TextureFormat::RGBA16,
        TextureFormat::RGBA16F,
        TextureFormat::RGBA32F,
        TEXTURE_FORMAT_NULL
    };

    static const TextureFormat::Enum s_ktxValidFormats[] =
    {
        TextureFormat::RGB8,
        TextureFormat::RGB16,
        TextureFormat::RGB16F,
        TextureFormat::RGB32F,
        TextureFormat::RGBA8,
        TextureFormat::RGBA16,
        TextureFormat::RGBA16F,
        TextureFormat::RGBA32F,
        TEXTURE_FORMAT_NULL
    };

    static const TextureFormat::Enum s_tgaValidFormats[] =
    {
        TextureFormat::BGR8,
        TextureFormat::BGRA8,
        TEXTURE_FORMAT_NULL
    };

    static const TextureFormat::Enum s_hdrValidFormats[] =
    {
        TextureFormat::RGBE,
        TEXTURE_FORMAT_NULL
    };

    const TextureFormat::Enum* getValidTextureFormats(ImageFileType::Enum _fileType)
    {
        if (ImageFileType::DDS == _fileType)
        {
            return s_ddsValidFormats;
        }
        else if (ImageFileType::KTX == _fileType)
        {
            return s_ktxValidFormats;
        }
        else if (ImageFileType::TGA == _fileType)
        {
            return s_tgaValidFormats;
        }
        else if (ImageFileType::HDR == _fileType)
        {
            return s_hdrValidFormats;
        }
        else
        {
            return NULL;
        }
    }

    void getValidTextureFormatsStr(char* _str, ImageFileType::Enum _fileType)
    {
        const TextureFormat::Enum* validFormatsList = getValidTextureFormats(_fileType);
        if (NULL == validFormatsList)
        {
            _str = NULL;
            return;
        }

        uint8_t ii = 0;
        TextureFormat::Enum curr;
        if (TEXTURE_FORMAT_NULL != (curr = validFormatsList[ii++]))
        {
            strcpy(_str, getTextureFormatStr(curr));
        }
        while (TEXTURE_FORMAT_NULL != (curr = validFormatsList[ii++]))
        {
            strcat(_str, " ");
            strcat(_str, getTextureFormatStr(curr));
        }
    }

    static bool contains(TextureFormat::Enum _format, const TextureFormat::Enum* _formatList)
    {
        TextureFormat::Enum curr;
        uint8_t ii = 0;
        while (TEXTURE_FORMAT_NULL != (curr = _formatList[ii++]))
        {
            if (curr == _format)
            {
                return true;
            }
        }

        return false;
    };

    bool checkValidInternalFormat(ImageFileType::Enum _fileType, TextureFormat::Enum _internalFormat)
    {
        if (ImageFileType::DDS == _fileType)
        {
            return contains(_internalFormat, s_ddsValidFormats);
        }
        else if (ImageFileType::KTX == _fileType)
        {
            return contains(_internalFormat, s_ktxValidFormats);
        }
        else if (ImageFileType::TGA == _fileType)
        {
            return contains(_internalFormat, s_tgaValidFormats);
        }
        else if (ImageFileType::HDR == _fileType)
        {
            return contains(_internalFormat, s_hdrValidFormats);
        }

        return false;
    }

    // Image data info.
    //-----

    struct PixelDataType
    {
        enum Enum
        {
            UINT8,
            UINT16,
            UINT32,
            HALF_FLOAT,
            FLOAT,

            Count,
        };
    };

    static const ImageDataInfo s_imageDataInfo[TextureFormat::Count] =
    {
        {  3, 3, 0, PixelDataType::UINT8       }, //BGR8
        {  3, 3, 0, PixelDataType::UINT8       }, //RGB8
        {  6, 3, 0, PixelDataType::UINT16      }, //RGB16
        {  6, 3, 0, PixelDataType::HALF_FLOAT  }, //RGB16F
        { 12, 3, 0, PixelDataType::FLOAT       }, //RGB32F
        {  4, 4, 0, PixelDataType::UINT8       }, //RGBE
        {  4, 4, 1, PixelDataType::UINT8       }, //BGRA8
        {  4, 4, 1, PixelDataType::UINT8       }, //RGBA8
        {  8, 4, 1, PixelDataType::UINT16      }, //RGBA16
        {  8, 4, 1, PixelDataType::HALF_FLOAT  }, //RGBA16F
        { 16, 4, 1, PixelDataType::FLOAT       }, //RGBA32F
        {  0, 0, 0, 0 }, //Unknown
    };

    const ImageDataInfo& getImageDataInfo(TextureFormat::Enum _format)
    {
        DEBUG_CHECK(_format < TextureFormat::Count, "Reading array out of bounds!");
        return s_imageDataInfo[_format];
    }

    // HDR format.
    //-----

#define HDR_VALID_PROGRAMTYPE 0x01
#define HDR_VALID_GAMMA       0x02
#define HDR_VALID_EXPOSURE    0x04
#define HDR_MAGIC             CMFT_MAKEFOURCC('#','?','R','A')
#define HDR_MAGIC_FULL        "#?RADIANCE"
#define HDR_MAGIC_LEN         10

    struct HdrHeader
    {
        int m_valid;
        float m_gamma;
        float m_exposure;
    };

    // TGA format.
    //-----

#define TGA_HEADER_SIZE 18
#define TGA_ID          { 'T', 'R', 'U', 'E', 'V', 'I', 'S', 'I', 'O', 'N', '-', 'X', 'F', 'I', 'L', 'E', '.', '\0' }
#define TGA_ID_LEN      18
#define TGA_FOOTER_SIZE 26

#define TGA_IT_NOIMAGE     0x0
#define TGA_IT_COLORMAPPED 0x1
#define TGA_IT_RGB         0x2
#define TGA_IT_BW          0x3
#define TGA_IT_RLE         0x8

#define TGA_DESC_HORIZONTAL 0x10
#define TGA_DESC_VERTICAL   0x20

    struct TgaHeader
    {
        uint8_t m_idLength;
        uint8_t m_colorMapType;
        uint8_t m_imageType;
        int16_t m_colorMapOrigin;
        int16_t m_colorMapLength;
        uint8_t m_colorMapDepth;
        int16_t m_xOrigin;
        int16_t m_yOrigin;
        uint16_t m_width;
        uint16_t m_height;
        uint8_t m_bitsPerPixel;
        uint8_t m_imageDescriptor;
    };

    struct TgaFooter
    {
        uint32_t m_extensionOffset;
        uint32_t m_developerOffset;
        uint8_t m_signature[18];
    };

    // DDS format.
    //-----

#define DDS_MAGIC             CMFT_MAKEFOURCC('D', 'D', 'S', ' ')
#define DDS_HEADER_SIZE       124
#define DDS_IMAGE_DATA_OFFSET (DDS_HEADER_SIZE + 4)
#define DDS_PIXELFORMAT_SIZE  32
#define DDS_DX10_HEADER_SIZE  20

#define DDS_DXT1 CMFT_MAKEFOURCC('D', 'X', 'T', '1')
#define DDS_DXT2 CMFT_MAKEFOURCC('D', 'X', 'T', '2')
#define DDS_DXT3 CMFT_MAKEFOURCC('D', 'X', 'T', '3')
#define DDS_DXT4 CMFT_MAKEFOURCC('D', 'X', 'T', '4')
#define DDS_DXT5 CMFT_MAKEFOURCC('D', 'X', 'T', '5')
#define DDS_ATI1 CMFT_MAKEFOURCC('A', 'T', 'I', '1')
#define DDS_BC4U CMFT_MAKEFOURCC('B', 'C', '4', 'U')
#define DDS_ATI2 CMFT_MAKEFOURCC('A', 'T', 'I', '2')
#define DDS_BC5U CMFT_MAKEFOURCC('B', 'C', '5', 'U')

#define DDS_DX10 CMFT_MAKEFOURCC('D', 'X', '1', '0')

#define D3DFMT_R8G8B8        20
#define D3DFMT_A8R8G8B8      21
#define D3DFMT_X8R8G8B8      22
#define D3DFMT_A8B8G8R8      32
#define D3DFMT_X8B8G8R8      33
#define D3DFMT_A16B16G16R16  36
#define D3DFMT_A16B16G16R16F 113
#define D3DFMT_A32B32G32R32F 116

#define DDSD_CAPS                   0x00000001
#define DDSD_HEIGHT                 0x00000002
#define DDSD_WIDTH                  0x00000004
#define DDSD_PITCH                  0x00000008
#define DDSD_PIXELFORMAT            0x00001000
#define DDSD_MIPMAPCOUNT            0x00020000
#define DDSD_LINEARSIZE             0x00080000
#define DDSD_DEPTH                  0x00800000

#define DDPF_ALPHAPIXELS            0x00000001
#define DDPF_ALPHA                  0x00000002
#define DDPF_FOURCC                 0x00000004
#define DDPF_INDEXED                0x00000020
#define DDPF_RGB                    0x00000040
#define DDPF_YUV                    0x00000200
#define DDPF_LUMINANCE              0x00020000
#define DDPF_RGBA                   (DDPF_RGB|DDPF_ALPHAPIXELS)
#define DDPF_LUMINANCEA             (DDPF_LUMINANCE|DDPF_ALPHAPIXELS)
#define DDS_PF_BC_24                0x00100000
#define DDS_PF_BC_32                0x00200000
#define DDS_PF_BC_48                0x00400000

#define DDSCAPS_COMPLEX             0x00000008
#define DDSCAPS_TEXTURE             0x00001000
#define DDSCAPS_MIPMAP              0x00400000

#define DDSCAPS2_CUBEMAP            0x00000200
#define DDSCAPS2_CUBEMAP_POSITIVEX  0x00000400
#define DDSCAPS2_CUBEMAP_NEGATIVEX  0x00000800
#define DDSCAPS2_CUBEMAP_POSITIVEY  0x00001000
#define DDSCAPS2_CUBEMAP_NEGATIVEY  0x00002000
#define DDSCAPS2_CUBEMAP_POSITIVEZ  0x00004000
#define DDSCAPS2_CUBEMAP_NEGATIVEZ  0x00008000

#define DDS_CUBEMAP_ALLFACES ( DDSCAPS2_CUBEMAP_POSITIVEX|DDSCAPS2_CUBEMAP_NEGATIVEX \
                             | DDSCAPS2_CUBEMAP_POSITIVEY|DDSCAPS2_CUBEMAP_NEGATIVEY \
                             | DDSCAPS2_CUBEMAP_POSITIVEZ|DDSCAPS2_CUBEMAP_NEGATIVEZ )

#define DDSCAPS2_VOLUME             0x00200000

#define DXGI_FORMAT_UNKNOWN             0
#define DXGI_FORMAT_R32G32B32A32_FLOAT  2
#define DXGI_FORMAT_R16G16B16A16_FLOAT  10
#define DXGI_FORMAT_R16G16B16A16_UINT   12
#define DXGI_FORMAT_R8G8B8A8_UNORM      28
#define DXGI_FORMAT_R8G8B8A8_UINT       30
#define DXGI_FORMAT_B8G8R8A8_UNORM      87
#define DXGI_FORMAT_B8G8R8X8_UNORM      88
#define DXGI_FORMAT_B8G8R8A8_TYPELESS   90

#define DDS_DIMENSION_TEXTURE1D 2
#define DDS_DIMENSION_TEXTURE2D 3
#define DDS_DIMENSION_TEXTURE3D 4

#define DDS_ALPHA_MODE_UNKNOWN        0x0
#define DDS_ALPHA_MODE_STRAIGHT       0x1
#define DDS_ALPHA_MODE_PREMULTIPLIED  0x2
#define DDS_ALPHA_MODE_OPAQUE         0x3
#define DDS_ALPHA_MODE_CUSTOM         0x4

#define D3D10_RESOURCE_MISC_GENERATE_MIPS      0x1L
#define D3D10_RESOURCE_MISC_SHARED             0x2L
#define D3D10_RESOURCE_MISC_TEXTURECUBE        0x4L
#define D3D10_RESOURCE_MISC_SHARED_KEYEDMUTEX  0x10L
#define D3D10_RESOURCE_MISC_GDI_COMPATIBLE     0x20L

    struct DdsPixelFormat
    {
        uint32_t m_size;
        uint32_t m_flags;
        uint32_t m_fourcc;
        uint32_t m_rgbBitCount;
        uint32_t m_rBitMask;
        uint32_t m_gBitMask;
        uint32_t m_bBitMask;
        uint32_t m_aBitMask;
    };

    struct DdsHeader
    {
        uint32_t m_size;
        uint32_t m_flags;
        uint32_t m_height;
        uint32_t m_width;
        uint32_t m_pitchOrLinearSize;
        uint32_t m_depth;
        uint32_t m_mipMapCount;
        uint32_t m_reserved1[11];
        DdsPixelFormat m_pixelFormat;
        uint32_t m_caps;
        uint32_t m_caps2;
        uint32_t m_caps3;
        uint32_t m_caps4;
        uint32_t m_reserved2;
    };

    struct DdsHeaderDxt10
    {
        uint32_t m_dxgiFormat;
        uint32_t m_resourceDimension;
        uint32_t m_miscFlags;
        uint32_t m_arraySize;
        uint32_t m_miscFlags2;
    };

    static const DdsPixelFormat s_ddsPixelFormat[] =
    {
        { sizeof(DdsPixelFormat), DDPF_RGB,  D3DFMT_R8G8B8,   24, 0x00ff0000, 0x0000ff00, 0x000000ff, 0x00000000 }, //BGR8
        { sizeof(DdsPixelFormat), DDPF_RGBA, D3DFMT_A8B8G8R8, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000 }, //BGRA8
        { sizeof(DdsPixelFormat), DDPF_FOURCC, DDS_DX10,  64, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000 }, //RGBA16
        { sizeof(DdsPixelFormat), DDPF_FOURCC, DDS_DX10,  64, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000 }, //RGBA16F
        { sizeof(DdsPixelFormat), DDPF_FOURCC, DDS_DX10, 128, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000 }, //RGBA32F
    };

    static inline const DdsPixelFormat& getDdsPixelFormat(TextureFormat::Enum _format)
    {
        DEBUG_CHECK(checkValidInternalFormat(ImageFileType::DDS, _format), "Not a valid DDS texture format!");
        if      (TextureFormat::BGR8    == _format) { return s_ddsPixelFormat[0];  }
        else if (TextureFormat::BGRA8   == _format) { return s_ddsPixelFormat[1];  }
        else if (TextureFormat::RGBA16  == _format) { return s_ddsPixelFormat[2];  }
        else if (TextureFormat::RGBA16F == _format) { return s_ddsPixelFormat[3];  }
        else/*(TextureFormat::RGBA32F == _format)*/ { return s_ddsPixelFormat[4];  }
    }

    static inline uint8_t getDdsDxgiFormat(TextureFormat::Enum _format)
    {
        if      (TextureFormat::RGBA16  == _format) { return DXGI_FORMAT_R16G16B16A16_UINT;  }
        else if (TextureFormat::RGBA16F == _format) { return DXGI_FORMAT_R16G16B16A16_FLOAT; }
        else if (TextureFormat::RGBA32F == _format) { return DXGI_FORMAT_R32G32B32A32_FLOAT; }
        else { return DXGI_FORMAT_UNKNOWN; }
    }

    static const struct TranslateDdsPfBitCount
    {
        uint32_t m_bitCount;
        uint32_t m_flag;
    } s_translateDdsPfBitCount[] =
    {
        { 24,  DDS_PF_BC_24 },
        { 32,  DDS_PF_BC_32 },
        { 48,  DDS_PF_BC_48 },
    };

    static const struct TranslateDdsFormat
    {
        uint32_t m_format;
        TextureFormat::Enum m_textureFormat;

    } s_translateDdsFormat[] =
    {
        { D3DFMT_R8G8B8,          TextureFormat::BGR8    },
        { D3DFMT_A8R8G8B8,        TextureFormat::BGRA8   },
        { D3DFMT_A16B16G16R16,    TextureFormat::RGBA16  },
        { D3DFMT_A16B16G16R16F,   TextureFormat::RGBA16F },
        { D3DFMT_A32B32G32R32F,   TextureFormat::RGBA32F },
        { DDS_PF_BC_24|DDPF_RGB,  TextureFormat::BGR8    },
        { DDS_PF_BC_32|DDPF_RGBA, TextureFormat::BGRA8   },
        { DDS_PF_BC_48|DDPF_RGB,  TextureFormat::RGB16   },
    };

    static const struct TranslateDdsDxgiFormat
    {
        uint8_t m_dxgiFormat;
        TextureFormat::Enum m_textureFormat;

    } s_translateDdsDxgiFormat[] =
    {
        { DXGI_FORMAT_R16G16B16A16_UINT,  TextureFormat::RGBA16  },
        { DXGI_FORMAT_R16G16B16A16_FLOAT, TextureFormat::RGBA16F },
        { DXGI_FORMAT_R32G32B32A32_FLOAT, TextureFormat::RGBA32F },
    };

    // KTX format.
    //-----

#define KTX_MAGIC             { 0xAB, 0x4B, 0x54, 0x58, 0x20, 0x31, 0x31, 0xBB, 0x0D, 0x0A, 0x1A, 0x0A }
#define KTX_MAGIC_SHORT       0x58544BAB
#define KTX_MAGIC_LEN         12
#define KTX_ENDIAN_REF        0x04030201
#define KTX_ENDIAN_REF_REV    0x01020304
#define KTX_HEADER_SIZE       52
#define KTX_UNPACK_ALIGNMENT  4

// GL data type.
#define GL_BYTE             0x1400
#define GL_UNSIGNED_BYTE    0x1401
#define GL_SHORT            0x1402
#define GL_UNSIGNED_SHORT   0x1403
#define GL_INT              0x1404
#define GL_UNSIGNED_INT     0x1405
#define GL_FLOAT            0x1406
#define GL_HALF_FLOAT       0x140B
#define GL_FIXED            0x140C

// GL pixel format.
#define GL_RGB              0x1907
#define GL_RGBA             0x1908

#define GL_RGBA32F          0x8814
#define GL_RGB32F           0x8815
#define GL_RGBA16F          0x881A
#define GL_RGB16F           0x881B
#define GL_RGBA32UI         0x8D70
#define GL_RGB32UI          0x8D71
#define GL_RGBA16UI         0x8D76
#define GL_RGB16UI          0x8D77
#define GL_RGBA8UI          0x8D7C
#define GL_RGB8UI           0x8D7D
#define GL_RGBA32I          0x8D82
#define GL_RGB32I           0x8D83
#define GL_RGBA16I          0x8D88
#define GL_RGB16I           0x8D89
#define GL_RGBA8I           0x8D8E
#define GL_RGB8I            0x8D8F

    struct KtxHeader
    {
        uint32_t m_endianness;
        uint32_t m_glType;
        uint32_t m_glTypeSize;
        uint32_t m_glFormat;
        uint32_t m_glInternalFormat;
        uint32_t m_glBaseInternalFormat;
        uint32_t m_pixelWidth;
        uint32_t m_pixelHeight;
        uint32_t m_pixelDepth;
        uint32_t m_numArrayElements;
        uint32_t m_numFaces;
        uint32_t m_numMips;
        uint32_t m_bytesKeyValue;
    };

    struct GlSizedInternalFormat
    {
        uint32_t m_glInternalFormat;
        uint32_t m_glFormat;
    };

    static const GlSizedInternalFormat s_glSizedInternalFormats[TextureFormat::Count] =
    {
        { 0, 0 }, //BGR8
        { GL_RGB8UI,   GL_RGB  }, //RGB8
        { GL_RGB16UI,  GL_RGB  }, //RGB16
        { GL_RGB16F,   GL_RGB  }, //RGB16F
        { GL_RGB32F,   GL_RGB  }, //RGB32F
        { 0, 0 }, //RGBE
        { 0, 0 }, //BGRA8
        { GL_RGBA8UI,  GL_RGBA }, //RGBA8
        { GL_RGBA16UI, GL_RGBA }, //RGBA16
        { GL_RGBA16F,  GL_RGBA }, //RGBA16F
        { GL_RGBA32F,  GL_RGBA }, //RGBA32F
        { 0, 0 }, //Unknown
    };

    static const GlSizedInternalFormat& getGlSizedInternalFormat(TextureFormat::Enum _format)
    {
        DEBUG_CHECK(_format < TextureFormat::Count, "Reading array out of bounds!");
        return s_glSizedInternalFormats[_format];
    }

    static const uint32_t s_pixelDataTypeToGlType[PixelDataType::Count] =
    {
        GL_UNSIGNED_BYTE,  // UINT8
        GL_UNSIGNED_SHORT, // UINT16
        GL_UNSIGNED_INT,   // UINT32
        GL_HALF_FLOAT,     // HALF_FLOAT
        GL_FLOAT,          // FLOAT
    };

    static uint32_t pixelDataTypeToGlType(PixelDataType::Enum _pdt)
    {
        DEBUG_CHECK(_pdt < PixelDataType::Count, "Reading array out of bounds!");
        return s_pixelDataTypeToGlType[_pdt];
    }

    static const struct TranslateKtxFormat
    {
        uint32_t m_glInternalFormat;
        TextureFormat::Enum m_textureFormat;

    } s_translateKtxFormat[] =
    {
        { GL_RGB,      TextureFormat::RGB8    },
        { GL_RGB8UI,   TextureFormat::RGB8    },
        { GL_RGB16UI,  TextureFormat::RGB16   },
        { GL_RGB16F,   TextureFormat::RGB16F  },
        { GL_RGB32F,   TextureFormat::RGB32F  },
        { GL_RGBA,     TextureFormat::RGBA8   },
        { GL_RGBA8UI,  TextureFormat::RGBA8   },
        { GL_RGBA16UI, TextureFormat::RGBA16  },
        { GL_RGBA16F,  TextureFormat::RGBA16F },
        { GL_RGBA32F,  TextureFormat::RGBA32F },
    };

    // Image -> format headers/footers.
    //-----

    // Notice: creates proper dds headers only for uncompressed formats.
    void ddsHeaderFromImage(DdsHeader& _ddsHeader, DdsHeaderDxt10* _ddsHeaderDxt10, const Image& _image)
    {
        const DdsPixelFormat& ddsPixelFormat = getDdsPixelFormat(_image.m_format);

        const uint32_t bytesPerPixel = getImageDataInfo(_image.m_format).m_bytesPerPixel;
        const bool hasMipMaps = _image.m_numMips > 1;
        const bool hasMultipleFaces = _image.m_numFaces > 0;
        const bool isCubemap = _image.m_numFaces == 6;

        if (DDS_DX10 == ddsPixelFormat.m_fourcc)
        {
            if (NULL != _ddsHeaderDxt10)
            {
                _ddsHeaderDxt10->m_dxgiFormat = getDdsDxgiFormat(_image.m_format);
                DEBUG_CHECK(0 != _ddsHeaderDxt10->m_dxgiFormat, "Dxt10 format should not be 0.");
                _ddsHeaderDxt10->m_resourceDimension = DDS_DIMENSION_TEXTURE2D;
                _ddsHeaderDxt10->m_miscFlags = isCubemap ? D3D10_RESOURCE_MISC_TEXTURECUBE : 0;
                _ddsHeaderDxt10->m_arraySize = 1;
                _ddsHeaderDxt10->m_miscFlags2 = 0;
            }
            else
            {
                WARN("Dds header dxt10 is required but it is NULL.");
            }
        }

        memset(&_ddsHeader, 0, sizeof(DdsHeader));
        _ddsHeader.m_size = DDS_HEADER_SIZE;
        _ddsHeader.m_flags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT
                           | (hasMipMaps ? DDSD_MIPMAPCOUNT : 0)
                           | DDSD_PITCH
                           ;
        _ddsHeader.m_height = _image.m_height;
        _ddsHeader.m_width = _image.m_width;
        _ddsHeader.m_pitchOrLinearSize = _image.m_width * bytesPerPixel;
        _ddsHeader.m_mipMapCount = _image.m_numMips;
        memcpy(&_ddsHeader.m_pixelFormat, &ddsPixelFormat, sizeof(DdsPixelFormat));
        _ddsHeader.m_caps = DDSCAPS_TEXTURE
                          | (hasMipMaps ? DDSCAPS_MIPMAP : 0)
                          | ((hasMipMaps | hasMultipleFaces) ? DDSCAPS_COMPLEX : 0)
                          ;
        _ddsHeader.m_caps2 = (isCubemap ? DDSCAPS2_CUBEMAP | DDS_CUBEMAP_ALLFACES : 0);
    }

    void printDdsHeader(const DdsHeader& _ddsHeader)
    {
        printf("ddsHeader.m_size                      = %u\n"
               "ddsHeader.m_flags                     = %u\n"
               "ddsHeader.m_height                    = %u\n"
               "ddsHeader.m_width                     = %u\n"
               "ddsHeader.m_pitchOrLinearSize         = %u\n"
               "ddsHeader.m_depth                     = %u\n"
               "ddsHeader.m_mipMapCount               = %u\n"
               "ddsHeader.m_reserved1[0]              = %u\n"
               "ddsHeader.m_pixelFormat.m_size        = %u\n"
               "ddsHeader.m_pixelFormat.m_flags       = %u\n"
               "ddsHeader.m_pixelFormat.m_fourcc      = %u\n"
               "ddsHeader.m_pixelFormat.m_rgbBitCount = %u\n"
               "ddsHeader.m_pixelFormat.m_rBitMask    = %u\n"
               "ddsHeader.m_pixelFormat.m_gBitMask    = %u\n"
               "ddsHeader.m_pixelFormat.m_bBitMask    = %u\n"
               "ddsHeader.m_pixelFormat.m_aBitMask    = %u\n"
               "ddsHeader.m_caps                      = %u\n"
               "ddsHeader.m_caps2                     = %u\n"
               "ddsHeader.m_caps3                     = %u\n"
               "ddsHeader.m_caps4                     = %u\n"
               "ddsHeader.m_reserved2                 = %u\n"
               , _ddsHeader.m_size
               , _ddsHeader.m_flags
               , _ddsHeader.m_height
               , _ddsHeader.m_width
               , _ddsHeader.m_pitchOrLinearSize
               , _ddsHeader.m_depth
               , _ddsHeader.m_mipMapCount
               , _ddsHeader.m_reserved1[0]
               , _ddsHeader.m_pixelFormat.m_size
               , _ddsHeader.m_pixelFormat.m_flags
               , _ddsHeader.m_pixelFormat.m_fourcc
               , _ddsHeader.m_pixelFormat.m_rgbBitCount
               , _ddsHeader.m_pixelFormat.m_rBitMask
               , _ddsHeader.m_pixelFormat.m_gBitMask
               , _ddsHeader.m_pixelFormat.m_bBitMask
               , _ddsHeader.m_pixelFormat.m_aBitMask
               , _ddsHeader.m_caps
               , _ddsHeader.m_caps2
               , _ddsHeader.m_caps3
               , _ddsHeader.m_caps4
               , _ddsHeader.m_reserved2
               );
    }

    void printDdsHeaderDxt10(const DdsHeaderDxt10& _ddsHeaderDxt10)
    {
        printf("ddsHeaderDxt10.m_dxgiFormat        = %u\n"
               "ddsHeaderDxt10.m_resourceDimension = %u\n"
               "ddsHeaderDxt10.m_miscFlags         = %u\n"
               "ddsHeaderDxt10.m_arraySize         = %u\n"
               "ddsHeaderDxt10.m_miscFlags2        = %u\n"
               , _ddsHeaderDxt10.m_dxgiFormat
               , _ddsHeaderDxt10.m_resourceDimension
               , _ddsHeaderDxt10.m_miscFlags
               , _ddsHeaderDxt10.m_arraySize
               , _ddsHeaderDxt10.m_miscFlags2
               );
    }

    void ktxHeaderFromImage(KtxHeader& _ktxHeader, const Image& _image)
    {
        const ImageDataInfo& imageDataInfo = getImageDataInfo(_image.m_format);

        _ktxHeader.m_endianness = KTX_ENDIAN_REF;
        _ktxHeader.m_glType = pixelDataTypeToGlType((PixelDataType::Enum)imageDataInfo.m_pixelType);
        _ktxHeader.m_glTypeSize = (imageDataInfo.m_bytesPerPixel/imageDataInfo.m_numChanels);
        _ktxHeader.m_glFormat = getGlSizedInternalFormat(_image.m_format).m_glFormat;
        _ktxHeader.m_glInternalFormat = getGlSizedInternalFormat(_image.m_format).m_glInternalFormat;
        _ktxHeader.m_glBaseInternalFormat = _ktxHeader.m_glFormat;
        _ktxHeader.m_pixelWidth = _image.m_width;
        _ktxHeader.m_pixelHeight = _image.m_height;
        _ktxHeader.m_pixelDepth = 0;
        _ktxHeader.m_numArrayElements = 0;
        _ktxHeader.m_numFaces = _image.m_numFaces;
        _ktxHeader.m_numMips = _image.m_numMips;
        _ktxHeader.m_bytesKeyValue = 0;

        DEBUG_CHECK(_ktxHeader.m_glTypeSize == 1
                 || _ktxHeader.m_glTypeSize == 2
                 || _ktxHeader.m_glTypeSize == 4
                  , "Invalid ktx header glTypeSize."
                  );
        DEBUG_CHECK(0 != _image.m_numMips, "Mips count cannot be 0.");
    }

    void printKtxHeader(const KtxHeader& _ktxHeader)
    {
        printf("ktxHeader.m_endianness       = %u\n"
               "ktxHeader.m_glType           = %u\n"
               "ktxHeader.m_glTypeSize       = %u\n"
               "ktxHeader.m_glFormat         = %u\n"
               "ktxHeader.m_glInternalFormat = %u\n"
               "ktxHeader.m_glBaseInternal   = %u\n"
               "ktxHeader.m_pixelWidth       = %u\n"
               "ktxHeader.m_pixelHeight      = %u\n"
               "ktxHeader.m_pixelDepth       = %u\n"
               "ktxHeader.m_numArrayElements = %u\n"
               "ktxHeader.m_numFaces         = %u\n"
               "ktxHeader.m_numMips          = %u\n"
               "ktxHeader.m_bytesKeyValue    = %u\n"
               , _ktxHeader.m_endianness
               , _ktxHeader.m_glType
               , _ktxHeader.m_glTypeSize
               , _ktxHeader.m_glFormat
               , _ktxHeader.m_glInternalFormat
               , _ktxHeader.m_glBaseInternalFormat
               , _ktxHeader.m_pixelWidth
               , _ktxHeader.m_pixelHeight
               , _ktxHeader.m_pixelDepth
               , _ktxHeader.m_numArrayElements
               , _ktxHeader.m_numFaces
               , _ktxHeader.m_numMips
               , _ktxHeader.m_bytesKeyValue
               );
    }

    void tgaHeaderFromImage(TgaHeader& _tgaHeader, const Image& _image)
    {
        memset(&_tgaHeader, 0, sizeof(TgaHeader));
        _tgaHeader.m_idLength = 0;
        _tgaHeader.m_colorMapType = 0;
        _tgaHeader.m_imageType = TGA_IT_RGB;
        _tgaHeader.m_xOrigin = 0;
        _tgaHeader.m_yOrigin = 0;
        _tgaHeader.m_width = uint16_t(_image.m_width);
        _tgaHeader.m_height = uint16_t(_image.m_height);
        _tgaHeader.m_bitsPerPixel = getImageDataInfo(_image.m_format).m_bytesPerPixel*8;
        _tgaHeader.m_imageDescriptor = (getImageDataInfo(_image.m_format).m_hasAlpha ? 0x8 : 0x0);
    }

    void printTgaHeader(const TgaHeader& _tgaHeader)
    {
        printf("tgaHeader.m_idLength        = %u\n"
               "tgaHeader.m_colorMapType    = %u\n"
               "tgaHeader.m_imageType       = %u\n"
               "tgaHeader.m_colorMapOrigin  = %d\n"
               "tgaHeader.m_colorMapLength  = %d\n"
               "tgaHeader.m_colorMapDepth   = %u\n"
               "tgaHeader.m_xOrigin         = %d\n"
               "tgaHeader.m_yOrigin         = %d\n"
               "tgaHeader.m_width           = %u\n"
               "tgaHeader.m_height          = %u\n"
               "tgaHeader.m_bitsPerPixel    = %u\n"
               "tgaHeader.m_imageDescriptor = %u\n"
               , _tgaHeader.m_idLength
               , _tgaHeader.m_colorMapType
               , _tgaHeader.m_imageType
               , _tgaHeader.m_colorMapOrigin
               , _tgaHeader.m_colorMapLength
               , _tgaHeader.m_colorMapDepth
               , _tgaHeader.m_xOrigin
               , _tgaHeader.m_yOrigin
               , _tgaHeader.m_width
               , _tgaHeader.m_height
               , _tgaHeader.m_bitsPerPixel
               , _tgaHeader.m_imageDescriptor
               );
    }

    void hdrHeaderFromImage(HdrHeader& _hdrHeader, const Image& _image)
    {
        BX_UNUSED(_image);

        memset(&_hdrHeader, 0, sizeof(HdrHeader));
        _hdrHeader.m_valid = HDR_VALID_GAMMA | HDR_VALID_EXPOSURE;
        _hdrHeader.m_gamma = 1.0f;
        _hdrHeader.m_exposure = 1.0f;
    }

    void printHdrHeader(const HdrHeader& _hdrHeader)
    {
        printf("hdrHeader.m_valid    = %d\n"
               "hdrHeader.m_gamma    = %f\n"
               "hdrHeader.m_exposure = %f\n"
               , _hdrHeader.m_valid
               , _hdrHeader.m_gamma
               , _hdrHeader.m_exposure
               );
    }

    // Image.
    //-----

    void imageUnload(Image& _image)
    {
        if (_image.m_data)
        {
            free(_image.m_data);
            _image.m_data = NULL;
        }
    }

    void imageRef(Image& _dst, const Image& _src)
    {
        _dst.m_data     = _src.m_data;
        _dst.m_width    = _src.m_width;
        _dst.m_height   = _src.m_height;
        _dst.m_dataSize = _src.m_dataSize;
        _dst.m_format   = _src.m_format;
        _dst.m_numMips  = _src.m_numMips;
        _dst.m_numFaces = _src.m_numFaces;
    }

    void imageMove(Image& _dst, Image& _src)
    {
        imageUnload(_dst);
        imageRef(_dst, _src);
        _src.m_data = NULL;
    }

    void imageCopy(Image& _dst, const Image& _src)
    {
        imageUnload(_dst);

        _dst.m_data = malloc(_src.m_dataSize);
        MALLOC_CHECK(_dst.m_data);
        memcpy(_dst.m_data, _src.m_data, _src.m_dataSize);
        _dst.m_width    = _src.m_width;
        _dst.m_height   = _src.m_height;
        _dst.m_dataSize = _src.m_dataSize;
        _dst.m_format   = _src.m_format;
        _dst.m_numMips  = _src.m_numMips;
        _dst.m_numFaces = _src.m_numFaces;
    }

    uint32_t imageGetNumPixels(const Image& _image)
    {
        DEBUG_CHECK(0 != _image.m_numMips, "Mips count cannot be 0.");

        uint32_t count = 0;
        for (uint8_t mip = 0; mip < _image.m_numMips; ++mip)
        {
            const uint32_t width  = max(UINT32_C(1), _image.m_width  >> mip);
            const uint32_t height = max(UINT32_C(1), _image.m_height >> mip);
            count += width * height;
        }
        count *= _image.m_numFaces;

        return count;
    }

    void imageGetMipOffsets(uint32_t _offsets[CUBE_FACE_NUM][MAX_MIP_NUM], const Image& _image)
    {
        const uint32_t bytesPerPixel = getImageDataInfo(_image.m_format).m_bytesPerPixel;

        uint32_t offset = 0;
        for (uint8_t face = 0; face < _image.m_numFaces; ++face)
        {
            for (uint8_t mip = 0; mip < _image.m_numMips; ++mip)
            {
                _offsets[face][mip] = offset;

                const uint32_t width  = max(UINT32_C(1), _image.m_width  >> mip);
                const uint32_t height = max(UINT32_C(1), _image.m_height >> mip);
                offset += width * height * bytesPerPixel;
            }
        }
    }

    void imageGetFaceOffsets(uint32_t _faceOffsets[CUBE_FACE_NUM], const Image& _image)
    {
        const uint32_t bytesPerPixel = getImageDataInfo(_image.m_format).m_bytesPerPixel;

        uint32_t offset = 0;
        for (uint8_t face = 0; face < _image.m_numFaces; ++face)
        {
            _faceOffsets[face] = offset;

            for (uint8_t mip = 0; mip < _image.m_numMips; ++mip)
            {
                const uint32_t width  = max(UINT32_C(1), _image.m_width  >> mip);
                const uint32_t height = max(UINT32_C(1), _image.m_height >> mip);
                offset += width * height * bytesPerPixel;
            }
        }
    }

    // To rgba32f.
    //-----

    inline void bgr8ToRgba32f(float* _rgba32f, const uint8_t* _bgr8)
    {
        _rgba32f[0] = float(_bgr8[2]) * (1.0f/255.0f);
        _rgba32f[1] = float(_bgr8[1]) * (1.0f/255.0f);
        _rgba32f[2] = float(_bgr8[0]) * (1.0f/255.0f);
        _rgba32f[3] = 1.0f;
    }

    inline void bgra8ToRgba32f(float* _rgba32f, const uint8_t* _bgra8)
    {
        _rgba32f[0] = float(_bgra8[2]) * (1.0f/255.0f);
        _rgba32f[1] = float(_bgra8[1]) * (1.0f/255.0f);
        _rgba32f[2] = float(_bgra8[0]) * (1.0f/255.0f);
        _rgba32f[3] = float(_bgra8[3]) * (1.0f/255.0f);
    }

    inline void rgb8ToRgba32f(float* _rgba32f, const uint8_t* _rgb8)
    {
        _rgba32f[0] = float(_rgb8[0]) * (1.0f/255.0f);
        _rgba32f[1] = float(_rgb8[1]) * (1.0f/255.0f);
        _rgba32f[2] = float(_rgb8[2]) * (1.0f/255.0f);
        _rgba32f[3] = 1.0f;
    }

    inline void rgba8ToRgba32f(float* _rgba32f, const uint8_t* _rgba8)
    {
        _rgba32f[0] = float(_rgba8[0]) * (1.0f/255.0f);
        _rgba32f[1] = float(_rgba8[1]) * (1.0f/255.0f);
        _rgba32f[2] = float(_rgba8[2]) * (1.0f/255.0f);
        _rgba32f[3] = float(_rgba8[3]) * (1.0f/255.0f);
    }

    inline void rgb16ToRgba32f(float* _rgba32f, const uint16_t* _rgb16)
    {
        _rgba32f[0] = float(_rgb16[0]) * (1.0f/65535.0f);
        _rgba32f[1] = float(_rgb16[1]) * (1.0f/65535.0f);
        _rgba32f[2] = float(_rgb16[2]) * (1.0f/65535.0f);
        _rgba32f[3] = 1.0f;
    }

    inline void rgba16ToRgba32f(float* _rgba32f, const uint16_t* _rgba16)
    {
        _rgba32f[0] = float(_rgba16[0]) * (1.0f/65535.0f);
        _rgba32f[1] = float(_rgba16[1]) * (1.0f/65535.0f);
        _rgba32f[2] = float(_rgba16[2]) * (1.0f/65535.0f);
        _rgba32f[3] = float(_rgba16[3]) * (1.0f/65535.0f);
    }

    inline void rgb16fToRgba32f(float* _rgba32f, const uint16_t* _rgb16f)
    {
        _rgba32f[0] = bx::halfToFloat(_rgb16f[0]);
        _rgba32f[1] = bx::halfToFloat(_rgb16f[1]);
        _rgba32f[2] = bx::halfToFloat(_rgb16f[2]);
        _rgba32f[3] = 1.0f;
    }

    inline void rgba16fToRgba32f(float* _rgba32f, const uint16_t* _rgba16f)
    {
        _rgba32f[0] = bx::halfToFloat(_rgba16f[0]);
        _rgba32f[1] = bx::halfToFloat(_rgba16f[1]);
        _rgba32f[2] = bx::halfToFloat(_rgba16f[2]);
        _rgba32f[3] = bx::halfToFloat(_rgba16f[3]);
    }

    inline void rgb32fToRgba32f(float* _rgba32f, const float* _rgb32f)
    {
        _rgba32f[0] = _rgb32f[0];
        _rgba32f[1] = _rgb32f[1];
        _rgba32f[2] = _rgb32f[2];
        _rgba32f[3] = 1.0f;
    }

    inline void rgba32fToRgba32f(float* _dst, const float* _src)
    {
        memcpy(_dst, _src, 4*sizeof(float));
    }

    inline void rgbeToRgba32f(float* _rgba32f, const uint8_t* _rgbe)
    {
        if (_rgbe[3])
        {
            const float exp = ldexp(1.0f, _rgbe[3] - (128+8));
            _rgba32f[0] = float(_rgbe[0]) * exp;
            _rgba32f[1] = float(_rgbe[1]) * exp;
            _rgba32f[2] = float(_rgbe[2]) * exp;
            _rgba32f[3] = 1.0f;
        }
        else
        {
            _rgba32f[0] = 0.0f;
            _rgba32f[1] = 0.0f;
            _rgba32f[2] = 0.0f;
            _rgba32f[3] = 1.0f;
        }
    }

    void toRgba32f(float _rgba32f[4], TextureFormat::Enum _srcFormat, const void* _src)
    {
        switch(_srcFormat)
        {
        case TextureFormat::BGR8:     bgr8ToRgba32f(_rgba32f,     (uint8_t*)_src); break;
        case TextureFormat::RGB8:     rgb8ToRgba32f(_rgba32f,     (uint8_t*)_src); break;
        case TextureFormat::RGB16:    rgb16ToRgba32f(_rgba32f,   (uint16_t*)_src); break;
        case TextureFormat::RGB16F:   rgb16fToRgba32f(_rgba32f,  (uint16_t*)_src); break;
        case TextureFormat::RGB32F:   rgb32fToRgba32f(_rgba32f,     (float*)_src); break;
        case TextureFormat::RGBE:     rgbeToRgba32f(_rgba32f,     (uint8_t*)_src); break;
        case TextureFormat::BGRA8:    bgra8ToRgba32f(_rgba32f,    (uint8_t*)_src); break;
        case TextureFormat::RGBA8:    rgba8ToRgba32f(_rgba32f,    (uint8_t*)_src); break;
        case TextureFormat::RGBA16:   rgba16ToRgba32f(_rgba32f,  (uint16_t*)_src); break;
        case TextureFormat::RGBA16F:  rgba16fToRgba32f(_rgba32f, (uint16_t*)_src); break;
        case TextureFormat::RGBA32F:  rgba32fToRgba32f(_rgba32f,    (float*)_src); break;
        default: DEBUG_CHECK(false, "Unknown image format.");
        };
    }

    void imageToRgba32f(Image& _dst, const Image& _src)
    {
        // Alloc dst data.
        const uint32_t pixelCount = imageGetNumPixels(_src);
        const uint8_t dstBytesPerPixel = getImageDataInfo(TextureFormat::RGBA32F).m_bytesPerPixel;
        const uint32_t dataSize = pixelCount*dstBytesPerPixel;
        void* data = malloc(dataSize);
        MALLOC_CHECK(data);

        // Get total number of channels.
        const uint8_t numChannelsPerPixel = getImageDataInfo(TextureFormat::RGBA32F).m_numChanels;
        const uint32_t totalNumChannels = pixelCount*numChannelsPerPixel;

        // Convert each channel.
        float* dst = (float*)data;
        const float* end = (float*)dst + totalNumChannels;
        switch(_src.m_format)
        {
        case TextureFormat::BGR8:
            {
                const uint8_t* src = (const uint8_t*)_src.m_data;

                for (;dst < end; dst+=4, src+=3)
                {
                    bgr8ToRgba32f(dst, src);
                }
            }
        break;

        case TextureFormat::RGB8:
            {
                const uint8_t* src = (const uint8_t*)_src.m_data;

                for (;dst < end; dst+=4, src+=3)
                {
                    rgb8ToRgba32f(dst, src);
                }
            }
        break;

        case TextureFormat::RGB16:
            {
                const uint16_t* src = (const uint16_t*)_src.m_data;

                for (;dst < end; dst+=4, src+=3)
                {
                    rgb16ToRgba32f(dst, src);
                }
            }
        break;

        case TextureFormat::RGB16F:
            {
                const uint16_t* src = (const uint16_t*)_src.m_data;

                for (;dst < end; dst+=4, src+=3)
                {
                    rgb16fToRgba32f(dst, src);
                }
            }
        break;

        case TextureFormat::RGB32F:
            {
                const float* src = (const float*)_src.m_data;

                for (;dst < end; dst+=4, src+=3)
                {
                    rgb32fToRgba32f(dst, src);
                }
            }
        break;

        case TextureFormat::RGBE:
            {
                const uint8_t* src = (const uint8_t*)_src.m_data;

                for (;dst < end; dst+=4, src+=4)
                {
                    rgbeToRgba32f(dst, src);
                }
            }
        break;

        case TextureFormat::BGRA8:
            {
                const uint8_t* src = (const uint8_t*)_src.m_data;

                for (;dst < end; dst+=4, src+=4)
                {
                    bgra8ToRgba32f(dst, src);
                }
            }
        break;

        case TextureFormat::RGBA8:
            {
                const uint8_t* src = (const uint8_t*)_src.m_data;

                for (;dst < end; dst+=4, src+=4)
                {
                    rgba8ToRgba32f(dst, src);
                }
            }
        break;

        case TextureFormat::RGBA16:
            {
                const uint16_t* src = (const uint16_t*)_src.m_data;

                for (;dst < end; dst+=4, src+=4)
                {
                    rgba16ToRgba32f(dst, src);
                }
            }
        break;

        case TextureFormat::RGBA16F:
            {
                const uint16_t* src = (const uint16_t*)_src.m_data;

                for (;dst < end; dst+=4, src+=4)
                {
                    rgba16fToRgba32f(dst, src);
                }
            }
        break;

        case TextureFormat::RGBA32F:
            {
                // Copy data.
                memcpy(data, _src.m_data, dataSize);
            }
        break;

        default:
            {
                DEBUG_CHECK(false, "Unknown image format.");
            }
        break;
        };

        // Fill image structure.
        Image result;
        result.m_data = data;
        result.m_width = _src.m_width;
        result.m_height = _src.m_height;
        result.m_dataSize = dataSize;
        result.m_format = TextureFormat::RGBA32F;
        result.m_numMips = _src.m_numMips;
        result.m_numFaces = _src.m_numFaces;

        // Fill image structure.
        imageMove(_dst, result);
    }

    void imageToRgba32f(Image& _image)
    {
        Image tmp;
        imageToRgba32f(tmp, _image);
        imageMove(_image, tmp);
    }

    // From Rgba32f.
    //-----

    inline void bgr8FromRgba32f(uint8_t* _bgr8, const float* _rgba32f)
    {
        _bgr8[2] = uint8_t(clamp(_rgba32f[0], 0.0f, 1.0f) * 255.0f);
        _bgr8[1] = uint8_t(clamp(_rgba32f[1], 0.0f, 1.0f) * 255.0f);
        _bgr8[0] = uint8_t(clamp(_rgba32f[2], 0.0f, 1.0f) * 255.0f);
    }

    inline void bgra8FromRgba32f(uint8_t* _bgra8, const float* _rgba32f)
    {
        _bgra8[2] = uint8_t(clamp(_rgba32f[0], 0.0f, 1.0f) * 255.0f);
        _bgra8[1] = uint8_t(clamp(_rgba32f[1], 0.0f, 1.0f) * 255.0f);
        _bgra8[0] = uint8_t(clamp(_rgba32f[2], 0.0f, 1.0f) * 255.0f);
        _bgra8[3] = uint8_t(clamp(_rgba32f[3], 0.0f, 1.0f) * 255.0f);
    }

    inline void rgb8FromRgba32f(uint8_t* _rgb8, const float* _rgba32f)
    {
        _rgb8[0] = uint8_t(clamp(_rgba32f[0], 0.0f, 1.0f) * 255.0f);
        _rgb8[1] = uint8_t(clamp(_rgba32f[1], 0.0f, 1.0f) * 255.0f);
        _rgb8[2] = uint8_t(clamp(_rgba32f[2], 0.0f, 1.0f) * 255.0f);
    }

    inline void rgba8FromRgba32f(uint8_t* _rgba8, const float* _rgba32f)
    {
        _rgba8[0] = uint8_t(clamp(_rgba32f[0], 0.0f, 1.0f) * 255.0f);
        _rgba8[1] = uint8_t(clamp(_rgba32f[1], 0.0f, 1.0f) * 255.0f);
        _rgba8[2] = uint8_t(clamp(_rgba32f[2], 0.0f, 1.0f) * 255.0f);
        _rgba8[3] = uint8_t(clamp(_rgba32f[3], 0.0f, 1.0f) * 255.0f);
    }

    inline void rgb16FromRgba32f(uint16_t* _rgb16, const float* _rgba32f)
    {
        _rgb16[0] = uint16_t(clamp(_rgba32f[0], 0.0f, 1.0f) * 65535.0f);
        _rgb16[1] = uint16_t(clamp(_rgba32f[1], 0.0f, 1.0f) * 65535.0f);
        _rgb16[2] = uint16_t(clamp(_rgba32f[2], 0.0f, 1.0f) * 65535.0f);
    }

    inline void rgba16FromRgba32f(uint16_t* _rgba16, const float* _rgba32f)
    {
        _rgba16[0] = uint16_t(clamp(_rgba32f[0], 0.0f, 1.0f) * 65535.0f);
        _rgba16[1] = uint16_t(clamp(_rgba32f[1], 0.0f, 1.0f) * 65535.0f);
        _rgba16[2] = uint16_t(clamp(_rgba32f[2], 0.0f, 1.0f) * 65535.0f);
        _rgba16[3] = uint16_t(clamp(_rgba32f[3], 0.0f, 1.0f) * 65535.0f);
    }

    inline void rgb16fFromRgba32f(uint16_t* _rgb16f, const float* _rgba32f)
    {
        _rgb16f[0] = bx::halfFromFloat(_rgba32f[0]);
        _rgb16f[1] = bx::halfFromFloat(_rgba32f[1]);
        _rgb16f[2] = bx::halfFromFloat(_rgba32f[2]);
        _rgb16f[3] = bx::halfFromFloat(_rgba32f[3]);
    }

    inline void rgba16fFromRgba32f(uint16_t* _rgba16f, const float* _rgba32f)
    {
        _rgba16f[0] = bx::halfFromFloat(_rgba32f[0]);
        _rgba16f[1] = bx::halfFromFloat(_rgba32f[1]);
        _rgba16f[2] = bx::halfFromFloat(_rgba32f[2]);
        _rgba16f[3] = bx::halfFromFloat(_rgba32f[3]);
    }

    inline void rgb32fFromRgba32f(float* _rgb32f, const float* _rgba32f)
    {
        _rgb32f[0] = _rgba32f[0];
        _rgb32f[1] = _rgba32f[1];
        _rgb32f[2] = _rgba32f[2];
    }

    inline void rgba32fFromRgba32f(float* _dst, const float* _src)
    {
        memcpy(_dst, _src, 4*sizeof(float));
    }

    inline void rgbeFromRgba32f(uint8_t* _rgbe, const float* _rgba32f)
    {
        const float maxVal = max(_rgba32f[0], max(_rgba32f[1], _rgba32f[2]));
        const float exp = ceilf(log2f(maxVal));
        const float toRgb8 = 255.0f * 1.0f/ldexp(1.0f, int(exp)); //ldexp -> exp2 (c++11 - <cmath.h>)
        _rgbe[0] = uint8_t(_rgba32f[0] * toRgb8);
        _rgbe[1] = uint8_t(_rgba32f[1] * toRgb8);
        _rgbe[2] = uint8_t(_rgba32f[2] * toRgb8);
        _rgbe[3] = uint8_t(exp+128.0f);
    }

    void fromRgba32f(void* _out, TextureFormat::Enum _format, const float _rgba32f[4])
    {
        switch(_format)
        {
        case TextureFormat::BGR8:     bgr8FromRgba32f((uint8_t*)_out,     _rgba32f); break;
        case TextureFormat::RGB8:     rgb8FromRgba32f((uint8_t*)_out,     _rgba32f); break;
        case TextureFormat::RGB16:    rgb16FromRgba32f((uint16_t*)_out,   _rgba32f); break;
        case TextureFormat::RGB16F:   rgb16fFromRgba32f((uint16_t*)_out,  _rgba32f); break;
        case TextureFormat::RGB32F:   rgb32fFromRgba32f((float*)_out,     _rgba32f); break;
        case TextureFormat::RGBE:     rgbeFromRgba32f((uint8_t*)_out,     _rgba32f); break;
        case TextureFormat::BGRA8:    bgra8FromRgba32f((uint8_t*)_out,    _rgba32f); break;
        case TextureFormat::RGBA8:    rgba8FromRgba32f((uint8_t*)_out,    _rgba32f); break;
        case TextureFormat::RGBA16:   rgba16FromRgba32f((uint16_t*)_out,  _rgba32f); break;
        case TextureFormat::RGBA16F:  rgba16fFromRgba32f((uint16_t*)_out, _rgba32f); break;
        case TextureFormat::RGBA32F:  rgba32fFromRgba32f((float*)_out,    _rgba32f); break;
        default: DEBUG_CHECK(false, "Unknown image format.");
        };
    }

    void imageFromRgba32f(Image& _dst, TextureFormat::Enum _dstFormat, const Image& _src)
    {
        DEBUG_CHECK(TextureFormat::RGBA32F == _src.m_format, "Source image is not in RGBA32F format!");

        // Alloc dst data.
        const uint32_t pixelCount = imageGetNumPixels(_src);
        const uint8_t dstBytesPerPixel = getImageDataInfo(_dstFormat).m_bytesPerPixel;
        const uint32_t dstDataSize = pixelCount*dstBytesPerPixel;
        void* dstData = malloc(dstDataSize);
        MALLOC_CHECK(dstData);

        // Get total number of channels.
        const uint8_t srcNumChannels = getImageDataInfo(_src.m_format).m_numChanels;
        const uint32_t totalNumChannels = pixelCount*srcNumChannels;

        // Convert data.
        const float* src = (const float*)_src.m_data;
        const float* end = (const float*)_src.m_data + totalNumChannels;
        switch(_dstFormat)
        {
        case TextureFormat::BGR8:
            {
                uint8_t* dst = (uint8_t*)dstData;

                for (;src < end; src+=4, dst+=3)
                {
                    bgr8FromRgba32f(dst, src);
                }
            }
        break;

        case TextureFormat::RGB8:
            {
                uint8_t* dst = (uint8_t*)dstData;

                for (;src < end; src+=4, dst+=3)
                {
                    rgb8FromRgba32f(dst, src);
                }
            }
        break;

        case TextureFormat::RGB16:
            {
                uint16_t* dst = (uint16_t*)dstData;

                for (;src < end; src+=4, dst+=3)
                {
                    rgb16FromRgba32f(dst, src);
                }
            }
        break;

        case TextureFormat::RGB16F:
            {
                uint16_t* dst = (uint16_t*)dstData;

                for (;src < end; src+=4, dst+=3)
                {
                    rgb16fFromRgba32f(dst, src);
                }
            }
        break;

        case TextureFormat::RGB32F:
            {
                float* dst = (float*)dstData;

                for (;src < end; src+=4, dst+=3)
                {
                    rgb32fFromRgba32f(dst, src);
                }
            }
        break;

        case TextureFormat::RGBE:
            {
                uint8_t* dst = (uint8_t*)dstData;

                for (;src < end; src+=4, dst+=4)
                {
                    rgbeFromRgba32f(dst, src);
                }
            }
        break;

        case TextureFormat::BGRA8:
            {
                uint8_t* dst = (uint8_t*)dstData;

                for (;src < end; src+=4, dst+=4)
                {
                    bgra8FromRgba32f(dst, src);
                }
            }
        break;

        case TextureFormat::RGBA8:
            {
                uint8_t* dst = (uint8_t*)dstData;

                for (;src < end; src+=4, dst+=4)
                {
                    rgba8FromRgba32f(dst, src);
                }
            }
        break;

        case TextureFormat::RGBA16:
            {
                uint16_t* dst = (uint16_t*)dstData;

                for (;src < end; src+=4, dst+=4)
                {
                    rgba16FromRgba32f(dst, src);
                }
            }
        break;

        case TextureFormat::RGBA16F:
            {
                uint16_t* dst = (uint16_t*)dstData;

                for (;src < end; src+=4, dst+=4)
                {
                    rgba16fFromRgba32f(dst, src);
                }
            }
        break;

        case TextureFormat::RGBA32F:
            {
                float* dst = (float*)dstData;

                for (;src < end; src+=4, dst+=4)
                {
                    rgba32fFromRgba32f(dst, src);
                }
            }
        break;

        default:
            {
                DEBUG_CHECK(false, "Unknown image format.");
            }
        break;
        };

        // Fill image structure.
        Image result;
        result.m_data = dstData;
        result.m_width = _src.m_width;
        result.m_height = _src.m_height;
        result.m_dataSize = dstDataSize;
        result.m_format = _dstFormat;
        result.m_numMips = _src.m_numMips;
        result.m_numFaces = _src.m_numFaces;

        // Output.
        imageMove(_dst, result);
    }

    void imageFromRgba32f(Image& _image, TextureFormat::Enum _textureFormat)
    {
        Image tmp;
        imageFromRgba32f(tmp, _textureFormat, _image);
        imageMove(_image, tmp);
    }

    void imageConvert(Image& _dst, TextureFormat::Enum _dstFormat, const Image& _src)
    {
        // Image _src to rgba32f.
        Image imageRgba32f;
        if (TextureFormat::RGBA32F == _src.m_format)
        {
            imageRef(imageRgba32f, _src);
        }
        else
        {
            imageToRgba32f(imageRgba32f, _src);
        }

        // Image rgba32f to _dst.
        if (TextureFormat::RGBA32F == _dstFormat)
        {
            imageUnload(_dst);
            imageCopy(_dst, imageRgba32f);
        }
        else
        {
            imageUnload(_dst);
            imageFromRgba32f(_dst, _dstFormat, imageRgba32f);
        }

        // Unload imageRgba32f if its a copy and NOT a reference of _src.
        if (_src.m_data != imageRgba32f.m_data)
        {
            imageUnload(imageRgba32f);
        }
    }

    void imageConvert(Image& _image, TextureFormat::Enum _format)
    {
        if (_format != _image.m_format)
        {
            Image tmp;
            imageConvert(tmp, _format, _image);
            imageMove(_image, tmp);
        }
    }

    bool imageRefOrConvert(Image& _dst, TextureFormat::Enum _format, const Image& _src)
    {
        if (_format == _src.m_format)
        {
            imageRef(_dst, _src);
            return true;
        }
        else
        {
            imageUnload(_dst);
            imageConvert(_dst, _format, _src);
            return false;
        }
    }

    void imageGetPixel(void* _out, TextureFormat::Enum _format, uint32_t _x, uint32_t _y, uint8_t _mip, uint8_t _face, const Image& _image)
    {
        // Input check.
        DEBUG_CHECK(_x < _image.m_width,       "Invalid input parameters. X coord bigger than image width.");
        DEBUG_CHECK(_y < _image.m_height,      "Invalid input parameters. Y coord bigger than image height.");
        DEBUG_CHECK(_face < _image.m_numFaces, "Invalid input parameters. Requesting pixel from non-existing face.");
        DEBUG_CHECK(_mip < _image.m_numMips,   "Invalid input parameters. Requesting pixel from non-existing mip level.");

        const uint32_t bytesPerPixel = getImageDataInfo(_image.m_format).m_bytesPerPixel;
        const uint32_t pitch = _image.m_width * bytesPerPixel;

        // Get face and mip offset.
        uint32_t offset = 0;
        for (uint8_t face = 0; face < _face; ++face)
        {
            for (uint8_t mip = 0; mip < _mip; ++mip)
            {
                const uint32_t width  = max(UINT32_C(1), _image.m_width  >> mip);
                const uint32_t height = max(UINT32_C(1), _image.m_height >> mip);
                offset += width * height * bytesPerPixel;
            }
        }

        const void* src = (const void*)((const uint8_t*)_image.m_data + offset + _y*pitch + _x*bytesPerPixel);

        // Output.
        if (_image.m_format == _format)
        {
            // Image is already in requested format, just copy data.
            memcpy(_out, src, bytesPerPixel);
        }
        else if (_image.m_format == TextureFormat::RGBA32F)
        {
            // Image is in rgba32f format. Transform to output format.
            fromRgba32f(_out, _format, (const float*)src);
        }
        else
        {
            // Image is in some other format.
            // Transform to rgba32f and then back to requested output format.
            float buf[4];
            toRgba32f(buf, _image.m_format, src);
            fromRgba32f(_out, _format, buf);
        }
    }

    void imageResize(Image& _dst, uint32_t _width, uint32_t _height, const Image& _src)
    {
        // Operation is done in rgba32f format.
        Image imageRgba32f;
        const bool imageIsRef = imageRefOrConvert(imageRgba32f, TextureFormat::RGBA32F, _src);

        // Alloc dst data.
        const uint32_t bytesPerPixel = 4 /*numChannels*/ * 4 /*bytesPerChannel*/;
        const uint32_t dstPitch = _width * bytesPerPixel;
        const uint32_t dstFaceDataSize = dstPitch * _height;
        const uint32_t dstDataSize = dstFaceDataSize * imageRgba32f.m_numFaces;
        void* dstData = malloc(dstDataSize);
        MALLOC_CHECK(dstData);

        // Get source offsets.
        uint32_t srcFaceOffsets[6];
        imageGetFaceOffsets(srcFaceOffsets, imageRgba32f);
        const uint32_t srcPitch = imageRgba32f.m_width * bytesPerPixel;

        // Get required parameters for processing.
        const float dstToSrcRatioX = float(int32_t(imageRgba32f.m_width))  / float(int32_t(_width));
        const float dstToSrcRatioY = float(int32_t(imageRgba32f.m_height)) / float(int32_t(_height));

        // Resize base image.
        for (uint8_t face = 0; face < imageRgba32f.m_numFaces; ++face)
        {
            uint8_t* dstFaceData = (uint8_t*)dstData + face*dstFaceDataSize;
            const uint8_t* srcFaceData = (const uint8_t*)imageRgba32f.m_data + srcFaceOffsets[face];

            for (uint32_t yDst = 0; yDst < _height; ++yDst)
            {
                uint8_t* dstFaceRow = (uint8_t*)dstFaceData + yDst*dstPitch;

                for (uint32_t xDst = 0; xDst < _width; ++xDst)
                {
                    float* dstFaceColumn = (float*)((uint8_t*)dstFaceRow + xDst*bytesPerPixel);

                    float color[3] = { 0.0f, 0.0f, 0.0f };
                    uint32_t weight = 0;

                    uint32_t ySrc = uint32_t(float(yDst)*dstToSrcRatioY);
                    uint32_t ySrcEnd = ySrc + max(uint32_t(1), uint32_t(dstToSrcRatioY));
                    for (; ySrc < ySrcEnd; ++ySrc)
                    {
                        const uint8_t* srcRowData = (const uint8_t*)srcFaceData + ySrc*srcPitch;

                        uint32_t xSrc = uint32_t(float(xDst)*dstToSrcRatioX);
                        uint32_t xSrcEnd = xSrc + max(uint32_t(1), uint32_t(dstToSrcRatioX));
                        for (; xSrc < xSrcEnd; ++xSrc)
                        {
                            const float* srcColumnData = (const float*)((const uint8_t*)srcRowData + xSrc*bytesPerPixel);
                            color[0] += srcColumnData[0];
                            color[1] += srcColumnData[1];
                            color[2] += srcColumnData[2];
                            weight++;
                        }
                    }

                    const float invWeight = 1.0f/float(max(weight, UINT32_C(1)));
                    dstFaceColumn[0] = color[0] * invWeight;
                    dstFaceColumn[1] = color[1] * invWeight;
                    dstFaceColumn[2] = color[2] * invWeight;
                    dstFaceColumn[3] = 1.0f;
                }
            }
        }

        // Fill image structure.
        Image result;
        result.m_width = _width;
        result.m_height = _height;
        result.m_dataSize = dstDataSize;
        result.m_format = TextureFormat::RGBA32F;
        result.m_numMips = 1;
        result.m_numFaces = imageRgba32f.m_numFaces;
        result.m_data = dstData;

        // Convert result to source format.
        if (TextureFormat::RGBA32F == _src.m_format)
        {
            imageMove(_dst, result);
        }
        else
        {
            imageConvert(_dst, (TextureFormat::Enum)_src.m_format, result);
            imageUnload(result);
        }

        // Cleanup.
        if (!imageIsRef)
        {
            imageUnload(imageRgba32f);
        }
    }

    void imageResize(Image& _image, uint32_t _width, uint32_t _height)
    {
        Image tmp;
        imageResize(tmp, _width, _height, _image);
        imageMove(_image, tmp);
    }

    void imageTransformUseMacroInstead(Image* _image, ...)
    {
        va_list argList;
        va_start(argList, _image);

        uint32_t op = va_arg(argList, uint32_t);
        if (UINT32_MAX != op)
        {
            const uint32_t bytesPerPixel = getImageDataInfo(_image->m_format).m_bytesPerPixel;

            uint32_t offsets[CUBE_FACE_NUM][MAX_MIP_NUM];
            imageGetMipOffsets(offsets, *_image);

            for (uint8_t ii = 0; op != UINT32_MAX; ++ii, op = va_arg(argList, uint32_t))
            {
                const uint16_t imageOp = (op&IMAGE_OP_MASK);
                const uint8_t imageFace = (op&IMAGE_FACE_MASK)>>IMAGE_FACE_SHIFT;

                if (imageOp&IMAGE_OP_ROT_90)
                {
                    if (_image->m_width == _image->m_height)
                    {
                        for (uint8_t mip = 0; mip < _image->m_numMips; ++mip)
                        {
                            const uint32_t width  = max(UINT32_C(1), _image->m_width  >> mip);
                            const uint32_t height = max(UINT32_C(1), _image->m_height >> mip);
                            const uint32_t pitch = width * bytesPerPixel;

                            uint8_t* facePtr = (uint8_t*)_image->m_data + offsets[imageFace][mip];
                            for (uint32_t yy = 0, yyEnd = height-1; yy < height; ++yy, --yyEnd)
                            {
                                uint8_t* rowPtr    = (uint8_t*)facePtr + yy*pitch;
                                uint8_t* columnPtr = (uint8_t*)facePtr + yyEnd*bytesPerPixel;
                                for (uint32_t xx = 0, xxEnd = width-1; xx < width; ++xx, --xxEnd)
                                {
                                    if (xx < yyEnd)
                                    {
                                        uint8_t* aa = (uint8_t*)rowPtr    + xx*bytesPerPixel;
                                        uint8_t* bb = (uint8_t*)columnPtr + xxEnd*pitch;
                                        cmft_swap(aa, bb, bytesPerPixel);
                                    }
                                }
                            }
                        }
                    }
                    else
                    {
                        WARN("Because image data transformation is done in place, "
                             "rotation operations work only when image width is equal to image height."
                             );
                    }
                }

                if (imageOp&IMAGE_OP_ROT_180)
                {
                    if (_image->m_width == _image->m_height)
                    {
                        for (uint8_t mip = 0; mip < _image->m_numMips; ++mip)
                        {
                            const uint32_t width  = max(UINT32_C(1), _image->m_width  >> mip);
                            const uint32_t height = max(UINT32_C(1), _image->m_height >> mip);
                            const uint32_t pitch = width * bytesPerPixel;

                            uint8_t* facePtr = (uint8_t*)_image->m_data + offsets[imageFace][mip];
                            uint32_t yy = 0, yyEnd = height-1;
                            for (; yy < yyEnd; ++yy, --yyEnd)
                            {
                                uint8_t* rowPtr    = (uint8_t*)facePtr + pitch*yy;
                                uint8_t* rowPtrEnd = (uint8_t*)facePtr + pitch*yyEnd;
                                for (uint32_t xx = 0, xxEnd = width-1; xx < width; ++xx, --xxEnd)
                                {
                                    uint8_t* aa = (uint8_t*)rowPtr    + bytesPerPixel*xx;
                                    uint8_t* bb = (uint8_t*)rowPtrEnd + bytesPerPixel*xxEnd;
                                    cmft_swap(aa, bb, bytesPerPixel);
                                }
                            }

                            // Handle middle line as special case.
                            if (yy == yyEnd)
                            {
                                uint8_t* rowPtr = (uint8_t*)facePtr + pitch*yy;
                                for (uint32_t xx = 0, xxEnd = width-1; xx < xxEnd; ++xx, --xxEnd)
                                {
                                    uint8_t* aa = (uint8_t*)rowPtr + bytesPerPixel*xx;
                                    uint8_t* bb = (uint8_t*)rowPtr + bytesPerPixel*xxEnd;
                                    cmft_swap(aa, bb, bytesPerPixel);
                                }
                            }
                        }
                    }
                    else
                    {
                        WARN("Because image data transformation is done in place, "
                             "rotation operations work only when image width is equal to image height."
                             );
                    }
                }

                if (imageOp&IMAGE_OP_ROT_270)
                {
                    if (_image->m_width == _image->m_height)
                    {
                        for (uint8_t mip = 0; mip < _image->m_numMips; ++mip)
                        {
                            const uint32_t width  = max(UINT32_C(1), _image->m_width  >> mip);
                            const uint32_t height = max(UINT32_C(1), _image->m_height >> mip);
                            const uint32_t pitch = width * bytesPerPixel;

                            uint8_t* facePtr = (uint8_t*)_image->m_data + offsets[imageFace][mip];
                            for (uint32_t yy = 0; yy < height; ++yy)
                            {
                                uint8_t* rowPtr    = (uint8_t*)facePtr + yy*pitch;
                                uint8_t* columnPtr = (uint8_t*)facePtr + yy*bytesPerPixel;
                                for (uint32_t xx = 0; xx < width; ++xx)
                                {
                                    if (xx > yy)
                                    {
                                        uint8_t* aa = (uint8_t*)rowPtr    + xx*bytesPerPixel;
                                        uint8_t* bb = (uint8_t*)columnPtr + xx*pitch;
                                        cmft_swap(aa, bb, bytesPerPixel);
                                    }
                                }
                            }
                        }
                    }
                    else
                    {
                        WARN("Because image data transformation is done in place, "
                             "rotation operations work only when image width is equal to image height."
                             );
                    }
                }

                if (imageOp&IMAGE_OP_FLIP_X)
                {
                    for (uint8_t mip = 0; mip < _image->m_numMips; ++mip)
                    {
                        const uint32_t width  = max(UINT32_C(1), _image->m_width  >> mip);
                        const uint32_t height = max(UINT32_C(1), _image->m_height >> mip);
                        const uint32_t pitch = width * bytesPerPixel;

                        uint8_t* facePtr = (uint8_t*)_image->m_data + offsets[imageFace][mip];
                        for (uint32_t yy = 0, yyEnd = height-1; yy < yyEnd; ++yy, --yyEnd)
                        {
                            uint8_t* rowPtr    = (uint8_t*)facePtr + pitch*yy;
                            uint8_t* rowPtrEnd = (uint8_t*)facePtr + pitch*yyEnd;
                            cmft_swap(rowPtr, rowPtrEnd, pitch);
                        }
                    }
                }

                if (imageOp&IMAGE_OP_FLIP_Y)
                {
                    for (uint8_t mip = 0; mip < _image->m_numMips; ++mip)
                    {
                        const uint32_t width  = max(UINT32_C(1), _image->m_width  >> mip);
                        const uint32_t height = max(UINT32_C(1), _image->m_height >> mip);
                        const uint32_t pitch = width * bytesPerPixel;

                        uint8_t* facePtr = (uint8_t*)_image->m_data + offsets[imageFace][mip];
                        for (uint32_t yy = 0; yy < height; ++yy)
                        {
                            uint8_t* rowPtr = (uint8_t*)facePtr + pitch*yy;
                            for (uint32_t xx = 0, xxEnd = width-1; xx < xxEnd; ++xx, --xxEnd)
                            {
                                uint8_t* columnPtr    = (uint8_t*)rowPtr + bytesPerPixel*xx;
                                uint8_t* columnPtrEnd = (uint8_t*)rowPtr + bytesPerPixel*xxEnd;
                                cmft_swap(columnPtr, columnPtrEnd, bytesPerPixel);
                            }
                        }
                    }
                }
            }
        }

        va_end(argList);
    }

    void imageGenerateMipMapChain(Image& _image, uint8_t _numMips)
    {
        // Processing is done in rgba32f format.
        Image imageRgba32f;
        const bool imageIsRef = imageRefOrConvert(imageRgba32f, TextureFormat::RGBA32F, _image);

        // Calculate dataSize and offsets for the entire mip map chain.
        uint32_t dstOffsets[CUBE_FACE_NUM][MAX_MIP_NUM];
        uint32_t dstDataSize = 0;
        uint8_t mipCount = 0;
        const uint8_t maxMipNum = min(_numMips, uint8_t(MAX_MIP_NUM));
        const uint32_t bytesPerPixel = 4 /*numChannels*/ * 4 /*bytesPerChannel*/;
        for (uint8_t face = 0; face < imageRgba32f.m_numFaces; ++face)
        {
            uint32_t width = 0;
            uint32_t height = 0;

            for (mipCount = 0; (mipCount < maxMipNum) && (width != 1) && (height != 1); ++mipCount)
            {
                dstOffsets[face][mipCount] = dstDataSize;
                width  = max(UINT32_C(1), imageRgba32f.m_width  >> mipCount);
                height = max(UINT32_C(1), imageRgba32f.m_height >> mipCount);

                dstDataSize += width * height * bytesPerPixel;
            }
        }

        // Alloc data.
        void* dstData = malloc(dstDataSize);
        MALLOC_CHECK(dstData);

        // Get source offsets.
        uint32_t srcOffsets[CUBE_FACE_NUM][MAX_MIP_NUM];
        imageGetMipOffsets(srcOffsets, imageRgba32f);

        // Generate mip chain.
        for (uint8_t face = 0; face < imageRgba32f.m_numFaces; ++face)
        {
            for (uint8_t mip = 0; mip < mipCount; ++mip)
            {
                const uint32_t width  = max(UINT32_C(1), imageRgba32f.m_width  >> mip);
                const uint32_t height = max(UINT32_C(1), imageRgba32f.m_height >> mip);
                const uint32_t pitch = width * bytesPerPixel;

                uint8_t* dstMipData       = (uint8_t*)dstData                   + dstOffsets[face][mip];
                const uint8_t* srcMipData = (const uint8_t*)imageRgba32f.m_data + srcOffsets[face][mip];

                // If mip is present, copy data.
                if (mip < imageRgba32f.m_numMips)
                {
                    for (uint32_t yy = 0; yy < height; ++yy)
                    {
                        uint8_t* dst       = (uint8_t*)dstMipData       + yy*pitch;
                        const uint8_t* src = (const uint8_t*)srcMipData + yy*pitch;
                        memcpy(dst, src, pitch);
                    }
                }
                // Else generate it.
                else
                {
                    const uint8_t parentMip = mip - 1;
                    const uint32_t parentWidth = max(UINT32_C(1), imageRgba32f.m_width >> parentMip);
                    const uint32_t parentPitch = parentWidth * bytesPerPixel;
                    const uint8_t* parentMipData = (const uint8_t*)dstData + dstOffsets[face][parentMip];

                    for (uint32_t yy = 0; yy < height; ++yy)
                    {
                        uint8_t* dstRowData = (uint8_t*)dstMipData + pitch*yy;
                        for (uint32_t xx = 0; xx < width; ++xx)
                        {
                            float* dstColumnData = (float*)((uint8_t*)dstRowData + xx*bytesPerPixel);

                            float color[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
                            for (uint32_t yParent = yy*2, end = yParent+2; yParent < end; ++yParent)
                            {
                                const uint8_t* parentRowData = (const uint8_t*)parentMipData + parentPitch*yParent;
                                for (uint32_t xParent = xx*2, end = xParent+2; xParent < end; ++xParent)
                                {
                                    const float* parentColumnData = (const float*)((const uint8_t*)parentRowData + xParent*bytesPerPixel);
                                    color[0] += parentColumnData[0];
                                    color[1] += parentColumnData[1];
                                    color[2] += parentColumnData[2];
                                    color[3] += parentColumnData[3];
                                }
                            }

                            dstColumnData[0] = color[0] * 0.25f;
                            dstColumnData[1] = color[1] * 0.25f;
                            dstColumnData[2] = color[2] * 0.25f;
                            dstColumnData[3] = color[3] * 0.25f;
                        }
                    }
                }
            }
        }

        // Fill image structure.
        Image result;
        result.m_width = imageRgba32f.m_width;
        result.m_height = imageRgba32f.m_height;
        result.m_dataSize = dstDataSize;
        result.m_format = imageRgba32f.m_format;
        result.m_numMips = mipCount;
        result.m_numFaces = imageRgba32f.m_numFaces;
        result.m_data = dstData;

        // Convert result to source format.
        if (TextureFormat::RGBA32F == (TextureFormat::Enum)_image.m_format)
        {
            imageMove(_image, result);
        }
        else
        {
            imageConvert(_image, (TextureFormat::Enum)_image.m_format, result);
            imageUnload(result);
        }

        // Cleanup.
        if (!imageIsRef)
        {
            imageUnload(imageRgba32f);
        }
    }

    void imageApplyGamma(Image& _image, float _gammaPow)
    {
        // Do nothing if _gammaPow is ~= 1.0f.
        if (0.0001f > fabsf(_gammaPow-1.0f))
        {
            return;
        }

        // Operation is done in rgba32f format.
        Image imageRgba32f;
        imageRefOrConvert(imageRgba32f, TextureFormat::RGBA32F, _image);

        // Iterate through image channels and apply gamma function.
        float* channel = (float*)imageRgba32f.m_data;
        const float* end = (const float*)((const uint8_t*)imageRgba32f.m_data + imageRgba32f.m_dataSize);

        for(;channel < end; channel+=4)
        {
            channel[0] = powf(channel[0], _gammaPow);
            channel[1] = powf(channel[1], _gammaPow);
            channel[2] = powf(channel[2], _gammaPow);
            //channel[3] = leave alpha channel as is.
        }

        // If image was converted, convert back to original format. Otherwise, a reference to self is passed.
        imageRefOrConvert(_image, (TextureFormat::Enum)_image.m_format, imageRgba32f);
    }

    void imageClamp(Image& _dst, const Image& _src)
    {
        // Get a copy in rgba32f format.
        Image imageRgba32f;
        imageConvert(imageRgba32f, TextureFormat::RGBA32F, _src);

        // Iterate through image channels and clamp to [0.0-1.0] range.
        float* channel = (float*)imageRgba32f.m_data;
        const float* end = (const float*)((const uint8_t*)imageRgba32f.m_data + imageRgba32f.m_dataSize);

        for(;channel < end; channel+=4)
        {
            channel[0] = clamp(channel[0], 0.0f, 1.0f);
            channel[1] = clamp(channel[1], 0.0f, 1.0f);
            channel[2] = clamp(channel[2], 0.0f, 1.0f);
            channel[3] = clamp(channel[3], 0.0f, 1.0f);
        }

        // Move or convert to original format.
        if (TextureFormat::RGBA32F == _src.m_format)
        {
            imageMove(_dst, imageRgba32f);
        }
        else
        {
            imageConvert(_dst, (TextureFormat::Enum)_src.m_format, imageRgba32f);
            imageUnload(imageRgba32f);
        }
    }

    void imageClamp(Image& _image)
    {
        // Operation is done in rgba32f format.
        Image imageRgba32f;
        imageRefOrConvert(imageRgba32f, TextureFormat::RGBA32F, _image);

        // Iterate through image channels and clamp to [0.0-1.0] range.
        float* channel = (float*)imageRgba32f.m_data;
        const float* end = (const float*)((const uint8_t*)imageRgba32f.m_data + imageRgba32f.m_dataSize);

        for(;channel < end; channel+=4)
        {
            channel[0] = clamp(channel[0], 0.0f, 1.0f);
            channel[1] = clamp(channel[1], 0.0f, 1.0f);
            channel[2] = clamp(channel[2], 0.0f, 1.0f);
            channel[3] = clamp(channel[3], 0.0f, 1.0f);
        }

        // If image was converted, convert back to original format. Otherwise, a reference to self is passed.
        imageRefOrConvert(_image, (TextureFormat::Enum)_image.m_format, imageRgba32f);
    }

    bool imageIsCubemap(const Image& _image)
    {
        return (6 == _image.m_numFaces) && (_image.m_width == _image.m_height);
    }

    bool imageIsLatLong(const Image& _image)
    {
        const float aspect = (float)(int32_t)_image.m_width/(float)(int32_t)_image.m_height;
        return (0.00001f > fabsf(aspect-2.0f));
    }

    bool imageIsHStrip(const Image& _image)
    {
        return (_image.m_width == 6*_image.m_height);
    }

    bool imageValidCubemapFaceList(const Image _faceList[6])
    {
        const uint32_t size = _faceList[0].m_width;
        const uint8_t numMips = _faceList[0].m_numMips;
        for (uint8_t face = 0; face < 6; ++face)
        {
            if (_faceList[face].m_width != _faceList[face].m_height
            ||  size    != _faceList[face].m_width
            ||  numMips != _faceList[face].m_numMips)
            {
                return false;
            }
        }
        return true;
    }

    bool imageIsCubeCross(Image& _image)
    {
        // Check face count.
        if (1 != _image.m_numFaces)
        {
            return false;
        }

        // Check aspect.
        const float aspect = (float)(int32_t)_image.m_width/(float)(int32_t)_image.m_height;
        const bool isVertical   = (aspect - 3.0f/4.0f) < 0.0001f;
        const bool isHorizontal = (aspect - 4.0f/3.0f) < 0.0001f;

        if (!isVertical && !isHorizontal)
        {
            return false;
        }

        // Define key points.
        const uint32_t bytesPerPixel = getImageDataInfo(_image.m_format).m_bytesPerPixel;
        const uint32_t imagePitch = _image.m_width * bytesPerPixel;

        const uint32_t faceSize = alignf((float)(int32_t)_image.m_width / (isVertical ? 3.0f : 4.0f), bytesPerPixel);
        const uint32_t facePitch = faceSize * bytesPerPixel;
        const uint32_t rowDataSize = imagePitch * faceSize;

        const uint32_t halfFacePitch   = alignf((float)(int32_t)facePitch   / 2.0f, bytesPerPixel);
        const uint32_t halfRowDataSize = alignf((float)(int32_t)rowDataSize / 2.0f, bytesPerPixel);

        uint32_t keyPointsOffsets[6];
        if (isVertical)
        {
            //   ___ ___ ___
            //  |   |   |   | -> rowDataSize
            //  |___|___|___|
            //
            //   ___  -> facePitch
            //
            //   . -> keyPoint
            //       ___
            //    . |Y+ | .
            //   ___|___|___
            //  |X- |Z+ |X+ |
            //  |___|___|___|
            //    . |Y- | .
            //      |___|
            //    . |Z- | .
            //      |___|
            //
            const uint32_t leftCenter  = halfRowDataSize + halfFacePitch;
            const uint32_t rightCenter = halfRowDataSize + 5*halfFacePitch;
            const uint32_t firstRow  = 0;
            const uint32_t thirdRow  = 2*rowDataSize;
            const uint32_t fourthRow = 3*rowDataSize;
            keyPointsOffsets[0] =  leftCenter + firstRow;  //+x
            keyPointsOffsets[1] = rightCenter + firstRow;  //-x
            keyPointsOffsets[2] =  leftCenter + thirdRow;  //+y
            keyPointsOffsets[3] = rightCenter + thirdRow;  //-y
            keyPointsOffsets[4] =  leftCenter + fourthRow; //+z
            keyPointsOffsets[5] = rightCenter + fourthRow; //-z
        }
        else
        {
            //       ___
            //    . |+Y | .   .
            //   ___|___|___ ___
            //  |-X |+Z |+X |-Z |
            //  |___|___|___|___|
            //    . |-Y | .   .
            //      |___|
            //
            const uint32_t center0 = halfRowDataSize + halfFacePitch;
            const uint32_t center1 = halfRowDataSize + 5*halfFacePitch;
            const uint32_t center2 = halfRowDataSize + 7*halfFacePitch;
            const uint32_t firstRow = 0;
            const uint32_t thirdRow = 2*rowDataSize;
            keyPointsOffsets[0] = firstRow + center0; //+x
            keyPointsOffsets[1] = firstRow + center1; //-x
            keyPointsOffsets[2] = firstRow + center2; //+y
            keyPointsOffsets[3] = thirdRow + center0; //-y
            keyPointsOffsets[4] = thirdRow + center1; //+z
            keyPointsOffsets[5] = thirdRow + center2; //-z
        }

        // Check key points.
        bool result = true;
        switch(_image.m_format)
        {
        case TextureFormat::BGR8:
        case TextureFormat::RGB8:
        case TextureFormat::BGRA8:
        case TextureFormat::RGBA8:
            {
                for (uint8_t key = 0; (true == result) && (key < 6); ++key)
                {
                    const uint8_t* point = (const uint8_t*)_image.m_data + keyPointsOffsets[key];
                    const bool tap0 = point[0] < 2;
                    const bool tap1 = point[1] < 2;
                    const bool tap2 = point[2] < 2;
                    result &= (tap0 & tap1 & tap2);
                }
            }
        break;

        case TextureFormat::RGB16:
        case TextureFormat::RGBA16:
            {
                for (uint8_t key = 0; (true == result) && (key < 6); ++key)
                {
                    const uint16_t* point = (const uint16_t*)((const uint8_t*)_image.m_data + keyPointsOffsets[key]);
                    const bool tap0 = point[0] < 2;
                    const bool tap1 = point[1] < 2;
                    const bool tap2 = point[2] < 2;
                    result &= (tap0 & tap1 & tap2);
                }
            }
        break;

        case TextureFormat::RGB16F:
        case TextureFormat::RGBA16F:
            {
                for (uint8_t key = 0; (true == result) && (key < 6); ++key)
                {
                    const uint16_t* point = (const uint16_t*)((const uint8_t*)_image.m_data + keyPointsOffsets[key]);
                    const bool tap0 = bx::halfToFloat(point[0]) < 0.01f;
                    const bool tap1 = bx::halfToFloat(point[1]) < 0.01f;
                    const bool tap2 = bx::halfToFloat(point[2]) < 0.01f;
                    result &= (tap0 & tap1 & tap2);
                }
            }
        break;

        case TextureFormat::RGB32F:
        case TextureFormat::RGBA32F:
            {
                for (uint8_t key = 0; (true == result) && (key < 6); ++key)
                {
                    const float* point = (const float*)((const uint8_t*)_image.m_data + keyPointsOffsets[key]);
                    const bool tap0 = point[0] < 0.01f;
                    const bool tap1 = point[1] < 0.01f;
                    const bool tap2 = point[2] < 0.01f;
                    result &= (tap0 & tap1 & tap2);
                }
            }
        break;

        case TextureFormat::RGBE:
            {
                for (uint8_t key = 0; (true == result) && (key < 6); ++key)
                {
                    const uint8_t* point = (const uint8_t*)_image.m_data + keyPointsOffsets[key];
                    const float exp = ldexp(1.0f, point[3] - (128+8));
                    const bool tap0 = float(point[0])*exp < 0.01f;
                    const bool tap1 = float(point[1])*exp < 0.01f;
                    const bool tap2 = float(point[2])*exp < 0.01f;
                    result &= (tap0 & tap1 & tap2);
                }
            }
        break;

        default:
            {
                DEBUG_CHECK(false, "Unknown image format.");
                result = false;
            }
        break;
        };

        return result;
    }

    bool imageCubemapFromCross(Image& _dst, const Image& _src)
    {
        // Checking image aspect.
        const float aspect = (float)(int32_t)_src.m_width/(float)(int32_t)_src.m_height;
        const bool isVertical   = (aspect - 3.0f/4.0f) < 0.0001f;
        const bool isHorizontal = (aspect - 3.0f/4.0f) < 0.0001f;

        if (!isVertical && !isHorizontal)
        {
            return false;
        }

        // Get sizes.
        const uint32_t srcBytesPerPixel = getImageDataInfo(_src.m_format).m_bytesPerPixel;
        const uint32_t imagePitch = _src.m_width * srcBytesPerPixel;
        const uint32_t faceSize = isVertical ? (_src.m_width+2)/3 : (_src.m_width+3)/4;
        const uint32_t facePitch = faceSize * srcBytesPerPixel;
        const uint32_t faceDataSize = facePitch * faceSize;
        const uint32_t rowDataSize = imagePitch * faceSize;

        // Alloc data.
        const uint32_t dstDataSize = faceDataSize * CUBE_FACE_NUM;
        void* data = malloc(dstDataSize);
        MALLOC_CHECK(data);

        // Setup offsets.
        uint32_t faceOffsets[6];
        if (isVertical)
        {
            //   ___ ___ ___
            //  |   |   |   | -> rowDataSize
            //  |___|___|___|
            //
            //   ___  -> facePitch
            //
            //       ___
            //      |+Y |
            //   ___|___|___
            //  |-X |+Z |+X |
            //  |___|___|___|
            //      |-Y |
            //      |___|
            //      |-Z |
            //      |___|
            //
            faceOffsets[0] = rowDataSize + 2*facePitch; //+x
            faceOffsets[1] = rowDataSize;               //-x
            faceOffsets[2] = facePitch;                 //+y
            faceOffsets[3] = 2*rowDataSize + facePitch; //-y
            faceOffsets[4] = rowDataSize + facePitch;   //+z
            faceOffsets[5] = 3*rowDataSize + facePitch; //-z
        }
        else
        {
            //       ___
            //      |+Y |
            //   ___|___|___ ___
            //  |-X |+Z |+X |-Z |
            //  |___|___|___|___|
            //      |-Y |
            //      |___|
            //
            faceOffsets[0] = rowDataSize + 2*facePitch; //+x
            faceOffsets[1] = rowDataSize;               //-x
            faceOffsets[2] = facePitch;                 //+y
            faceOffsets[3] = 2*rowDataSize + facePitch; //-y
            faceOffsets[4] = rowDataSize + facePitch;   //+z
            faceOffsets[5] = rowDataSize + 3*facePitch; //-z
        }

        // Copy data.
        for (uint8_t face = 0; face < 6; ++face)
        {
            const uint8_t* srcFaceData = (const uint8_t*)_src.m_data + faceOffsets[face];
            uint8_t* dstFaceData = (uint8_t*)data + faceDataSize*face;
            for (uint32_t yy = 0; yy < faceSize; ++yy)
            {
                memcpy(&dstFaceData[facePitch*yy], &srcFaceData[imagePitch*yy], facePitch);
            }
        }

        // Fill image structure.
        Image result;
        result.m_width = faceSize;
        result.m_height = faceSize;
        result.m_dataSize = dstDataSize;
        result.m_format = _src.m_format;
        result.m_numMips = 1;
        result.m_numFaces = 6;
        result.m_data = data;

        // Transform -Z face properly.
        if (isVertical)
        {
            imageTransform(result, IMAGE_FACE_NEGATIVEZ | IMAGE_OP_FLIP_X | IMAGE_OP_FLIP_Y);
        }

        // Output.
        imageMove(_dst, result);

        return true;
    }

    void imageCubemapFromCross(Image& _image)
    {
        Image tmp;
        if (imageCubemapFromCross(tmp, _image))
        {
            imageMove(_image, tmp);
        }
    }

    bool imageCubemapFromLatLong(Image& _dst, const Image& _src, bool _useBilinearInterpolation)
    {
        if (!imageIsLatLong(_src))
        {
            return false;
        }

        // Conversion is done in rgba32f format.
        Image imageRgba32f;
        const bool imageIsRef = imageRefOrConvert(imageRgba32f, TextureFormat::RGBA32F, _src);

        // Alloc data.
        const uint32_t bytesPerPixel = 4 /*numChannels*/ * 4 /*bytesPerChannel*/;
        const uint32_t dstFaceSize = (imageRgba32f.m_height+1)/2;
        const uint32_t dstPitch = dstFaceSize * bytesPerPixel;
        const uint32_t dstFaceDataSize = dstPitch * dstFaceSize;
        const uint32_t dstDataSize = dstFaceDataSize * CUBE_FACE_NUM;
        void* dstData = malloc(dstDataSize);
        MALLOC_CHECK(dstData);

        // Get source parameters.
        const float srcWidthf  = float(int32_t(imageRgba32f.m_width));
        const float srcHeightf = float(int32_t(imageRgba32f.m_height));
        const uint32_t srcPitch = imageRgba32f.m_width * bytesPerPixel;
        const float invDstFaceSizef = 1.0f/float(dstFaceSize);

        // Iterate over destination image (cubemap).
        for (uint8_t face = 0; face < 6; ++face)
        {
            uint8_t* dstFaceData = (uint8_t*)dstData + face*dstFaceDataSize;
            for (uint32_t yy = 0; yy < dstFaceSize; ++yy)
            {
                uint8_t* dstRowData = (uint8_t*)dstFaceData + yy*dstPitch;
                for (uint32_t xx = 0; xx < dstFaceSize; ++xx)
                {
                    float* dstColumnData = (float*)((uint8_t*)dstRowData + xx*bytesPerPixel);

                    // Cubemap (u,v) on current face.
                    const float uu = 2.0f*xx*invDstFaceSizef-1.0f;
                    const float vv = 2.0f*yy*invDstFaceSizef-1.0f;

                    // Get cubemap vector (x,y,z) from (u,v,faceIdx).
                    float vec[3];
                    texelCoordToVec(vec, uu, vv, face, dstFaceSize);

                    // Convert cubemap vector (x,y,z) to latlong (u,v).
                    float xSrc;
                    float ySrc;
                    latLongFromVec(xSrc, ySrc, vec);

                    // Convert from [0..1] to [0..(size-1)] range.
                    xSrc *= srcWidthf-1.0f;
                    ySrc *= srcHeightf-1.0f;

                    // Sample from latlong (u,v).
                    if (_useBilinearInterpolation)
                    {
                        const uint32_t x0 = uint32_t(xSrc);
                        const uint32_t y0 = uint32_t(ySrc);
                        const uint32_t x1 = min(x0+1, imageRgba32f.m_width-1);
                        const uint32_t y1 = min(y0+1, imageRgba32f.m_height-1);

                        const float *src0 = (const float*)((const uint8_t*)imageRgba32f.m_data + y0*srcPitch + x0*bytesPerPixel);
                        const float *src1 = (const float*)((const uint8_t*)imageRgba32f.m_data + y0*srcPitch + x1*bytesPerPixel);
                        const float *src2 = (const float*)((const uint8_t*)imageRgba32f.m_data + y1*srcPitch + x0*bytesPerPixel);
                        const float *src3 = (const float*)((const uint8_t*)imageRgba32f.m_data + y1*srcPitch + x1*bytesPerPixel);

                        const float tx = xSrc - float(int32_t(x0));
                        const float ty = ySrc - float(int32_t(y0));
                        const float invTx = 1.0f - tx;
                        const float invTy = 1.0f - ty;

                        float p0[3];
                        float p1[3];
                        float p2[3];
                        float p3[3];
                        vec3Mul(p0, src0, invTx*invTy);
                        vec3Mul(p1, src1,    tx*invTy);
                        vec3Mul(p2, src2, invTx*   ty);
                        vec3Mul(p3, src3,    tx*   ty);

                        const float rr = p0[0] + p1[0] + p2[0] + p3[0];
                        const float gg = p0[1] + p1[1] + p2[1] + p3[1];
                        const float bb = p0[2] + p1[2] + p2[2] + p3[2];

                        dstColumnData[0] = rr;
                        dstColumnData[1] = gg;
                        dstColumnData[2] = bb;
                        dstColumnData[3] = 1.0f;
                    }
                    else
                    {
                        const uint32_t xx = uint32_t(xSrc);
                        const uint32_t yy = uint32_t(ySrc);
                        const float *src = (const float*)((const uint8_t*)imageRgba32f.m_data + yy*srcPitch + xx*bytesPerPixel);

                        dstColumnData[0] = src[0];
                        dstColumnData[1] = src[1];
                        dstColumnData[2] = src[2];
                        dstColumnData[3] = 1.0f;
                    }

                }
            }
        }

        // Fill image structure.
        Image result;
        result.m_width = dstFaceSize;
        result.m_height = dstFaceSize;
        result.m_dataSize = dstDataSize;
        result.m_format = TextureFormat::RGBA32F;
        result.m_numMips = 1;
        result.m_numFaces = 6;
        result.m_data = dstData;

        // Convert result to source format.
        if (TextureFormat::RGBA32F == _src.m_format)
        {
            imageMove(_dst, result);
        }
        else
        {
            imageConvert(_dst, (TextureFormat::Enum)_src.m_format, result);
            imageUnload(result);
        }

        // Cleanup.
        if (!imageIsRef)
        {
            imageUnload(imageRgba32f);
        }

        return true;
    }

    void imageCubemapFromLatLong(Image& _image, bool _useBilinearInterpolation)
    {
        Image tmp;
        if (imageCubemapFromLatLong(tmp, _image, _useBilinearInterpolation))
        {
            imageMove(_image, tmp);
        }
    }

    bool imageLatLongFromCubemap(Image& _dst, const Image& _src, bool _useBilinearInterpolation)
    {
        // Input check.
        if(!imageIsCubemap(_src))
        {
            return false;
        }

        // Conversion is done in rgba32f format.
        Image imageRgba32f;
        const bool imageIsRef = imageRefOrConvert(imageRgba32f, TextureFormat::RGBA32F, _src);

        // Alloc data.
        const uint32_t bytesPerPixel = 4 /*numChannels*/ * 4 /*bytesPerChannel*/;
        const uint32_t dstHeight = imageRgba32f.m_height*2;
        const uint32_t dstWidth = imageRgba32f.m_height*4;
        uint32_t dstDataSize = 0;
        uint32_t dstMipOffsets[MAX_MIP_NUM];
        for (uint8_t mip = 0; mip < imageRgba32f.m_numMips; ++mip)
        {
            dstMipOffsets[mip] = dstDataSize;
            const uint32_t dstMipWidth  = max(UINT32_C(1), dstWidth  >> mip);
            const uint32_t mipHeight = max(UINT32_C(1), dstHeight >> mip);
            dstDataSize += dstMipWidth * mipHeight * bytesPerPixel;
        }
        void* dstData = malloc(dstDataSize);
        MALLOC_CHECK(dstData);

        // Get source image parameters.
        uint32_t srcOffsets[CUBE_FACE_NUM][MAX_MIP_NUM];
        imageGetMipOffsets(srcOffsets, imageRgba32f);

        // Iterate over destination image (latlong).
        for (uint8_t mip = 0; mip < imageRgba32f.m_numMips; ++mip)
        {
            const uint32_t dstMipWidth  = max(UINT32_C(1), dstWidth  >> mip);
            const uint32_t dstMipHeight = max(UINT32_C(1), dstHeight >> mip);
            const uint32_t dstMipPitch = dstMipWidth * bytesPerPixel;
            const float invDstWidthf  = 1.0f/float(dstMipWidth-1);
            const float invDstHeightf = 1.0f/float(dstMipHeight-1);

            const uint32_t srcMipWidth  = max(UINT32_C(1), imageRgba32f.m_width  >> mip);
            const uint32_t srcMipHeight = max(UINT32_C(1), imageRgba32f.m_height >> mip);
            const uint32_t srcPitch = srcMipWidth * bytesPerPixel;
            const float srcWidthf  = float(int32_t(srcMipWidth));
            const float srcHeightf = float(int32_t(srcMipHeight));

            uint8_t* dstMipData = (uint8_t*)dstData + dstMipOffsets[mip];
            for (uint32_t yy = 0; yy < dstMipHeight; ++yy)
            {
                uint8_t* dstRowData = (uint8_t*)dstMipData + yy*dstMipPitch;
                for (uint32_t xx = 0; xx < dstMipWidth; ++xx)
                {
                    float* dstColumnData = (float*)((uint8_t*)dstRowData + xx*bytesPerPixel);

                    // Latlong (x,y).
                    const float xDst = xx*invDstWidthf;
                    const float yDst = yy*invDstHeightf;

                    // Get cubemap vector (x,y,z) coresponding to latlong (x,y).
                    float vec[3];
                    vecFromLatLong(vec, xDst, yDst);

                    // Get cubemap (u,v,faceIdx) from cubemap vector (x,y,z).
                    float xSrc;
                    float ySrc;
                    uint8_t faceIdx;
                    vecToTexelCoord(xSrc, ySrc, faceIdx, vec);

                    // Convert from [0-1] to [0-size] range.
                    xSrc *= srcWidthf;
                    ySrc *= srcHeightf;

                    // Sample from cubemap (u,v, faceIdx).
                    if (_useBilinearInterpolation)
                    {
                        const uint32_t x0 = uint32_t(xSrc);
                        const uint32_t y0 = uint32_t(ySrc);
                        const uint32_t x1 = min(x0+1, srcMipWidth-1);
                        const uint32_t y1 = min(y0+1, srcMipHeight-1);

                        const uint8_t* srcFaceData = (const uint8_t*)imageRgba32f.m_data + srcOffsets[faceIdx][mip];
                        const float *src0 = (const float*)((const uint8_t*)srcFaceData + y0*srcPitch + x0*bytesPerPixel);
                        const float *src1 = (const float*)((const uint8_t*)srcFaceData + y0*srcPitch + x1*bytesPerPixel);
                        const float *src2 = (const float*)((const uint8_t*)srcFaceData + y1*srcPitch + x0*bytesPerPixel);
                        const float *src3 = (const float*)((const uint8_t*)srcFaceData + y1*srcPitch + x1*bytesPerPixel);

                        const float tx = xSrc - float(int32_t(x0));
                        const float ty = ySrc - float(int32_t(y0));
                        const float invTx = 1.0f - tx;
                        const float invTy = 1.0f - ty;

                        float p0[3];
                        float p1[3];
                        float p2[3];
                        float p3[3];
                        vec3Mul(p0, src0, invTx*invTy);
                        vec3Mul(p1, src1,    tx*invTy);
                        vec3Mul(p2, src2, invTx*   ty);
                        vec3Mul(p3, src3,    tx*   ty);

                        const float rr = p0[0] + p1[0] + p2[0] + p3[0];
                        const float gg = p0[1] + p1[1] + p2[1] + p3[1];
                        const float bb = p0[2] + p1[2] + p2[2] + p3[2];

                        dstColumnData[0] = rr;
                        dstColumnData[1] = gg;
                        dstColumnData[2] = bb;
                        dstColumnData[3] = 1.0f;
                    }
                    else
                    {
                        const uint32_t xx = uint32_t(xSrc);
                        const uint32_t yy = uint32_t(ySrc);

                        const uint8_t* srcFaceData = (const uint8_t*)imageRgba32f.m_data + srcOffsets[faceIdx][mip];
                        const float *src = (const float*)((const uint8_t*)srcFaceData + yy*srcPitch + xx*bytesPerPixel);

                        dstColumnData[0] = src[0];
                        dstColumnData[1] = src[1];
                        dstColumnData[2] = src[2];
                        dstColumnData[3] = 1.0f;
                    }
                }
            }
        }

        // Fill image structure.
        Image result;
        result.m_width = dstWidth;
        result.m_height = dstHeight;
        result.m_dataSize = dstDataSize;
        result.m_format = TextureFormat::RGBA32F;
        result.m_numMips = imageRgba32f.m_numMips;
        result.m_numFaces = 1;
        result.m_data = dstData;

        // Convert back to source format.
        if (TextureFormat::RGBA32F == _src.m_format)
        {
            imageMove(_dst, result);
        }
        else
        {
            imageConvert(_dst, (TextureFormat::Enum)_src.m_format, result);
            imageUnload(result);
        }

        // Cleanup.
        if (!imageIsRef)
        {
            imageUnload(imageRgba32f);
        }

        return true;
    }

    void imageLatLongFromCubemap(Image& _cubemap, bool _useBilinearInterpolation)
    {
        Image tmp;
        if (imageLatLongFromCubemap(tmp, _cubemap, _useBilinearInterpolation))
        {
            imageMove(_cubemap, tmp);
        }
    }

    bool imageHStripFromCubemap(Image& _dst, const Image& _src)
    {
        // Input check.
        if(!imageIsCubemap(_src))
        {
            return false;
        }

        // Calculate destination offsets and alloc data.
        uint32_t dstDataSize = 0;
        uint32_t dstMipOffsets[MAX_MIP_NUM];
        const uint32_t dstWidth = _src.m_width*6;
        const uint32_t dstHeight = _src.m_width;
        const uint32_t bytesPerPixel = getImageDataInfo(_src.m_format).m_bytesPerPixel;
        for (uint8_t mip = 0; mip < _src.m_numMips; ++mip)
        {
            dstMipOffsets[mip] = dstDataSize;
            const uint32_t mipWidth  = max(UINT32_C(1), dstWidth  >> mip);
            const uint32_t mipHeight = max(UINT32_C(1), dstHeight >> mip);

            dstDataSize += mipWidth * mipHeight * bytesPerPixel;
        }
        void* dstData = malloc(dstDataSize);
        MALLOC_CHECK(dstData);

        // Get source image offsets.
        uint32_t srcOffsets[CUBE_FACE_NUM][MAX_MIP_NUM];
        imageGetMipOffsets(srcOffsets, _src);

        for (uint8_t face = 0; face < 6; ++face)
        {
            for (uint8_t mip = 0; mip < _src.m_numMips; ++mip)
            {
                // Get src data ptr for current mip and face.
                const uint8_t* srcFaceData = (const uint8_t*)_src.m_data + srcOffsets[face][mip];

                // Get dst ptr for current mip level.
                uint8_t* dstMipData = (uint8_t*)dstData + dstMipOffsets[mip];

                // Advance by (srcPitch * faceIdx) to get to the desired face in the strip.
                const uint32_t srcMipSize = max(UINT32_C(1), _src.m_width >> mip);
                const uint32_t srcMipPitch = srcMipSize * bytesPerPixel;
                uint8_t* dstFaceData = (uint8_t*)dstMipData + srcMipPitch*face;

                const uint32_t dstMipPitch = srcMipPitch*6;

                for (uint32_t yy = 0; yy < srcMipSize; ++yy)
                {
                    const uint8_t* srcRowData = (const uint8_t*)srcFaceData + yy*srcMipPitch;
                    uint8_t* dstRowData = (uint8_t*)dstFaceData + yy*dstMipPitch;

                    memcpy(dstRowData, srcRowData, srcMipPitch);
                }
            }
        }

        // Fill image structure.
        Image result;
        result.m_width = dstWidth;
        result.m_height = dstHeight;
        result.m_dataSize = dstDataSize;
        result.m_format = _src.m_format;
        result.m_numMips = _src.m_numMips;
        result.m_numFaces = 1;
        result.m_data = dstData;

        // Output.
        imageMove(_dst, result);

        return true;
    }

    void imageHStripFromCubemap(Image& _cubemap)
    {
        Image tmp;
        if (imageHStripFromCubemap(tmp, _cubemap))
        {
            imageMove(_cubemap, tmp);
        }
    }

    bool imageCubemapFromHStrip(Image& _dst, const Image& _src)
    {
        // Input check.
        if(!imageIsHStrip(_src))
        {
            return false;
        }

        // Calculate destination offsets and alloc data.
        uint32_t dstDataSize = 0;
        uint32_t dstOffsets[CUBE_FACE_NUM][MAX_MIP_NUM];
        const uint32_t dstSize = _src.m_height;
        const uint32_t bytesPerPixel = getImageDataInfo(_src.m_format).m_bytesPerPixel;
        for (uint8_t face = 0; face < 6; ++face)
        {
            for (uint8_t mip = 0; mip < _src.m_numMips; ++mip)
            {
                dstOffsets[face][mip] = dstDataSize;
                const uint32_t mipSize = max(UINT32_C(1), dstSize >> mip);

                dstDataSize += mipSize * mipSize * bytesPerPixel;
            }
        }
        void* dstData = malloc(dstDataSize);
        MALLOC_CHECK(dstData);

        uint32_t srcOffsets[CUBE_FACE_NUM][MAX_MIP_NUM];
        imageGetMipOffsets(srcOffsets, _src);

        for (uint8_t face = 0; face < 6; ++face)
        {
            for (uint8_t mip = 0; mip < _src.m_numMips; ++mip)
            {
                // Get dst data ptr for current mip and face.
                uint8_t* dstFaceData = (uint8_t*)dstData + dstOffsets[face][mip];

                // Get src ptr for current mip level.
                const uint8_t* srcMipData = (const uint8_t*)_src.m_data + srcOffsets[0][mip];

                // Advance by (dstPitch * faceIdx) to get to the desired face in the strip.
                const uint32_t dstMipSize = max(UINT32_C(1), dstSize >> mip);
                const uint32_t dstMipPitch = dstMipSize * bytesPerPixel;
                const uint8_t* srcFaceData = (const uint8_t*)srcMipData + dstMipPitch*face;

                const uint32_t srcMipPitch = dstMipPitch*6;

                for (uint32_t yy = 0; yy < dstMipSize; ++yy)
                {
                    const uint8_t* srcRowData = (const uint8_t*)srcFaceData + yy*srcMipPitch;
                    uint8_t* dstRowData = (uint8_t*)dstFaceData + yy*dstMipPitch;

                    memcpy(dstRowData, srcRowData, dstMipPitch);
                }
            }
        }

        // Fill image structure.
        Image result;
        result.m_width = dstSize;
        result.m_height = dstSize;
        result.m_dataSize = dstDataSize;
        result.m_format = _src.m_format;
        result.m_numMips = _src.m_numMips;
        result.m_numFaces = 6;
        result.m_data = dstData;

        // Output.
        imageMove(_dst, result);

        return true;
    }

    void imageCubemapFromHStrip(Image& _image)
    {
        Image tmp;
        if (imageCubemapFromHStrip(tmp, _image))
        {
            imageMove(_image, tmp);
        }
    }

    bool imageFaceListFromCubemap(Image _faceList[6], const Image& _cubemap)
    {
        // Input check.
        if(!imageIsCubemap(_cubemap))
        {
            return false;
        }

        // Get destination sizes and offsets.
        uint32_t dstDataSize = 0;
        uint32_t dstMipOffsets[MAX_MIP_NUM];
        const uint8_t bytesPerPixel = getImageDataInfo(_cubemap.m_format).m_bytesPerPixel;
        for (uint8_t mip = 0; mip < _cubemap.m_numMips; ++mip)
        {
            dstMipOffsets[mip] = dstDataSize;
            const uint32_t mipSize = max(UINT32_C(1), _cubemap.m_width >> mip);
            dstDataSize += mipSize * mipSize * bytesPerPixel;
        }

        // Get source offsets.
        uint32_t cubemapOffsets[CUBE_FACE_NUM][MAX_MIP_NUM];
        imageGetMipOffsets(cubemapOffsets, _cubemap);

        for (uint8_t face = 0; face < 6; ++face)
        {
            void* dstData = malloc(dstDataSize);
            MALLOC_CHECK(dstData);

            for (uint8_t mip = 0; mip < _cubemap.m_numMips; ++mip)
            {
                const uint8_t* srcFaceData = (const uint8_t*)_cubemap.m_data + cubemapOffsets[face][mip];
                uint8_t* dstFaceData = (uint8_t*)dstData + dstMipOffsets[mip];

                const uint32_t mipFaceSize = max(UINT32_C(1), _cubemap.m_width >> mip);
                const uint32_t mipPitch = mipFaceSize * bytesPerPixel;

                for (uint32_t yy = 0; yy < mipFaceSize; ++yy)
                {
                    const uint8_t* srcRowData = (const uint8_t*)srcFaceData + yy*mipPitch;
                    uint8_t* dstRowData = (uint8_t*)dstFaceData + yy*mipPitch;

                    memcpy(dstRowData, srcRowData, mipPitch);
                }
            }

            // Fill image structure.
            Image result;
            result.m_width = _cubemap.m_width;
            result.m_height = _cubemap.m_height;
            result.m_dataSize = dstDataSize;
            result.m_format = _cubemap.m_format;
            result.m_numMips = _cubemap.m_numMips;
            result.m_numFaces = 1;
            result.m_data = dstData;

            // Output.
            imageMove(_faceList[face], result);
        }

        return true;
    }

    bool imageCubemapFromFaceList(Image& _cubemap, const Image _faceList[6])
    {
        // Input check.
        if (!imageValidCubemapFaceList(_faceList))
        {
            return false;
        }

        // Alloc destination data.
        const uint32_t dstDataSize = _faceList[0].m_dataSize * 6;
        void* dstData = malloc(dstDataSize);
        MALLOC_CHECK(dstData);

        // Get source offsets.
        uint32_t srcOffsets[CUBE_FACE_NUM][MAX_MIP_NUM];
        imageGetMipOffsets(srcOffsets, _faceList[0]);
        const uint32_t bytesPerPixel = getImageDataInfo(_faceList[0].m_format).m_bytesPerPixel;

        // Copy data.
        uint32_t destinationOffset = 0;
        for (uint8_t face = 0; face < 6; ++face)
        {
            const uint8_t* srcFaceData = (const uint8_t*)_faceList[face].m_data;
            for (uint8_t mip = 0; mip < _faceList[0].m_numMips; ++mip)
            {
                const uint8_t* srcMipData = (const uint8_t*)srcFaceData + srcOffsets[0][mip];
                uint8_t* dstMipData = (uint8_t*)dstData + destinationOffset;

                const uint32_t mipFaceSize = max(UINT32_C(1), _faceList[0].m_width >> mip);
                const uint32_t mipPitch = mipFaceSize * bytesPerPixel;
                const uint32_t mipFaceDataSize = mipPitch * mipFaceSize;

                destinationOffset += mipFaceDataSize;

                for (uint32_t yy = 0; yy < mipFaceSize; ++yy)
                {
                    const uint8_t* srcRowData = (const uint8_t*)srcMipData + yy*mipPitch;
                    uint8_t* dstRowData = (uint8_t*)dstMipData + yy*mipPitch;

                    memcpy(dstRowData, srcRowData, mipPitch);
                }
            }
        }

        // Fill image structure.
        Image result;
        result.m_width = _faceList[0].m_width;
        result.m_height = _faceList[0].m_height;
        result.m_dataSize = dstDataSize;
        result.m_format = _faceList[0].m_format;
        result.m_numMips = _faceList[0].m_numMips;
        result.m_numFaces = 6;
        result.m_data = dstData;

        // Output.
        imageMove(_cubemap, result);

        return true;
    }

    bool imageCrossFromCubemap(Image& _dst, const Image& _src, bool _vertical)
    {
        // Input check.
        if(!imageIsCubemap(_src))
        {
            return false;
        }

        // Copy source image.
        Image srcCpy;
        imageCopy(srcCpy, _src);

        // Transform -z image face properly.
        if (_vertical)
        {
            imageTransform(srcCpy, IMAGE_FACE_NEGATIVEZ | IMAGE_OP_FLIP_X | IMAGE_OP_FLIP_Y);
        }

        // Calculate destination offsets and alloc data.
        uint32_t dstDataSize = 0;
        uint32_t dstMipOffsets[MAX_MIP_NUM];
        const uint32_t dstWidth  = (_vertical?3:4) * srcCpy.m_width;
        const uint32_t dstHeight = (_vertical?4:3) * srcCpy.m_width;
        const uint32_t bytesPerPixel = getImageDataInfo(srcCpy.m_format).m_bytesPerPixel;
        for (uint8_t mip = 0; mip < srcCpy.m_numMips; ++mip)
        {
            dstMipOffsets[mip] = dstDataSize;
            const uint32_t mipWidth  = max(UINT32_C(1), dstWidth  >> mip);
            const uint32_t mipHeight = max(UINT32_C(1), dstHeight >> mip);

            dstDataSize += mipWidth * mipHeight * bytesPerPixel;
        }
        void* dstData = malloc(dstDataSize);
        MALLOC_CHECK(dstData);

        // Get black pixel.
        void* blackPixel = alloca(bytesPerPixel);
        const float blackPixelRgba32f[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        fromRgba32f(blackPixel, TextureFormat::Enum(srcCpy.m_format), blackPixelRgba32f);

        // Fill with black.
        for (uint8_t mip = 0; mip < srcCpy.m_numMips; ++mip)
        {
            const uint32_t mipWidth  = max(UINT32_C(1), dstWidth  >> mip);
            const uint32_t mipHeight = max(UINT32_C(1), dstHeight >> mip);
            const uint32_t mipPitch = mipWidth*bytesPerPixel;

            uint8_t* dstMipData = (uint8_t*)dstData + dstMipOffsets[mip];
            for (uint32_t yy = 0; yy < mipHeight; ++yy)
            {
                uint8_t* dstRowData = (uint8_t*)dstMipData + yy*mipPitch;
                for (uint32_t xx = 0; xx < mipWidth; ++xx)
                {
                    uint8_t* dstColumnData = (uint8_t*)dstRowData + xx*bytesPerPixel;
                    memcpy(dstColumnData, blackPixel, bytesPerPixel);
                }
            }
        }

        // Get source offsets.
        uint32_t srcOffsets[CUBE_FACE_NUM][MAX_MIP_NUM];
        imageGetMipOffsets(srcOffsets, srcCpy);

        for (uint8_t mip = 0; mip < srcCpy.m_numMips; ++mip)
        {
            const uint32_t srcWidth = max(UINT32_C(1), srcCpy.m_width >> mip);
            const uint32_t srcPitch = srcWidth * bytesPerPixel;

            const uint32_t mipWidth = max(UINT32_C(1), dstWidth >> mip);
            const uint32_t mipPitch = mipWidth * bytesPerPixel;
            const uint32_t faceSize = mipWidth / (_vertical?3:4);
            const uint32_t facePitch = faceSize * bytesPerPixel;
            const uint32_t rowDataSize = mipPitch * faceSize;

            // Destination offsets.
            uint32_t faceOffsets[6];
            if (_vertical)
            {
                //   ___ ___ ___
                //  |   |   |   | -> rowDataSize
                //  |___|___|___|
                //
                //   ___  -> facePitch
                //
                //       ___
                //      |Y+ |
                //   ___|___|___
                //  |X- |Z+ |X+ |
                //  |___|___|___|
                //      |Y- |
                //      |___|
                //      |Z- |
                //      |___|
                //
                faceOffsets[0] = rowDataSize + 2*facePitch; //+x
                faceOffsets[1] = rowDataSize;               //-x
                faceOffsets[2] = facePitch;                 //+y
                faceOffsets[3] = 2*rowDataSize + facePitch; //-y
                faceOffsets[4] = rowDataSize + facePitch;   //+z
                faceOffsets[5] = 3*rowDataSize + facePitch; //-z
            }
            else
            {
                //       ___
                //      |+Y |
                //   ___|___|___ ___
                //  |-X |+Z |+X |-Z |
                //  |___|___|___|___|
                //      |-Y |
                //      |___|
                //
                faceOffsets[0] = rowDataSize + 2*facePitch; //+x
                faceOffsets[1] = rowDataSize;               //-x
                faceOffsets[2] = facePitch;                 //+y
                faceOffsets[3] = 2*rowDataSize + facePitch; //-y
                faceOffsets[4] = rowDataSize + facePitch;   //+z
                faceOffsets[5] = rowDataSize + 3*facePitch; //-z
            }

            uint8_t* dstMipData = (uint8_t*)dstData + dstMipOffsets[mip];
            for (uint8_t face = 0; face < 6; ++face)
            {
                uint8_t* dstFaceData = (uint8_t*)dstMipData + faceOffsets[face];
                const uint8_t* srcFaceData = (uint8_t*)srcCpy.m_data + srcOffsets[face][mip];
                for (uint32_t yy = 0; yy < faceSize; ++yy)
                {
                    uint8_t* dstRowData = (uint8_t*)dstFaceData + yy*mipPitch;
                    const uint8_t* srcRowData = (const uint8_t*)srcFaceData + yy*srcPitch;

                    memcpy(dstRowData, srcRowData, facePitch);
                }
            }
        }

        // Fill image structure.
        Image result;
        result.m_width = dstWidth;
        result.m_height = dstHeight;
        result.m_dataSize = dstDataSize;
        result.m_format = srcCpy.m_format;
        result.m_numMips = srcCpy.m_numMips;
        result.m_numFaces = 1;
        result.m_data = dstData;

        // Output.
        imageMove(_dst, result);

        // Cleanup.
        imageUnload(srcCpy);

        return true;
    }

    void imageCrossFromCubemap(Image& _image, bool _vertical)
    {
        Image tmp;
        if (imageCrossFromCubemap(tmp, _image, _vertical))
        {
            imageMove(_image, tmp);
        }
    }

    // Image loading.
    //-----

    bool imageLoadDds(Image& _image, FILE* _fp)
    {
        CMFT_UNUSED size_t read;

        // Read magic.
        uint32_t magic;
        read = fread(&magic, sizeof(uint32_t), 1, _fp);
        DEBUG_CHECK(read == 1, "Could not read from file.");
        FERROR_CHECK(_fp);

        // Check magic.
        if (DDS_MAGIC != magic)
        {
            WARN("Dds magic invalid.");
            return false;
        }

        // Read header.
        DdsHeader ddsHeader;
        read = 0;
        read += fread(&ddsHeader.m_size,                      1, sizeof(ddsHeader.m_size),                      _fp);
        read += fread(&ddsHeader.m_flags,                     1, sizeof(ddsHeader.m_flags),                     _fp);
        read += fread(&ddsHeader.m_height,                    1, sizeof(ddsHeader.m_height),                    _fp);
        read += fread(&ddsHeader.m_width,                     1, sizeof(ddsHeader.m_width),                     _fp);
        read += fread(&ddsHeader.m_pitchOrLinearSize,         1, sizeof(ddsHeader.m_pitchOrLinearSize),         _fp);
        read += fread(&ddsHeader.m_depth,                     1, sizeof(ddsHeader.m_depth),                     _fp);
        read += fread(&ddsHeader.m_mipMapCount,               1, sizeof(ddsHeader.m_mipMapCount),               _fp);
        read += fread(&ddsHeader.m_reserved1,                 1, sizeof(ddsHeader.m_reserved1),                 _fp);
        read += fread(&ddsHeader.m_pixelFormat.m_size,        1, sizeof(ddsHeader.m_pixelFormat.m_size),        _fp);
        read += fread(&ddsHeader.m_pixelFormat.m_flags,       1, sizeof(ddsHeader.m_pixelFormat.m_flags),       _fp);
        read += fread(&ddsHeader.m_pixelFormat.m_fourcc,      1, sizeof(ddsHeader.m_pixelFormat.m_fourcc),      _fp);
        read += fread(&ddsHeader.m_pixelFormat.m_rgbBitCount, 1, sizeof(ddsHeader.m_pixelFormat.m_rgbBitCount), _fp);
        read += fread(&ddsHeader.m_pixelFormat.m_rBitMask,    1, sizeof(ddsHeader.m_pixelFormat.m_rBitMask),    _fp);
        read += fread(&ddsHeader.m_pixelFormat.m_gBitMask,    1, sizeof(ddsHeader.m_pixelFormat.m_gBitMask),    _fp);
        read += fread(&ddsHeader.m_pixelFormat.m_bBitMask,    1, sizeof(ddsHeader.m_pixelFormat.m_bBitMask),    _fp);
        read += fread(&ddsHeader.m_pixelFormat.m_aBitMask,    1, sizeof(ddsHeader.m_pixelFormat.m_aBitMask),    _fp);
        read += fread(&ddsHeader.m_caps,                      1, sizeof(ddsHeader.m_caps),                      _fp);
        read += fread(&ddsHeader.m_caps2,                     1, sizeof(ddsHeader.m_caps2),                     _fp);
        read += fread(&ddsHeader.m_caps3,                     1, sizeof(ddsHeader.m_caps3),                     _fp);
        read += fread(&ddsHeader.m_caps4,                     1, sizeof(ddsHeader.m_caps4),                     _fp);
        read += fread(&ddsHeader.m_reserved2,                 1, sizeof(ddsHeader.m_reserved2),                 _fp);
        DEBUG_CHECK(read == DDS_HEADER_SIZE, "Error reading file header.");
        FERROR_CHECK(_fp);

        // Read DdsDxt10 header if present.
        DdsHeaderDxt10 ddsHeaderDxt10;
        memset(&ddsHeaderDxt10, 0, sizeof(DdsHeaderDxt10));
        const bool hasDdsDxt10 = (DDS_DX10 == ddsHeader.m_pixelFormat.m_fourcc && (ddsHeader.m_flags&DDPF_FOURCC));
        if (hasDdsDxt10)
        {
            read = 0;
            read += fread(&ddsHeaderDxt10.m_dxgiFormat,        1, sizeof(ddsHeaderDxt10.m_dxgiFormat),        _fp);
            read += fread(&ddsHeaderDxt10.m_resourceDimension, 1, sizeof(ddsHeaderDxt10.m_resourceDimension), _fp);
            read += fread(&ddsHeaderDxt10.m_miscFlags,         1, sizeof(ddsHeaderDxt10.m_miscFlags),         _fp);
            read += fread(&ddsHeaderDxt10.m_arraySize,         1, sizeof(ddsHeaderDxt10.m_arraySize),         _fp);
            read += fread(&ddsHeaderDxt10.m_miscFlags2,        1, sizeof(ddsHeaderDxt10.m_miscFlags2),        _fp);
            DEBUG_CHECK(read == DDS_DX10_HEADER_SIZE, "Error reading Dds dx10 file header.");
            FERROR_CHECK(_fp);
        }

        // Validate header.
        if (DDS_HEADER_SIZE != ddsHeader.m_size)
        {
            WARN("Invalid Dds header size!");
            return false;
        }

        if ((ddsHeader.m_flags & (DDSD_CAPS|DDSD_HEIGHT|DDSD_WIDTH|DDSD_PIXELFORMAT)) != (DDSD_CAPS|DDSD_HEIGHT|DDSD_WIDTH|DDSD_PIXELFORMAT))
        {
            WARN("Invalid Dds header flags!");
            return false;
        }

        if (0 == (ddsHeader.m_caps & DDSCAPS_TEXTURE))
        {
            WARN("Invalid Dds header caps!");
            return false;
        }

        if (0 == ddsHeader.m_mipMapCount)
        {
            WARN("Dds image mipmap count is 0. Setting to 1.");
            ddsHeader.m_mipMapCount = 1;
        }

        const bool isCubemap = (0 != (ddsHeader.m_caps2 & DDSCAPS2_CUBEMAP));
        if (isCubemap && (DDS_CUBEMAP_ALLFACES != (ddsHeader.m_caps2 & DDS_CUBEMAP_ALLFACES)))
        {
            WARN("Partial cubemap not supported!");
            return false;
        }

        // Get format.
        TextureFormat::Enum format = TextureFormat::Unknown;
        if (hasDdsDxt10)
        {
            for (uint8_t ii = 0, end = CMFT_COUNTOF(s_translateDdsDxgiFormat); ii < end; ++ii)
            {
                if (s_translateDdsDxgiFormat[ii].m_dxgiFormat == ddsHeaderDxt10.m_dxgiFormat)
                {
                    format = s_translateDdsDxgiFormat[ii].m_textureFormat;
                    break;
                }
            }
        }
        else
        {
            uint32_t ddsBcFlag = 0;
            for (uint8_t ii = 0, end = CMFT_COUNTOF(s_translateDdsPfBitCount); ii < end; ++ii)
            {
                if (s_translateDdsPfBitCount[ii].m_bitCount == ddsHeader.m_pixelFormat.m_rgbBitCount)
                {
                    ddsBcFlag = s_translateDdsPfBitCount[ii].m_flag;
                    break;
                }
            }

            const uint32_t ddsFormat = ddsHeader.m_pixelFormat.m_flags & DDPF_FOURCC
                ? ddsHeader.m_pixelFormat.m_fourcc
                : (ddsHeader.m_pixelFormat.m_flags | ddsBcFlag)
                ;

            for (uint8_t ii = 0, end = CMFT_COUNTOF(s_translateDdsFormat); ii < end; ++ii)
            {
                if (s_translateDdsFormat[ii].m_format == ddsFormat)
                {
                    format = s_translateDdsFormat[ii].m_textureFormat;
                    break;
                }
            }
        }

        if (TextureFormat::Unknown == format)
        {
            const uint8_t bytesPerPixel = uint8_t(ddsHeader.m_pixelFormat.m_rgbBitCount/8);
            for (uint8_t ii = 0, end = CMFT_COUNTOF(s_ddsValidFormats); ii < end; ++ii)
            {
                if (bytesPerPixel == getImageDataInfo(s_ddsValidFormats[ii]).m_bytesPerPixel)
                {
                    format = TextureFormat::Enum(ii);
                }
            }

            WARN("DDS data format unknown. Guessing... Got %s.", getTextureFormatStr(format));

            if (TextureFormat::Unknown == format)
            {
                WARN("DDS data format not supported!");
                return false;
            }
        }

        // Calculate data size.
        const uint8_t numFaces = isCubemap ? 6 : 1;
        const uint32_t bytesPerPixel = getImageDataInfo(format).m_bytesPerPixel;
        uint32_t dataSize = 0;
        for (uint8_t face = 0; face < numFaces; ++face)
        {
            for (uint8_t mip = 0; mip < ddsHeader.m_mipMapCount; ++mip)
            {
                uint32_t width  = max(UINT32_C(1), ddsHeader.m_width  >> mip);
                uint32_t height = max(UINT32_C(1), ddsHeader.m_height >> mip);
                dataSize += width * height * bytesPerPixel;
            }
        }

        // Some software tools produce invalid dds file.
        // Flags claim there should be a ddsdxt10 header after dds header but in fact image data starts there.
        // Therefore, to handle those situations, image data size will be checked against remaining unread data size.

        // Current position in file.
        long int fpCurrentPos = ftell(_fp);

        // Remaining unread data size.
        fseek(_fp, 0L, SEEK_END);
        long int fpRemaining = ftell(_fp) - fpCurrentPos;

        // Seek back to currentPos or 20 before currentPos in case remaining unread data size does match image data size.
        fseek(_fp, fpCurrentPos - DDS_DX10_HEADER_SIZE*(fpRemaining == (long int)dataSize-DDS_DX10_HEADER_SIZE), SEEK_SET);

        // Alloc and read data.
        void* data = malloc(dataSize);
        MALLOC_CHECK(data);
        read = fread(data, 1, dataSize, _fp);
        DEBUG_CHECK(read == dataSize, "Could not read from file.");
        FERROR_CHECK(_fp);

        // Fill image structure.
        Image result;
        result.m_width = ddsHeader.m_width;
        result.m_height = ddsHeader.m_height;
        result.m_dataSize = dataSize;
        result.m_format = format;
        result.m_numMips = uint8_t(ddsHeader.m_mipMapCount);
        result.m_numFaces = numFaces;
        result.m_data = data;

        // Output.
        imageMove(_image, result);

        return true;
    }

    bool imageLoadKtx(Image& _image, FILE* _fp)
    {
        CMFT_UNUSED size_t read;
        CMFT_UNUSED int seek;

        KtxHeader ktxHeader;

        // Read magic.
        uint8_t magic[12];
        read = fread(&magic, KTX_MAGIC_LEN, 1, _fp);
        DEBUG_CHECK(read == 1, "Could not read from file.");
        FERROR_CHECK(_fp);

        const uint8_t ktxMagic[12] = KTX_MAGIC;
        if (0 != memcmp(magic, ktxMagic, KTX_MAGIC_LEN))
        {
            WARN("Ktx magic invalid.");
            return false;
        }

        // Read header.
        read = 0;
        read += fread(&ktxHeader.m_endianness,           1, sizeof(ktxHeader.m_endianness),           _fp);
        read += fread(&ktxHeader.m_glType,               1, sizeof(ktxHeader.m_glType),               _fp);
        read += fread(&ktxHeader.m_glTypeSize,           1, sizeof(ktxHeader.m_glTypeSize),           _fp);
        read += fread(&ktxHeader.m_glFormat,             1, sizeof(ktxHeader.m_glFormat),             _fp);
        read += fread(&ktxHeader.m_glInternalFormat,     1, sizeof(ktxHeader.m_glInternalFormat),     _fp);
        read += fread(&ktxHeader.m_glBaseInternalFormat, 1, sizeof(ktxHeader.m_glBaseInternalFormat), _fp);
        read += fread(&ktxHeader.m_pixelWidth,           1, sizeof(ktxHeader.m_pixelWidth),           _fp);
        read += fread(&ktxHeader.m_pixelHeight,          1, sizeof(ktxHeader.m_pixelHeight),          _fp);
        read += fread(&ktxHeader.m_pixelDepth,           1, sizeof(ktxHeader.m_pixelDepth),           _fp);
        read += fread(&ktxHeader.m_numArrayElements,     1, sizeof(ktxHeader.m_numArrayElements),     _fp);
        read += fread(&ktxHeader.m_numFaces,             1, sizeof(ktxHeader.m_numFaces),             _fp);
        read += fread(&ktxHeader.m_numMips,              1, sizeof(ktxHeader.m_numMips),              _fp);
        read += fread(&ktxHeader.m_bytesKeyValue,        1, sizeof(ktxHeader.m_bytesKeyValue),        _fp);
        DEBUG_CHECK(read == KTX_HEADER_SIZE, "Error reading Ktx file header.");
        FERROR_CHECK(_fp);

        if (0 == ktxHeader.m_numMips)
        {
            WARN("Ktx image mipmap count is 0. Setting to 1.");
            ktxHeader.m_numMips = 1;
        }

        // Get format.
        TextureFormat::Enum format = TextureFormat::Unknown;
        for (uint8_t ii = 0, end = CMFT_COUNTOF(s_translateKtxFormat); ii < end; ++ii)
        {
            if (s_translateKtxFormat[ii].m_glInternalFormat == ktxHeader.m_glInternalFormat)
            {
                format = s_translateKtxFormat[ii].m_textureFormat;
                break;
            }
        }

        if (TextureFormat::Unknown == format)
        {
            WARN("Ktx file internal format unknown.");
            return false;
        }

        const uint32_t bytesPerPixel = getImageDataInfo(format).m_bytesPerPixel;

        // Compute data offsets.
        uint32_t offsets[MAX_MIP_NUM][CUBE_FACE_NUM];
        uint32_t dataSize = 0;
        for (uint8_t face = 0; face < ktxHeader.m_numFaces; ++face)
        {
            for (uint8_t mip = 0; mip < ktxHeader.m_numMips; ++mip)
            {
                offsets[mip][face] = dataSize;
                const uint32_t width  = max(UINT32_C(1), ktxHeader.m_pixelWidth  >> mip);
                const uint32_t height = max(UINT32_C(1), ktxHeader.m_pixelHeight >> mip);
                dataSize += width * height * bytesPerPixel;
            }
        }

        // Alloc data.
        void* data = (void*)malloc(dataSize);
        MALLOC_CHECK(data);

        // Jump header key-value data.
        seek = fseek(_fp, ktxHeader.m_bytesKeyValue, SEEK_CUR);
        DEBUG_CHECK(0 == seek, "File seek error.");
        FERROR_CHECK(_fp);

        // Read data.
        for (uint8_t mip = 0; mip < ktxHeader.m_numMips; ++mip)
        {
            const uint32_t width  = max(UINT32_C(1), ktxHeader.m_pixelWidth  >> mip);
            const uint32_t height = max(UINT32_C(1), ktxHeader.m_pixelHeight >> mip);
            const uint32_t pitch = width * bytesPerPixel;

            // Read face size.
            uint32_t faceSize;
            read = fread(&faceSize, sizeof(uint32_t), 1, _fp);
            DEBUG_CHECK(read == 1, "Error reading Ktx data.");
            FERROR_CHECK(_fp);

            const uint32_t mipSize = faceSize * ktxHeader.m_numFaces;
            const uint32_t pitchRounding = (KTX_UNPACK_ALIGNMENT-1)-((pitch    + KTX_UNPACK_ALIGNMENT-1)&(KTX_UNPACK_ALIGNMENT-1));
            const uint32_t faceRounding  = (KTX_UNPACK_ALIGNMENT-1)-((faceSize + KTX_UNPACK_ALIGNMENT-1)&(KTX_UNPACK_ALIGNMENT-1));
            const uint32_t mipRounding   = (KTX_UNPACK_ALIGNMENT-1)-((mipSize  + KTX_UNPACK_ALIGNMENT-1)&(KTX_UNPACK_ALIGNMENT-1));

            if (faceSize != ((pitch + pitchRounding) * height))
            {
                WARN("Ktx face size invalid.");
            }

            for (uint8_t face = 0; face < ktxHeader.m_numFaces; ++face)
            {
                uint8_t* faceData = (uint8_t*)data + offsets[mip][face];

                if (0 == pitchRounding)
                {
                    // Read entire face at once.
                    read = fread(faceData, 1, faceSize, _fp);
                    DEBUG_CHECK(read == faceSize, "Error reading Ktx face data.");
                    FERROR_CHECK(_fp);
                }
                else
                {
                    // Read row by row.
                    for (uint32_t yy = 0; yy < ktxHeader.m_pixelHeight; ++yy)
                    {
                        // Read row.
                        uint8_t* dst = (uint8_t*)faceData + yy*pitch;
                        read = fread(dst, 1, pitch, _fp);
                        DEBUG_CHECK(read == pitch, "Error reading Ktx row data.");
                        FERROR_CHECK(_fp);

                        // Jump row rounding.
                        int seek = fseek(_fp, pitchRounding, SEEK_CUR);
                        BX_UNUSED(seek);
                        DEBUG_CHECK(0 == seek, "File seek error.");
                        FERROR_CHECK(_fp);
                    }
                }

                // Jump face rounding.
                int seek = fseek(_fp, faceRounding, SEEK_CUR);
                BX_UNUSED(seek);
                DEBUG_CHECK(0 == seek, "File seek error.");
                FERROR_CHECK(_fp);
            }

            // Jump mip rounding.
            int seek = fseek(_fp, mipRounding, SEEK_CUR);
            BX_UNUSED(seek);
            DEBUG_CHECK(0 == seek, "File seek error.");
            FERROR_CHECK(_fp);
        }

        // Fill image structure.
        Image result;
        result.m_width = ktxHeader.m_pixelWidth;
        result.m_height = ktxHeader.m_pixelHeight;
        result.m_dataSize = dataSize;
        result.m_format = format;
        result.m_numMips = uint8_t(ktxHeader.m_numMips);
        result.m_numFaces = uint8_t(ktxHeader.m_numFaces);
        result.m_data = data;

        // Output.
        imageMove(_image, result);

        return true;
    }

    bool imageLoadHdr(Image& _image, FILE* _fp)
    {
        CMFT_UNUSED char* get;
        CMFT_UNUSED size_t read;
        char buf[128];

        // Read first line.
        get = fgets(buf, sizeof(buf), _fp);
        DEBUG_CHECK(NULL != get, "Error reading first line of Hdr file.");

        // Check magic.
        if(0 != strncmp(buf, HDR_MAGIC_FULL, HDR_MAGIC_LEN))
        {
            WARN("HDR magic not valid.");
            return false;
        }

        HdrHeader hdrHeader;
        hdrHeader.m_valid = 0;
        hdrHeader.m_gamma = 1.0f;
        hdrHeader.m_exposure = 1.0f;

        // Read header.
        bool formatDefined = false;
        for(uint8_t ii = 0, stop = 20; ii < stop; ++ii)
        {
            // Read next line.
            get = fgets(buf, sizeof(buf), _fp);
            DEBUG_CHECK(NULL != get, "Error reading Hdr file header.");
            FERROR_CHECK(_fp);

            if ((0 == buf[0])
            || ('\n' == buf[0]))
            {
                // End of header.
                break;
            }
            else if (0 == strcmp(buf, "FORMAT=32-bit_rle_rgbe\n"))
            {
                formatDefined = true;
            }
            else if (1 == sscanf(buf, "GAMMA=%g", &hdrHeader.m_gamma))
            {
                hdrHeader.m_valid |= HDR_VALID_GAMMA;
            }
            else if (1 == sscanf(buf, "EXPOSURE=%g", &hdrHeader.m_exposure))
            {
                hdrHeader.m_valid |= HDR_VALID_EXPOSURE;
            }
        }

        if (!formatDefined)
        {
            WARN("Invalid Hdr header.");
        }

        // Read empty line (end of header).
        get = fgets(buf, sizeof(buf), _fp);
        DEBUG_CHECK(NULL != get, "Error reading end of Hdr file header.");
        FERROR_CHECK(_fp);

        // Read image size.
        int32_t width;
        int32_t height;
        read = sscanf(buf, "-Y %d +X %d", &height, &width);
        DEBUG_CHECK(2 == read, "Error reading Hdr image size.");

        // Allocate data.
        const uint32_t dataSize = width * height * 4 /* bytesPerChannel */;
        uint8_t* data = (uint8_t*)malloc(dataSize);
        MALLOC_CHECK(data);

        // Read first chunk.
        unsigned char rgbe[4];
        read = fread(rgbe, 4*sizeof(unsigned char), 1, _fp);
        DEBUG_CHECK(read == 1, "Could not read Hdr image data.");
        FERROR_CHECK(_fp);

        uint8_t* dataPtr = (uint8_t*)data;

        if ((width < 8)
        || (width > 0x7fff)
        || (rgbe[0] != 2)
        || (rgbe[1] != 2)
        || (rgbe[2] & 0x80))
        {
            // File not RLE.

            // Save already read pixel.
            dataPtr[0] = rgbe[0];
            dataPtr[1] = rgbe[1];
            dataPtr[2] = rgbe[2];
            dataPtr[3] = rgbe[3];
            dataPtr += 4;

            // Read rest of the file.
            const uint32_t remaningDataSize = dataSize - 4;
            read = fread(dataPtr, remaningDataSize, 1, _fp);
            DEBUG_CHECK(read == 1, "Error reading Hdr image data.");
            FERROR_CHECK(_fp);
        }
        else
        {
            // File is RLE.

            uint8_t* scanlineBuffer = (uint8_t*)alloca(width*4);
            MALLOC_CHECK(scanlineBuffer);
            uint8_t* ptr;
            const uint8_t* ptrEnd;
            uint32_t numScanlines = height-1;
            int32_t count;
            for (;;)
            {
                DEBUG_CHECK(((uint16_t(rgbe[2])<<8)|(rgbe[3]&0xff)) == width, "Hdr file scanline width is invalid.");

                ptr = scanlineBuffer;
                for (uint8_t ii = 0; ii < 4; ++ii)
                {
                    ptrEnd = (const uint8_t*)scanlineBuffer + width*(ii+1);
                    while (ptr < ptrEnd)
                    {
                        unsigned char rle[2];
                        read = fread(rle, sizeof(rle), 1, _fp);
                        DEBUG_CHECK(read == 1, "Error reading Hdr image data.");
                        FERROR_CHECK(_fp);

                        if (rle[0] > 128)
                        {
                            // RLE chunk.
                            count = rle[0] - 128;
                            DEBUG_CHECK((count != 0) && (count <= (ptrEnd - ptr)), "Bad scanline data!");
                            while (count-- > 0)
                            {
                                *ptr++ = rle[1];
                            }
                        }
                        else
                        {
                            // Normal chunk.
                            count = rle[0];
                            DEBUG_CHECK((count != 0) && (count <= (ptrEnd - ptr)), "Bad scanline data!");
                            *ptr++ = rle[1];
                            if (--count > 0)
                            {
                                read = fread(ptr, 1, count, _fp);
                                DEBUG_CHECK(int32_t(read) == count, "Error reading Hdr image data.");
                                FERROR_CHECK(_fp);
                                ptr += count;
                            }
                        }
                    }
                }

                // Copy scanline data.
                for (int32_t ii = 0; ii < width; ++ii)
                {
                    dataPtr[0] = scanlineBuffer[ii+(0*width)];
                    dataPtr[1] = scanlineBuffer[ii+(1*width)];
                    dataPtr[2] = scanlineBuffer[ii+(2*width)];
                    dataPtr[3] = scanlineBuffer[ii+(3*width)];
                    dataPtr += 4;
                }

                // Break if reached the end.
                if (0 == numScanlines--)
                {
                    break;
                }

                // Read next scanline.
                read = fread(rgbe, sizeof(rgbe), 1, _fp);
                DEBUG_CHECK(read == 1, "Could not read Hdr image data.");
                FERROR_CHECK(_fp);
            }
        }

        // Fill image structure.
        Image result;
        result.m_width = uint32_t(width);
        result.m_height = uint32_t(height);
        result.m_dataSize = dataSize;
        result.m_format = TextureFormat::RGBE;
        result.m_numMips = 1;
        result.m_numFaces = 1;
        result.m_data = (void*)data;

        // Output.
        imageMove(_image, result);

        return true;
    }

    bool imageLoadTga(Image& _image, FILE* _fp)
    {
        CMFT_UNUSED size_t read;
        CMFT_UNUSED int seek;

        // Load header.
        TgaHeader tgaHeader;
        read = 0;
        read += fread(&tgaHeader.m_idLength,        1, sizeof(tgaHeader.m_idLength),        _fp);
        read += fread(&tgaHeader.m_colorMapType,    1, sizeof(tgaHeader.m_colorMapType),    _fp);
        read += fread(&tgaHeader.m_imageType,       1, sizeof(tgaHeader.m_imageType),       _fp);
        read += fread(&tgaHeader.m_colorMapOrigin,  1, sizeof(tgaHeader.m_colorMapOrigin),  _fp);
        read += fread(&tgaHeader.m_colorMapLength,  1, sizeof(tgaHeader.m_colorMapLength),  _fp);
        read += fread(&tgaHeader.m_colorMapDepth,   1, sizeof(tgaHeader.m_colorMapDepth),   _fp);
        read += fread(&tgaHeader.m_xOrigin,         1, sizeof(tgaHeader.m_xOrigin),         _fp);
        read += fread(&tgaHeader.m_yOrigin,         1, sizeof(tgaHeader.m_yOrigin),         _fp);
        read += fread(&tgaHeader.m_width,           1, sizeof(tgaHeader.m_width),           _fp);
        read += fread(&tgaHeader.m_height,          1, sizeof(tgaHeader.m_height),          _fp);
        read += fread(&tgaHeader.m_bitsPerPixel,    1, sizeof(tgaHeader.m_bitsPerPixel),    _fp);
        read += fread(&tgaHeader.m_imageDescriptor, 1, sizeof(tgaHeader.m_imageDescriptor), _fp);
        DEBUG_CHECK(read == TGA_HEADER_SIZE, "Error reading file header.");
        FERROR_CHECK(_fp);

        // Check header.
        if(0 == (TGA_IT_RGB & tgaHeader.m_imageType))
        {
            WARN("Tga file is not true-color image.");
            return false;
        }

        // Get format.
        TextureFormat::Enum format;
        if (24 == tgaHeader.m_bitsPerPixel)
        {
            format = TextureFormat::BGR8;
            DEBUG_CHECK(0x0 == (tgaHeader.m_imageDescriptor&0xf), "Alpha channel not properly defined.");
        }
        else if (32 == tgaHeader.m_bitsPerPixel)
        {
            format = TextureFormat::BGRA8;
            DEBUG_CHECK(0x8 == (tgaHeader.m_imageDescriptor&0xf), "Alpha channel not properly defined.");
        }
        else
        {
            WARN("Non-supported Tga pixel depth - %u.", tgaHeader.m_bitsPerPixel);
            return false;
        }

        // Alloc data.
        const uint32_t numBytesPerPixel = tgaHeader.m_bitsPerPixel/8;
        const uint32_t numPixels = tgaHeader.m_width * tgaHeader.m_height;
        const uint32_t dataSize = numPixels * numBytesPerPixel;
        uint8_t* data = (uint8_t*)malloc(dataSize);
        MALLOC_CHECK(data);

        // Skip to data.
        const uint32_t skip = tgaHeader.m_idLength + (tgaHeader.m_colorMapType&0x1)*tgaHeader.m_colorMapLength;
        seek = fseek(_fp, skip, SEEK_CUR);
        DEBUG_CHECK(0 == seek, "File seek error.");
        FERROR_CHECK(_fp);

        // Load data.
        const bool bCompressed = (0 != (tgaHeader.m_imageType&TGA_IT_RLE));
        if (bCompressed)
        {
            uint8_t buf[5];
            uint32_t n = 0;
            uint8_t* dataPtr = data;
            while (n < numPixels)
            {
                read = fread(buf, numBytesPerPixel+1, 1, _fp);
                DEBUG_CHECK(read == 1, "Could not read from file.");
                FERROR_CHECK(_fp);

                const uint8_t count = buf[0] & 0x7f;

                memcpy(dataPtr, &buf[1], numBytesPerPixel);
                dataPtr += numBytesPerPixel;
                n++;

                if (buf[0] & 0x80)
                {
                    // RLE chunk.
                    for (uint8_t ii = 0; ii < count; ++ii)
                    {
                        memcpy(dataPtr, &buf[1], numBytesPerPixel);
                        dataPtr += numBytesPerPixel;
                        n++;
                    }
                }
                else
                {
                    // Normal chunk.
                    for (uint8_t ii = 0; ii < count; ++ii)
                    {
                        read = fread(buf, numBytesPerPixel, 1, _fp);
                        DEBUG_CHECK(read == 1, "Could not read from file.");
                        FERROR_CHECK(_fp);

                        memcpy(dataPtr, buf, numBytesPerPixel);
                        dataPtr += numBytesPerPixel;
                        n++;
                    }
                }
            }
        }
        else
        {
            read = fread(data, dataSize, 1, _fp);
            DEBUG_CHECK(read == 1, "Could not read from file.");
            FERROR_CHECK(_fp);
        }

        // Fill image structure.
        Image result;
        result.m_width = tgaHeader.m_width;
        result.m_height = tgaHeader.m_height;
        result.m_dataSize = dataSize;
        result.m_format = format;
        result.m_numMips = 1;
        result.m_numFaces = 1;
        result.m_data = data;

        // Flip if necessary.
        const uint32_t flip = 0
                            | (tgaHeader.m_imageDescriptor & TGA_DESC_HORIZONTAL ? IMAGE_OP_FLIP_Y : 0)
                            | (tgaHeader.m_imageDescriptor & TGA_DESC_VERTICAL   ? 0 : IMAGE_OP_FLIP_X)
                            ;
        if (flip)
        {
            imageTransform(result, flip);
        }

        // Output.
        imageMove(_image, result);

        return true;
    }

    static bool isTga(uint32_t _magic)
    {
        //byte 2 is imageType and must be: 1, 2, 3, 9, 10 or 11
        //byte 1 is colorMapType and must be 1 if imageType is 1 or 9, 0 otherwise
        const uint8_t colorMapType = uint8_t((_magic>> 8)&0xff);
        const uint8_t imageType    = uint8_t((_magic>>16)&0xff);
        switch(imageType)
        {
        case 1:
        case 9:
            return (1 == colorMapType);

        case 2:
        case 3:
        case 10:
        case 11:
            return (0 == colorMapType);
        };

        return false;
    }

    bool imageLoad(Image& _image, const char* _filePath, TextureFormat::Enum _convertTo)
    {
        CMFT_UNUSED size_t read;
        CMFT_UNUSED int seek;

        // Open file.
        FILE* fp = fopen(_filePath, "rb");
        if (NULL == fp)
        {
            WARN("Could not open file %s for reading.", _filePath);
            return false;
        }
        ScopeFclose cleanup(fp);

        // Read magic.
        uint32_t magic;
        read = fread(&magic, sizeof(uint32_t), 1, fp);
        DEBUG_CHECK(read == 1, "Could not read from file.");
        FERROR_CHECK(fp);

        // Seek to beginning.
        seek = fseek(fp, 0L, SEEK_SET);
        DEBUG_CHECK(0 == seek, "File seek error.");
        FERROR_CHECK(fp);

        // Load image.
        bool loaded = false;
        if (DDS_MAGIC == magic)
        {
            loaded = imageLoadDds(_image, fp);
        }
        else if (HDR_MAGIC == magic)
        {
            loaded = imageLoadHdr(_image, fp);
        }
        else if (KTX_MAGIC_SHORT == magic)
        {
            loaded = imageLoadKtx(_image, fp);
        }
        else if (isTga(magic))
        {
            loaded = imageLoadTga(_image, fp);
        }
        else
        {
            WARN("Could not load %s. Unknown file type.", _filePath);
            return false;
        }

        if (!loaded)
        {
            return false;
        }

        // Convert if necessary.
        if (TextureFormat::Unknown != _convertTo
        &&  _image.m_format != _convertTo)
        {
            imageConvert(_image, _convertTo);
        }

        return true;
    }

    // Image saving.
    //-----

    bool imageSaveDds(const char* _fileName, const Image& _image)
    {
        CMFT_UNUSED size_t write;

        DdsHeader ddsHeader;
        DdsHeaderDxt10 ddsHeaderDxt10;
        ddsHeaderFromImage(ddsHeader, &ddsHeaderDxt10, _image);

        // Open file.
        FILE* fp = fopen(_fileName, "wb");
        if (NULL == fp)
        {
            WARN("Could not open file %s for writing.", _fileName);
            return false;
        }
        ScopeFclose cleanup(fp);

        // Write magic.
        const uint32_t magic = DDS_MAGIC;
        write = fwrite(&magic, 1, 4, fp);
        DEBUG_CHECK(write == sizeof(magic), "Error writing Dds magic.");
        FERROR_CHECK(fp);

        // Write header.
        write = 0;
        write += fwrite(&ddsHeader.m_size,                      1, sizeof(ddsHeader.m_size),                      fp);
        write += fwrite(&ddsHeader.m_flags,                     1, sizeof(ddsHeader.m_flags),                     fp);
        write += fwrite(&ddsHeader.m_height,                    1, sizeof(ddsHeader.m_height),                    fp);
        write += fwrite(&ddsHeader.m_width,                     1, sizeof(ddsHeader.m_width),                     fp);
        write += fwrite(&ddsHeader.m_pitchOrLinearSize,         1, sizeof(ddsHeader.m_pitchOrLinearSize),         fp);
        write += fwrite(&ddsHeader.m_depth,                     1, sizeof(ddsHeader.m_depth),                     fp);
        write += fwrite(&ddsHeader.m_mipMapCount,               1, sizeof(ddsHeader.m_mipMapCount),               fp);
        write += fwrite(&ddsHeader.m_reserved1,                 1, sizeof(ddsHeader.m_reserved1),                 fp);
        write += fwrite(&ddsHeader.m_pixelFormat.m_size,        1, sizeof(ddsHeader.m_pixelFormat.m_size),        fp);
        write += fwrite(&ddsHeader.m_pixelFormat.m_flags,       1, sizeof(ddsHeader.m_pixelFormat.m_flags),       fp);
        write += fwrite(&ddsHeader.m_pixelFormat.m_fourcc,      1, sizeof(ddsHeader.m_pixelFormat.m_fourcc),      fp);
        write += fwrite(&ddsHeader.m_pixelFormat.m_rgbBitCount, 1, sizeof(ddsHeader.m_pixelFormat.m_rgbBitCount), fp);
        write += fwrite(&ddsHeader.m_pixelFormat.m_rBitMask,    1, sizeof(ddsHeader.m_pixelFormat.m_rBitMask),    fp);
        write += fwrite(&ddsHeader.m_pixelFormat.m_gBitMask,    1, sizeof(ddsHeader.m_pixelFormat.m_gBitMask),    fp);
        write += fwrite(&ddsHeader.m_pixelFormat.m_bBitMask,    1, sizeof(ddsHeader.m_pixelFormat.m_bBitMask),    fp);
        write += fwrite(&ddsHeader.m_pixelFormat.m_aBitMask,    1, sizeof(ddsHeader.m_pixelFormat.m_aBitMask),    fp);
        write += fwrite(&ddsHeader.m_caps,                      1, sizeof(ddsHeader.m_caps),                      fp);
        write += fwrite(&ddsHeader.m_caps2,                     1, sizeof(ddsHeader.m_caps2),                     fp);
        write += fwrite(&ddsHeader.m_caps3,                     1, sizeof(ddsHeader.m_caps3),                     fp);
        write += fwrite(&ddsHeader.m_caps4,                     1, sizeof(ddsHeader.m_caps4),                     fp);
        write += fwrite(&ddsHeader.m_reserved2,                 1, sizeof(ddsHeader.m_reserved2),                 fp);
        DEBUG_CHECK(write == DDS_HEADER_SIZE, "Error writing Dds file header.");
        FERROR_CHECK(fp);

        if (DDS_DX10 == ddsHeader.m_pixelFormat.m_fourcc)
        {
            write = 0;
            write += fwrite(&ddsHeaderDxt10.m_dxgiFormat,        1, sizeof(ddsHeaderDxt10.m_dxgiFormat),        fp);
            write += fwrite(&ddsHeaderDxt10.m_resourceDimension, 1, sizeof(ddsHeaderDxt10.m_resourceDimension), fp);
            write += fwrite(&ddsHeaderDxt10.m_miscFlags,         1, sizeof(ddsHeaderDxt10.m_miscFlags),         fp);
            write += fwrite(&ddsHeaderDxt10.m_arraySize,         1, sizeof(ddsHeaderDxt10.m_arraySize),         fp);
            write += fwrite(&ddsHeaderDxt10.m_miscFlags2,        1, sizeof(ddsHeaderDxt10.m_miscFlags2),        fp);
            DEBUG_CHECK(write == DDS_DX10_HEADER_SIZE, "Error writing Dds dx10 file header.");
            FERROR_CHECK(fp);
        }

        // Write data.
        DEBUG_CHECK(NULL != _image.m_data, "Image data is null.");
        write = fwrite(_image.m_data, 1, _image.m_dataSize, fp);
        DEBUG_CHECK(write == _image.m_dataSize, "Error writing Dds image data.");
        FERROR_CHECK(fp);

        return true;
    }

    bool imageSaveKtx(const char* _fileName, const Image& _image)
    {
        KtxHeader ktxHeader;
        ktxHeaderFromImage(ktxHeader, _image);

        // Open file.
        FILE* fp = fopen(_fileName, "wb");
        if (NULL == fp)
        {
            WARN("Could not open file %s for writing.", _fileName);
            return false;
        }
        ScopeFclose cleanup(fp);

        CMFT_UNUSED size_t write;

        // Write magic.
        const uint8_t magic[KTX_MAGIC_LEN+1] = KTX_MAGIC;
        write = fwrite(&magic, 1, KTX_MAGIC_LEN, fp);
        DEBUG_CHECK(write == KTX_MAGIC_LEN, "Error writing Ktx magic.");
        FERROR_CHECK(fp);

        // Write header.
        write = 0;
        write += fwrite(&ktxHeader.m_endianness,           1, sizeof(ktxHeader.m_endianness),           fp);
        write += fwrite(&ktxHeader.m_glType,               1, sizeof(ktxHeader.m_glType),               fp);
        write += fwrite(&ktxHeader.m_glTypeSize,           1, sizeof(ktxHeader.m_glTypeSize),           fp);
        write += fwrite(&ktxHeader.m_glFormat,             1, sizeof(ktxHeader.m_glFormat),             fp);
        write += fwrite(&ktxHeader.m_glInternalFormat,     1, sizeof(ktxHeader.m_glInternalFormat),     fp);
        write += fwrite(&ktxHeader.m_glBaseInternalFormat, 1, sizeof(ktxHeader.m_glBaseInternalFormat), fp);
        write += fwrite(&ktxHeader.m_pixelWidth,           1, sizeof(ktxHeader.m_pixelWidth),           fp);
        write += fwrite(&ktxHeader.m_pixelHeight,          1, sizeof(ktxHeader.m_pixelHeight),          fp);
        write += fwrite(&ktxHeader.m_pixelDepth,           1, sizeof(ktxHeader.m_pixelDepth),           fp);
        write += fwrite(&ktxHeader.m_numArrayElements,     1, sizeof(ktxHeader.m_numArrayElements),     fp);
        write += fwrite(&ktxHeader.m_numFaces,             1, sizeof(ktxHeader.m_numFaces),             fp);
        write += fwrite(&ktxHeader.m_numMips,              1, sizeof(ktxHeader.m_numMips),              fp);
        write += fwrite(&ktxHeader.m_bytesKeyValue,        1, sizeof(ktxHeader.m_bytesKeyValue),        fp);
        DEBUG_CHECK(write == KTX_HEADER_SIZE, "Error writing Ktx header.");

        // Get source offsets.
        uint32_t offsets[CUBE_FACE_NUM][MAX_MIP_NUM];
        imageGetMipOffsets(offsets, _image);

        const uint32_t bytesPerPixel = getImageDataInfo(_image.m_format).m_bytesPerPixel;
        const uint8_t pad[4] = { 0, 0, 0, 0 };

        // Write data.
        DEBUG_CHECK(NULL != _image.m_data, "Image data is null.");
        for (uint8_t mip = 0; mip < _image.m_numMips; ++mip)
        {
            const uint32_t width  = max(UINT32_C(1), _image.m_width  >> mip);
            const uint32_t height = max(UINT32_C(1), _image.m_height >> mip);

            const uint32_t pitch = width * bytesPerPixel;
            const uint32_t faceSize = pitch * height;
            const uint32_t mipSize = faceSize * _image.m_numFaces;

            const uint32_t pitchRounding = (KTX_UNPACK_ALIGNMENT-1)-((pitch    + KTX_UNPACK_ALIGNMENT-1)&(KTX_UNPACK_ALIGNMENT-1));
            const uint32_t faceRounding  = (KTX_UNPACK_ALIGNMENT-1)-((faceSize + KTX_UNPACK_ALIGNMENT-1)&(KTX_UNPACK_ALIGNMENT-1));
            const uint32_t mipRounding   = (KTX_UNPACK_ALIGNMENT-1)-((mipSize  + KTX_UNPACK_ALIGNMENT-1)&(KTX_UNPACK_ALIGNMENT-1));

            // Write face size.
            write = fwrite(&faceSize, sizeof(uint32_t), 1, fp);
            DEBUG_CHECK(write == 1, "Error writing Ktx data.");
            FERROR_CHECK(fp);

            for (uint8_t face = 0; face < _image.m_numFaces; ++face)
            {
                const uint8_t* faceData = (const uint8_t*)_image.m_data + offsets[face][mip];

                if (0 == pitchRounding)
                {
                    // Write entire face at once.
                    write = fwrite(faceData, 1, faceSize, fp);
                    DEBUG_CHECK(write == faceSize, "Error writing Ktx face data.");
                    FERROR_CHECK(fp);
                }
                else
                {
                    // Write row by row.
                    for (uint32_t yy = 0; yy < height; ++yy)
                    {
                        // Write row.
                        const uint8_t* src = (const uint8_t*)faceData + yy*pitch;
                        write = fwrite(src, 1, pitch, fp);
                        DEBUG_CHECK(write == pitch, "Error writing Ktx row data.");
                        FERROR_CHECK(fp);

                        // Write row rounding.
                        write = fwrite(&pad, 1, pitchRounding, fp);
                        DEBUG_CHECK(write == pitchRounding, "Error writing Ktx row rounding.");
                        FERROR_CHECK(fp);
                    }
                }

                // Write face rounding.
                if (faceRounding)
                {
                    write = fwrite(&pad, 1, faceRounding, fp);
                    DEBUG_CHECK(write == faceRounding, "Error writing Ktx face rounding.");
                    FERROR_CHECK(fp);
                }
            }

            // Write mip rounding.
            if (mipRounding)
            {
                write = fwrite(&pad, 1, mipRounding, fp);
                DEBUG_CHECK(write == mipRounding, "Error writing Ktx mip rounding.");
                FERROR_CHECK(fp);
            }
        }

        return true;
    }

    bool imageSaveHdr(const char* _fileName, const Image& _image)
    {
        // Hdr file type assumes rgbe image format.
        Image imageRgbe;
        const bool imageIsRef = imageRefOrConvert(imageRgbe, TextureFormat::RGBE, _image);

        // Open file.
        FILE* fp = fopen(_fileName, "wb");
        if (NULL == fp)
        {
            WARN("Could not open file %s for writing.", _fileName);
            return false;
        }
        ScopeFclose cleanup(fp);

        if (1 != imageRgbe.m_numFaces)
        {
            WARN("Image seems to be containing more than one face. "
                 "Only the first one will be saved due to the limits of HDR format."
                );
        }

        if (1 != imageRgbe.m_numMips)
        {
            WARN("Image seems to be containing more than one mip map. "
                 "Only the first one will be saved due to the limits of HDR format."
                );
        }

        HdrHeader hdrHeader;
        hdrHeaderFromImage(hdrHeader, imageRgbe);

        CMFT_UNUSED size_t write = 0;

        // Write magic.
        char magic[HDR_MAGIC_LEN+1] = HDR_MAGIC_FULL;
        magic[HDR_MAGIC_LEN] = '\n';
        write = fwrite(&magic, HDR_MAGIC_LEN+1, 1, fp);
        DEBUG_CHECK(write == 1, "Error writing Hdr magic.");
        FERROR_CHECK(fp);

        // Write comment.
        char comment[21] = "# Output from cmft.\n";
        write = fwrite(&comment, 20, 1, fp);
        DEBUG_CHECK(write == 1, "Error writing Hdr comment.");
        FERROR_CHECK(fp);

        // Write format.
        const char format[24] = "FORMAT=32-bit_rle_rgbe\n";
        write = fwrite(&format, 23, 1, fp);
        DEBUG_CHECK(write == 1, "Error writing Hdr format.");
        FERROR_CHECK(fp);

        // Don't write gamma for now...
        //char gamma[32];
        //sprintf(gamma, "GAMMA=%g\n", hdrHeader.m_gamma);
        //const size_t gammaLen = strlen(gamma);
        //write = fwrite(&gamma, gammaLen, 1, fp);
        //DEBUG_CHECK(write == 1, "Error writing Hdr gamma.");
        //FERROR_CHECK(fp);

        // Write exposure.
        char exposure[32];
        sprintf(exposure, "EXPOSURE=%g\n", hdrHeader.m_exposure);
        const size_t exposureLen = strlen(exposure);
        write = fwrite(&exposure, exposureLen, 1, fp);
        DEBUG_CHECK(write == 1, "Error writing Hdr exposure.");
        FERROR_CHECK(fp);

        // Write header terminator.
        char headerTerminator = '\n';
        write = fwrite(&headerTerminator, 1, 1, fp);
        DEBUG_CHECK(write == 1, "Error writing Hdr header terminator.");
        FERROR_CHECK(fp);

        // Write image size.
        char imageSize[32];
        sprintf(imageSize, "-Y %d +X %d\n", imageRgbe.m_height, imageRgbe.m_width);
        const size_t imageSizeLen = strlen(imageSize);
        write = fwrite(&imageSize, imageSizeLen, 1, fp);
        DEBUG_CHECK(write == 1, "Error writing Hdr image size.");
        FERROR_CHECK(fp);

        // Write data. //TODO: implement RLE option.
        DEBUG_CHECK(NULL != imageRgbe.m_data, "Image data is null.");
        const uint32_t pitch = imageRgbe.m_width * getImageDataInfo(imageRgbe.m_format).m_bytesPerPixel;
        const uint8_t* srcPtr = (uint8_t*)imageRgbe.m_data;
        for (uint32_t yy = 0; yy < imageRgbe.m_height; ++yy)
        {
            write = fwrite(srcPtr, 1, pitch, fp);
            DEBUG_CHECK(write == pitch, "Error writing Hdr data.");
            FERROR_CHECK(fp);
            srcPtr+=pitch;
        }

        // Cleanup.
        if (!imageIsRef)
        {
            imageUnload(imageRgbe);
        }

        return true;
    }

    bool imageSaveTga(const char* _fileName, const Image& _image, bool _yflip = true)
    {
        // Open file.
        FILE* fp = fopen(_fileName, "wb");
        if (NULL == fp)
        {
            WARN("Could not open file %s for writing.", _fileName);
            return false;
        }
        ScopeFclose cleanup(fp);

        if (1 != _image.m_numFaces)
        {
            WARN("Image seems to be containing more than one face. "
                 "Only the first one will be saved due to the limits of TGA format."
                );
        }

        if (1 != _image.m_numMips)
        {
            WARN("Image seems to be containing more than one mip map. "
                 "Only the first one will be saved due to the limits of TGA format."
                );
        }

        TgaHeader tgaHeader;
        tgaHeaderFromImage(tgaHeader, _image);

        // Write header.
        CMFT_UNUSED size_t write = 0;
        write += fwrite(&tgaHeader.m_idLength,        1, sizeof(tgaHeader.m_idLength),        fp);
        write += fwrite(&tgaHeader.m_colorMapType,    1, sizeof(tgaHeader.m_colorMapType),    fp);
        write += fwrite(&tgaHeader.m_imageType,       1, sizeof(tgaHeader.m_imageType),       fp);
        write += fwrite(&tgaHeader.m_colorMapOrigin,  1, sizeof(tgaHeader.m_colorMapOrigin),  fp);
        write += fwrite(&tgaHeader.m_colorMapLength,  1, sizeof(tgaHeader.m_colorMapLength),  fp);
        write += fwrite(&tgaHeader.m_colorMapDepth,   1, sizeof(tgaHeader.m_colorMapDepth),   fp);
        write += fwrite(&tgaHeader.m_xOrigin,         1, sizeof(tgaHeader.m_xOrigin),         fp);
        write += fwrite(&tgaHeader.m_yOrigin,         1, sizeof(tgaHeader.m_yOrigin),         fp);
        write += fwrite(&tgaHeader.m_width,           1, sizeof(tgaHeader.m_width),           fp);
        write += fwrite(&tgaHeader.m_height,          1, sizeof(tgaHeader.m_height),          fp);
        write += fwrite(&tgaHeader.m_bitsPerPixel,    1, sizeof(tgaHeader.m_bitsPerPixel),    fp);
        write += fwrite(&tgaHeader.m_imageDescriptor, 1, sizeof(tgaHeader.m_imageDescriptor), fp);
        DEBUG_CHECK(write == TGA_HEADER_SIZE, "Error writing Tga header.");
        FERROR_CHECK(fp);

        // Write data. //TODO: implement RLE option.
        DEBUG_CHECK(NULL != _image.m_data, "Image data is null.");
        const uint32_t pitch = _image.m_width * getImageDataInfo(_image.m_format).m_bytesPerPixel;
        if (_yflip)
        {
            const uint8_t* src = (uint8_t*)_image.m_data + _image.m_height * pitch;
            for (uint32_t yy = 0; yy < _image.m_height; ++yy)
            {
                src-=pitch;
                write = fwrite(src, 1, pitch, fp);
                DEBUG_CHECK(write == pitch, "Error writing Tga data.");
                FERROR_CHECK(fp);
            }
        }
        else
        {
            const uint8_t* src = (uint8_t*)_image.m_data;
            for (uint32_t yy = 0; yy < _image.m_height; ++yy)
            {
                write = fwrite(src, 1, pitch, fp);
                DEBUG_CHECK(write == pitch, "Error writing Tga data.");
                FERROR_CHECK(fp);
                src+=pitch;
            }
        }

        // Write footer.
        TgaFooter tgaFooter = { 0, 0, TGA_ID };
        write  = fwrite(&tgaFooter.m_extensionOffset, 1, sizeof(tgaFooter.m_extensionOffset), fp);
        write += fwrite(&tgaFooter.m_developerOffset, 1, sizeof(tgaFooter.m_developerOffset), fp);
        write += fwrite(&tgaFooter.m_signature,       1, sizeof(tgaFooter.m_signature),       fp);
        DEBUG_CHECK(TGA_FOOTER_SIZE == write, "Error writing Tga footer.");
        FERROR_CHECK(fp);

        return true;
    }

    bool imageSave(const Image& _image, const char* _fileName, ImageFileType::Enum _ft, TextureFormat::Enum _convertTo)
    {
        // Get image in desired format.
        Image image;
        bool imageIsRef;
        if (TextureFormat::Unknown != _convertTo)
        {
            imageIsRef = imageRefOrConvert(image, _convertTo, _image);
        }
        else
        {
            imageRef(image, _image);
            imageIsRef = true;
        }

        // Append appropriate extension to file name.
        char filePath[512];
        strcpy(filePath, _fileName);
        strcat(filePath, getFilenameExtensionStr(_ft));

        // Check for valid texture format and save.
        bool result = false;
        if (checkValidInternalFormat(_ft, image.m_format))
        {
            if (ImageFileType::DDS == _ft)
            {
                result = imageSaveDds(filePath, image);
            }
            else if (ImageFileType::KTX == _ft)
            {
                result = imageSaveKtx(filePath, image);
            }
            else if (ImageFileType::TGA == _ft)
            {
                result = imageSaveTga(filePath, image);
            }
            else if (ImageFileType::HDR == _ft)
            {
                result = imageSaveHdr(filePath, image);
            }
        }
        else
        {
            char buf[1024];
            getValidTextureFormatsStr(buf, _ft);

            WARN("Could not save %s as *.%s image."
                " Valid internal formats are: %s."
                " Choose one of the valid internal formats or a different file type.\n"
                , getTextureFormatStr(image.m_format)
                , getFilenameExtensionStr(_ft)
                , buf
                );
        }

        // Cleanup.
        if (!imageIsRef)
        {
            imageUnload(image);
        }

        return result;
    }

} // namespace cmft

/* vim: set sw=4 ts=4 expandtab: */
