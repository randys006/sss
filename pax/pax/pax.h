/****************************************************************************************************************
 * @defgroup PAX A C++ library for PAX files
 * @file     pax.h
 * @author   Randy J. Spaulding <randys006@gmail.com>
 * @version  0.1
 *
 * @section LICENSE
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the GNU
 * General Public License for more details at
 * https://www.gnu.org/copyleft/gpl.html
 *
 * @section DESCRIPTION
 *
 * A single-header-file library containing a C++17 driver for the PAX                                               
 * (Portable Arbitrary map eXtended) file format.
 *
 * @section PREREQUISITES
 *
 * Compiler having c++17 capability. Examples:
 *  - Visual Studio 2017 (15.9.17)
 *  - gcc 7
 *  - Clang 5
 * Doxygen 1.8.17 (for rendering documentation)
 *
 * @section OVERVIEW
 *
 * PAX was developed for the purpose of having a simple human-readable file format that contains both raster
 * data and metadata. The design is based on the image format pam from netpbm:
 * https://sourceforge.net/p/netpbm/wiki/Home/
 *
 * The fundamental tenets of PAX are listed in the sample file below:
 * @verbatim
 * P100 : V0.1 : PAX_META_ONLY
 * #
 * # A PAX file consists of a text header containing, in this order:
 * #   - A type description tag
 * #   - Single Linefeed (LF) '\n'
 * #   - Comments and metadata, each terminated by a LF
 * #   - Dimension tags and dimension lengths
 * #   - Binary raster data
 * #
 * # Every line has a type which is defined by the first character on the line:
 * #   - Comment lines begin with a hashtag '#'.
 * #   - Metadata lines begin with a percent symbol '%'.
 * #   - Any other character must be a valid header tag.
 * #
 * # Valid whitespace characters are space, tab, and CR.
 * # Leading whitespace is allowed, but should only be used for column-formatting.
 * # Empty lines are not allowed; there must always be at least once hashtag on each line.
 * #
 * # The rules comprising the PAX design paradigm are:
 * #    - The PAX header must be human-readable, clean, complete, and self-contained. All header tags
 * #      follow a specific syntax. Numeric tag indexes are specified in one of two ways:
 * #        - Tags from 1-20 can be specified using <value> such as first, thirteenth, etc.
 * #        - All tags can be specified using <number><postfix> such as 1st, 13th, etc.
 * #    - PAX is strongly-typed according to a set of types that is defined internally.
 * #    - The raster consists of a multidimensional array of elements using a generic type system.
 * #        - Each element contains a specified number of values, e.g. X, Y, and Z.
 * #        - Each value contains a specified number of bytes, e.g. 4 for 32-bit int, 8 for double, etc.
 * #        - Pax libraries must decide how to map pax types to language-specific internal types.
 * #      specified number of bytes. The elements are arranged into a specified number of dimensions.
 * #    - PAX avoids using excessive delimiters and markup.
 * #    - PAX allows an unlimited number of dimensions.
 * #        - Language-specific libraries must support a minimum of 2^32 dimensions.
 * #    - PAX allows an unlimited number of elements per dimension.
 * #        - Language-specific libraries must support a minimum of 2^32 elements per dimension.
 * #            - The lone exception to this is the PAX_STRING type, which can omit all dimension tags since
 * #              it only contains a single string by definition.
 * #    - Metadata types include signed and unsigned integers up to 64-bit, single- and double-precision
 * #      floating point, single- and double-precision complex, and simple strings.
 * #        - string metadata are terminated with a LF.
 * #    - Tags and keywords are case-insensitive.
 * #
 * % pi [ double ] = 3.14
 * % getty [string] = Four score and seven years ago...
 * @endverbatim
 *
 * Desired feature list:
 *  - Improve dimension index detection (tags such as '2st' shouldn't work)
 *  - International support
 *
 ***********************************************************************************************************/

#include <complex>
#ifdef _WIN32
#include <direct.h>
#include <fcntl.h>
#include <io.h>
#endif
#include <iomanip>
#include <iostream>
#include <list>
#include <map>
#include <numeric>
#include <regex>
#include <unordered_map>
#include <variant>

/************************************************************************************************************
 * The primary namespace for Spaulding Scientific Solutions
 ***********************************************************************************************************/
namespace sss {

/************************************************************************************************************
 * The primary namespace for all things PAX
 ***********************************************************************************************************/
namespace pax {

inline constexpr float PAX_VERSION{ 1.00 };
inline constexpr char   PAX_DATE[]{ "Dec 11, 2019" };

//////////////////////////////////////////////////////////////////////////
//
// PAX abstraction layer for cross-platform string support
// These macros select wide- or normal-char versions and ATL vs standard types
// With a few extra helper functions, too!
//
#ifdef _WIN32

#ifdef UNICODE
#define pax_ArgvType     wchar_t**
#define pax_T(x)         L ## x
#define pax_char         wchar_t
#define pax_filechar     wchar_t
#define pax_filestring   std::wstring
#define pax_strcmp       wcscmp
#define pax_stricmp      _wstricmp
#define pax_strncmp      strncmp
#define pax_strnicmp     strnicmp
#define pax_strncat      wscncat 
#define pax_strlen       wcslen
#define pax_strcpy       wcscpy
#define pax_strncpy      wcsncpy
#define pax_sprintf      wsprintf
#define pax_open         _wopen
#define pax_remove       _wremove
#define pax_getcwd       _getcwd
#define pax_lseek        _lseek
#define pax_read         _read
#define pax_write        _write
#define pax_close        _close
#define pax_stringstream std::wstringstream
#define pax_cout         wcout
#define pax_cerr         wcerr
#define pax_nullfile     L"nul"
#define pax_usleep(us)   std::this_thread::sleep_for(std::chrono::microseconds(us));
#define _L(str)          L ## str

#else
  // Not unicode...

#define pax_ArgvType     char**
#define pax_T(x)         L ## x
#define pax_char         char
#define pax_filechar     char
#define pax_filestring   std::string
#define pax_strcmp       strcmp
#define pax_stricmp      _stricmp
#define pax_strncmp      strncmp
#define pax_strnicmp     _strnicmp
#define pax_strncat      strncat 
#define pax_strlen       strlen
#define pax_strcpy       strcpy
#define pax_strncpy      strncpy
#define pax_sprintf      sprintf
#define pax_open         _open
#define pax_remove       remove
#define pax_getcwd       _getcwd
#define pax_lseek        _lseek
#define pax_read         _read
#define pax_write        _write
#define pax_close        _close
#define pax_stringstream std::stringstream
#define pax_cout         std::cout
#define pax_cerr         std::cerr
#define pax_nullfile     "nul"
#define pax_mkdir(a,b)   _mkdir(a)
#define pax_stat         _stat
#define pax_ifdir        _S_IFDIR
#ifndef mode_t
#define mode_t           int32_t
#endif
#define pax_usleep(us)   std::this_thread::sleep_for(std::chrono::microseconds(us));
#define _L(str)          str

#endif

#else

#define pax_ArgvType     char**
#define pax_T(x)         x
#define pax_char         char
#define pax_filechar     char
#define pax_filestring   std::string
#define pax_strcmp       strcmp
#define pax_stricmp      strcasecmp
#define pax_strncmp      strncmp
#define pax_strnicmp     strnicmp
#define pax_strncat      strncat 
#define pax_strlen       strlen
#define pax_strcpy       strcpy
#define pax_strncpy      strncpy
#define pax_sprintf      sprintf
#define pax_open         open
#define _open            open
#define pax_remove       remove
#define pax_getcwd       getcwd
#define pax_lseek        lseek
#define pax_read         read
#define pax_write        write
#define pax_close        close
#define pax_stringstream std::stringstream
#define pax_cout         std::cout
#define pax_cerr         std::cerr
#define pax_mkdir(a,b)   mkdir(a,b)
#define pax_stat         stat
#define pax_ifdir        S_IFDIR
#define pax_nullfile     "/dev/null"
#define pax_usleep(us)   usleep(us);
#define _L(str)          str

#define O_BINARY 0
#define O_TEXT 0
#define _O_TRUNC O_TRUNC
#endif

    class PaxMeta;
    template <typename T>
    class PaxArray;
    enum class paxTypes;
    typedef paxTypes paxTypes_e;
    class rasterFileBase;
    template <paxTypes_e E>
    class rasterFile;

    using paxMetaRegionEnum_t       = uint32_t;                 ///< Type alias for meta region enum
    using paxMetaLoc_t              = size_t;                   ///< Type alias for meta location
    using PaxMetaLocHash_t          = std::hash<std::string>;   ///< Type alias for meta hash
    using paxHeaderHashMap_t        =
        std::map<paxMetaLoc_t, std::list<PaxMetaLocHash_t>>;    ///< Type alias for hash storage in header
    using paxHeaderMetaMap_t        =
        std::map<paxMetaLoc_t, std::unordered_map<PaxMetaLocHash_t, PaxMeta>>; ///< Type alias, metadata map
    using paxMeta_t = std::variant<
        int8_t, uint8_t, int16_t, uint16_t, int32_t, uint32_t, int64_t, uint64_t, float, double,
        std::complex<float>, std::complex<double>, std::string>;    ///< Type alias for metadata values
    using paxDim_t                  = size_t;                       ///< Type alias for dimensions
    using paxBpv_t                  = size_t;                       ///< Type alias for BPV
    using paxVpe_t                  = size_t;                       ///< Type alias for VPE
    using paxBufData_t              = char;                         ///< Type alias for internal buffer type
    using paxBuf_t                  = PaxArray<paxBufData_t>;       ///< Type alias for a PAX buffer
    using paxBufPtr                 = std::shared_ptr<paxBuf_t>;    ///< Type alias for portable PAX buffer
    typedef std::shared_ptr<rasterFileBase> rasterFileBasePtr;
    template <paxTypes_e E> using rasterFilePtr = std::shared_ptr<rasterFile<E>>;

    /********************************************************************************************************
    * @enum PaxLineType Line types in PAX files
    ********************************************************************************************************/
    enum class PaxLineType {
        PLT_UNKNOWN,    ///< unknown or invalid line type
        PLT_PAXTYPE,    ///< PAX file header/type tag
        PLT_COMMENT,    ///< comment
        PLT_META,       ///< metadata
        PLT_PAXTAG,     ///< A PAX data descriptor tag
        PLT_PAXRASTER   ///< raster data
    };

/********************************************************************************************************
 * @name PAX_KEYWORDS Keywords and constants used in PAX
 *******************************************************************************************************/
///@{
    inline constexpr uint32_t PAX_MAX_METADATA_STRING_LENGTH{ 256 };
    inline constexpr uint32_t MIN_PAX_LENGTH{ 128 };
    inline constexpr char PAX_TAG[] { "PAX" };                  ///< Tag specifying the beginning of a block
    inline constexpr char BPV_TAG[]{ "BYTES_PER_VALUE" };       ///< Tag for number of bytes in one value
    inline constexpr char VPE_TAG[]{ "VALUES_PER_ELEMENT" };    ///< Tag for number of values in one element
    inline constexpr char DIM_TAG[]{ "ELEMENTS_IN_" };          ///< Pre-tag for elements in a dimension
    inline constexpr char DIM_TAG_POST[]{ "_DIMENSION" };       ///< Post-tag for elements in a dimension
    inline constexpr char DIM1_TAG[]{ "ELEMENTS_IN_SEQUENTIAL_DIMENSION" }; ///< TEMPCODE: legacy 1st-dim tag
    inline constexpr char DIM2_TAG[]{ "ELEMENTS_IN_STRIDED_DIMENSION" };    ///< TEMPCODE: legacy 2nd-dim tag
    inline constexpr char DATALEN_TAG[]{ "DATA_LENGTH" };       ///< Tag at end of header, before raster data
    inline constexpr char COMMENT_NAME_DELIM{ ';' };            ///< Delimiter used in comment names
    inline constexpr char PAX_WS[] { " \t\r" };                 ///< Legal whitespace characters
    inline constexpr char FIRST_POSTFIX[]{ "ST" };              ///< Postfix for 1st, first, etc.
    inline constexpr char SECOND_POSTFIX[]{ "ND" };             ///< Postfix for 2nd, second, etc.
    inline constexpr char THIRD_POSTFIX[]{ "RD" };              ///< Postfix for 3rd, third, etc.
    inline constexpr char FOURTH_POSTFIX[]{ "TH" };             ///< Postfix for 4th-20th, fourth-twentieth
    inline constexpr char FIRST_NUMERIC_TAG[]{ "FIRST" };               ///< Tag for the first dimension
    inline constexpr char SECOND_NUMERIC_TAG[]{ "SECOND" };             ///< Tag for the second dimension
    inline constexpr char THIRD_NUMERIC_TAG[]{ "THIRD" };               ///< Tag for the third dimension
    inline constexpr char FOURTHNUMERIC_TAG[]{ "FOURTH" };              ///< Tag for the fourth dimension
    inline constexpr char FIFTH_NUMERIC_TAG[]{ "FIFTH" };               ///< Tag for the fifth dimension
    inline constexpr char SIXTH_NUMERIC_TAG[]{ "SIXTH" };               ///< Tag for the sixth dimension
    inline constexpr char SEVENTH_NUMERIC_TAG[]{ "SEVENTH" };           ///< Tag for the seventh dimension
    inline constexpr char EIGHTH_NUMERIC_TAG[]{ "EIGHTH" };             ///< Tag for the eighth dimension
    inline constexpr char NINTH_NUMERIC_TAG[]{ "NINTH" };               ///< Tag for the ninth dimension
    inline constexpr char TENTH_NUMERIC_TAG[]{ "TENTH" };               ///< Tag for the 10th dimension
    inline constexpr char ELEVENTH_NUMERIC_TAG[]{ "ELEVENTH" };         ///< Tag for the 11th dimension
    inline constexpr char TWELFTH_NUMERIC_TAG[]{ "TWELFTH" };           ///< Tag for the 12th dimension
    inline constexpr char THIRTEENTH_NUMERIC_TAG[]{ "THIRTEENTH" };     ///< Tag for the 13th dimension
    inline constexpr char FOURTEENTH_NUMERIC_TAG[]{ "FOURTEENTH" };     ///< Tag for the 14th dimension
    inline constexpr char FIFTEENTH_NUMERIC_TAG[]{ "FIFTEENTH" };       ///< Tag for the 15th dimension
    inline constexpr char SIXTEENTH_NUMERIC_TAG[]{ "SIXTEENTH" };       ///< Tag for the 16th dimension
    inline constexpr char SEVENTEENTH_NUMERIC_TAG[]{ "SEVENTEENTH" };   ///< Tag for the 17th dimension
    inline constexpr char EIGHTEENTH_NUMERIC_TAG[]{ "EIGHTEENTH" };     ///< Tag for the 18th dimension
    inline constexpr char NINETEENTH_NUMERIC_TAG[]{ "NINETEENTH" };     ///< Tag for the 19th dimension
    inline constexpr char TWENTIETH_NUMERIC_TAG[]{ "TWENTIETH" };       ///< Tag for the 20th dimension
///@}

/************************************************************************************************************
 * @enum PAX_RETVAL Return values used in PAX functions.
 ***********************************************************************************************************/
    enum PAX_RETVAL : int32_t {
        PAX_INVALID     = -14,                  ///< Invalid parameters were given
        PAX_FAIL        = -13,                  ///< An unrecoverable failure occurred
        PAX_ERROR       = -12,                  ///< An error occurred
        PAX_WARN        = -1,                   ///< Warning, proceed with caution
        PAX_OK          = 0,                    ///< Success
        PAX_FALSE       = 0,                    ///< Boolean false
        PAX_TRUE        = 1,                    ///< Boolean true
    };

/************************************************************************************************************
 * @enum SKIP_OPTIONS Options for skipping in header.
 ***********************************************************************************************************/
    enum SKIP_OPTIONS : int32_t {
        _SKIP_DELIMITER = 0x01,     ///< delimiters will be skipped
        _SKIP_LINEFEED  = 0x02      ///< linefeeds will be skipped
    };

/************************************************************************************************************
 * @enum SKIP_OPTIONS Options for skipping in header.
 ***********************************************************************************************************/
/********************************************************************************************************
 * @typedef skipFlags skipFlags_e
 * Alias for skipFlags enumeration
 *******************************************************************************************************/
    typedef enum skipFlags {
        SKIP_NOTHING = 0,                                               ///< don't skip anything
        SKIP_DELIMITER = _SKIP_DELIMITER,                               ///< skip delimiters only
        SKIP_LINEFEED = _SKIP_LINEFEED,                                 ///< skip linefeeds only
        SKIP_DELIMIER_AND_LINEFEED = _SKIP_DELIMITER | _SKIP_LINEFEED,  ///< skip delimiters and linefeeds
        SKIP_ALL = _SKIP_DELIMITER | _SKIP_LINEFEED                     ///< skip everything
    } skipFlags_e;

/********************************************************************************************************
 * @enum paxMetaDataTypes Strongly-typed enum for identifying type of metadata.
 * Note that comments are a special type of unnamed metadata.
 *******************************************************************************************************/
 /********************************************************************************************************
 * @typedef paxMetaDataTypes paxMetaDataTypes_e
 * Alias for paxMetaDataTypes enumeration
 *******************************************************************************************************/
    typedef enum class paxMetaDataTypes {
        paxComment = -2,        ///< comment (no metadata)
        paxInvalid = -1,        ///< unknown or invalid type
        paxMetaStart = 0,       ///< first valid type index
        paxString = 0,          ///< string
        paxNumericStart = 1,    ///< index of first numeric type
        paxFloat = 1,           ///< single-precision float
        paxDouble = 2,          ///< double-precision float
        paxInt64 = 3,           ///< 64-bit signed int
        paxUint64 = 4,          ///< 64-bit unsigned int
        paxInt32 = 5,           ///< 32-bit signed int
        paxUint32 = 6,          ///< 32-bit unsigned int
        paxInt16 = 7,           ///< 16-bit signed int
        paxUint16 = 8,          ///< 16-bit unsigned int
        paxInt8 = 9,            ///< 8-bit signed int
        paxUint8 = 10,          ///< 8-bit unsigned int
        paxNumericEnd = 10,     ///< end of numeric types
        paxMetaEnd = 10,        ///< last valid type index
        //paxComplex  = 3
    } paxMetaDataTypes_e;

/********************************************************************************************************
 * @enum HEADERLINETYPE Strongly-typed enum defining header line types.
 *******************************************************************************************************/
/********************************************************************************************************
 * @typedef HEADERLINETYPE hlType_t
 * Alias for HEADERLINETYPE
 *******************************************************************************************************/
/********************************************************************************************************
 * @typedef HEADERLINETYPE headerLineType_t
 * Alias for HEADERLINETYPE
 *******************************************************************************************************/
    typedef enum class HEADERLINETYPE : int32_t {
        NOT_CHECKED = -2,
        UNKNOWN = -1,
        COMMENT = 0,
        METADATA = 1,
        PAX = 16,
        BPV,
        VPE,
        DIM,
        DATALEN
    } hlType_t, headerLineType_t;

/********************************************************************************************************
 * @name METATYPES Character strings defining names for types of metadata
 * TODO: remove need to be kept in sync with metaTypeTags in BufMan::getMeta() and struct meta below
 *******************************************************************************************************/
///@{
    inline constexpr uint32_t METATYPES             { 11 };
    inline constexpr uint32_t METATYPE_MAX_TAG_LEN  { 8 };
    inline constexpr char METATYPE_COMMENT_TAG[]    { "" };
    inline constexpr char METATYPE_INVALID_TAG[]    { "invalid" };
    inline constexpr char METATYPE_FLOAT_TAG[]      { "float" };
    inline constexpr char METATYPE_STRING_TAG[]     { "string" };
    inline constexpr char METATYPE_DOUBLE_TAG[]     { "double" };
    inline constexpr char METATYPE_INT64_TAG[]      { "int64" };
    inline constexpr char METATYPE_UINT64_TAG[]     { "uint64" };
    inline constexpr char METATYPE_INT32_TAG[]      { "int32" };
    inline constexpr char METATYPE_UINT32_TAG[]     { "uint32" };
    inline constexpr char METATYPE_INT16_TAG[]      { "int16" };
    inline constexpr char METATYPE_UINT16_TAG[]     { "uint16" };
    inline constexpr char METATYPE_INT8_TAG[]       { "int8" };
    inline constexpr char METATYPE_UINT8_TAG[]      { "uint8" };
///@}

/********************************************************************************************************
 * @name METAARRAYTAGS Character strings defining names for metadata array indices
 * TODO: implement unlimited dimensions
 *******************************************************************************************************/
///@{
    inline constexpr uint32_t METAARRAYINDEXES              { 4 };
    inline constexpr uint32_t  METAARRAYINDEX_MAX_TAG_LEN   { 6 };
    inline constexpr char METAARRAYINDEX_FIRST_TAG[]        { "first" };
    inline constexpr char METAARRAYINDEX_SECOND_TAG[]       { "second" };
    inline constexpr char METAARRAYINDEX_THIRD_TAG[]        { "third" };
    inline constexpr char METAARRAYINDEX_FOURTH_TAG[]       { "fourth" };
///@}

/************************************************************************************************************
 * @name PAX_LOGGING Helper macros used in logging, etc.
 ***********************************************************************************************************/
///@{

/*****************************************************************************************************************
 * @def PAX_LOG_TAG The tag for lines written by the PAX logger
 ****************************************************************************************************************/
#define PAX_LOG_TAG "*** PAX : "
/*****************************************************************************************************************
 * @def PAX_PAD0 The tag for PAX logger lines at verbosity 0
 ****************************************************************************************************************/
#define PAX_PAD0
/*****************************************************************************************************************
 * @def PAX_PAD1 The tag for PAX logger lines at verbosity 1
 ****************************************************************************************************************/
#define PAX_PAD1
/*****************************************************************************************************************
 * @def PAX_PAD2 The tag for PAX logger lines at verbosity 2
 ****************************************************************************************************************/
#define PAX_PAD2 << "..."
/************************************************************************************************************
 * @def PAX_PAD3 The tag for PAX logger lines at verbosity 3
 ***********************************************************************************************************/
#define PAX_PAD3 << "......"
/************************************************************************************************************
 * @def PAX_PAD4 The tag for PAX logger lines at verbosity 4
 ***********************************************************************************************************/
#define PAX_PAD4 << "........."
#define PAX_LOG(level, chain) PAX_LOGX(level, chain)
/************************************************************************************************************
 * @def PAX_LOG The main logging macro.
 * to expand macros in 'level' we must pass PAX_LOG to another macro that hasn't been defined yet
 ***********************************************************************************************************/
/************************************************************************************************************
 * @def PAX_LOG_ERROR Logging macro for errors.
 ***********************************************************************************************************/
#define PAX_LOG_ERROR(level, chain)                                                                 \
{   PaxStatic::setStatus(PAX_FAIL); PAX_LOG(level, << "ERROR: " chain); }
/************************************************************************************************************
 * @def PAX_LOG_ERRNO Logging macro for errors that adds errno.
 ***********************************************************************************************************/
#define PAX_LOG_ERRNO(level, chain)                                                                 \
{   PaxStatic::setStatus(PAX_FAIL); PAX_LOG(level, << "ERROR: " << errno chain); }
/************************************************************************************************************
 * @def PAX_LOG_WARN Logging macro for warnings.
 ***********************************************************************************************************/
#define PAX_LOG_WARN(level, chain)                                                                  \
{   PaxStatic::setStatus(PAX_WARN); PAX_LOG(level, << " WARN: " chain); }
/************************************************************************************************************
 * @def PAX_LOGX The macro that does the actual logging.
 ***********************************************************************************************************/
#define PAX_LOGX(level, chain)                                                                      \
if(PaxStatic::getVerbosity() >= level) {                                                            \
    std::ostringstream oss; oss << PAX_LOG_TAG << "[" << std::setw(2) << level << "] " <<           \
        std::left << std::setw(64) << __FUNCTION__ << " : " PAX_PAD ## level << std::right chain;   \
    std::cout << oss.str() << std::endl << std::flush; }
///@}

#define PAX_MIN(x, y) (((x) < (y)) ? (x) : (y))
#define PAX_MAX(x, y) (((x) < (y)) ? (y) : (x))

/************************************************************************************************************
 * @name SWAPPER Some cute little classes for temporary string manipulation. The primary
 * feature is allowing printing of a C-style substring in a char array without requiring
 * a copy. Just instantiate a TempNull in a local scope, passing it the address
 * at which to temporarily terminate the string. The TempNull restores the
 * original character when restore() is called or it goes out of scope.
 ***********************************************************************************************************/
///@{
    /********************************************************************************************************
    * @class Swapper
    * Swaps a character temporarily.
    * @tparam C The temporary character
    ********************************************************************************************************/
    template <uint8_t C>
    class Swapper {

    public:
        Swapper(char * null) : _null(null) { swap(); PAX_LOG(4, << "---Swapper ctor stored char " << _old); }
        virtual ~Swapper() { deswap(); PAX_LOG(4, << "---Swapper dtor restored char " << _old); }
        void restore() { deswap(); PAX_LOG(4, << "---Swapper restored char " << _old); }

    private:
        char swap() { if (_null && *_null != C) { _old = (uint8_t)*_null; *_null = C; return _old; } return _old = C; }
        char deswap() { if (_null) { *_null = _old; _null = NULL; } return _old; }

    protected:
        uint8_t   _old;
        char * _null;
    };

    /********************************************************************************************************
    * @class TempNull
    * Swapper using a C-string terminator (chr 0)
    ********************************************************************************************************/
    class TempNull : public Swapper<'\0'> {
    public:
        TempNull(char * null) : Swapper(null) {}
        ~TempNull() {}
    };
///@}


/************************************************************************************************************
 * @class PaxStatic
 * Global static data. Use this class with global helper functions for non-threadsafe data.
 ***********************************************************************************************************/
    class PaxStatic {
    private:

   /********************************************************************************************************
 * Executes a verbosity operation
 * Valid operations are:
 *  - 0: set verbosity
 *  - 1: get verbosity
 *  - 2: compare verbosity
 * @param[in]       op      The operation code
 * @param[in,out]   value   integer verbosity value
 * @return          current verbosity or result of comparison
 *******************************************************************************************************/
        static int verbosityOps(const int op, int &value) {

            static int _verb = 0;

            switch (op) {
            case 0: ///< set verbosity
                _verb = value;
                break;
            case 1: // get verbosity
                break;
            case 2: // compare given value to current
                value = (int)(_verb >= value);
                break;
            }

            return _verb;

        }

    public:
/********************************************************************************************************
 * sets the verbosity to the given value
 * @param[in]       verb desired value
 * @return          current verbosity
 *******************************************************************************************************/
        static int setVerbosity(const int verb) {
            int _verb = verb;
            return verbosityOps(0, _verb);
        }

/********************************************************************************************************
 * gets the current verbosity value
 * @return          current verbosity
 *******************************************************************************************************/
        static int getVerbosity() {
            int _verb = 0;
            return verbosityOps(1, _verb);
        }

/********************************************************************************************************
 * compares the current verbosity with the given value
 * @param[in]       verb desired value
 * @return          current verbosity
 *******************************************************************************************************/
        static int checkVerbosity(const int verb) {
            int _verb = verb;
            int _res = verbosityOps(2, _verb);
            return _res;
        }

    private:
 /********************************************************************************************************
 * Executes a status operation
 * Valid operations are:
 *  - 0: set status
 *  - 1: get status
 *  - 2: check status
 *  - 3: threshold status
 * @param[in]       op      The operation code
 * @param[in,out]   value   integer status value
 * @return          current status
 *******************************************************************************************************/
        static int statusOps(const int op, int& value) {

            static int _status = PAX_OK;
            static bool _test = true;

            switch (op) {
            case 0: ///< case 0: set status
                _status = value;
                break;
            case 1: ///< case 1: get status
                break;
            case 2: ///< case 2: compare equal
                _test = (_status == value);
                return (int)_test;
            case 3: ///< case 3: compare greater than
                _test = (_status >= value);
                return (int)_test;
            }

            return _status;

        }

    public:

/********************************************************************************************************
 * sets the status to the given value
 * @param[in]       status Current status value
 * @return          current status
*******************************************************************************************************/
        static int setStatus(const int status) {
            int _status = status;
            return statusOps(0, _status);
        }

        // returns the current status
/********************************************************************************************************
 * gets the current status
 * @return          current status
 *******************************************************************************************************/
        static int getStatus() {
            int _status = 0;
            return statusOps(1, _status);
        }

/********************************************************************************************************
 * checks to see if status is exactly the given value
 * @param[in]       status value to compare current status to
 * @return          1 if equal, 0 if not
 *******************************************************************************************************/
        static int checkStatus(const int status) {
            int _status = status;
            int equal = statusOps(2, _status);
            return equal;
        }

/********************************************************************************************************
 * checks to see if status is equal to or better than the given value
 * @param[in]       status status value for comparison
 * @return          1 if equal, 0 if not
 *******************************************************************************************************/
        static int thresholdStatus(const int status) {
            int _status = status;
            int threshold = statusOps(3, _status);
            return threshold;
        }

/********************************************************************************************************
 * Checks for an error and if none returns us to the OK state
 * @return          1 if error status, 0 otherwise
 *******************************************************************************************************/
        static int paxNoError() {
            int noError = thresholdStatus(PAX_ERROR + 1);

            if (noError) {
                setStatus(PAX_OK);
            }

            return noError;
        }

/********************************************************************************************************
 * Checks for any error, warning, etc.
 * @param[in]       ignoreWarnings Warnings are not considered
 * @return          1 if not a error/warning state, 0 otherwise
 *******************************************************************************************************/
        static int paxOk(const bool ignoreWarnings = false) {
            int ok = 1;
            if (ignoreWarnings) {
                ok = paxNoError();
            } else {
                ok = checkStatus(PAX_OK);
            }
            if (ok != 1) {
                PAX_LOG(3, << "paxOk failed due to status = " << getStatus() << (ignoreWarnings ? " (ignoring warnings)" : ""));
            }

            return ok;
        }

    private:
/********************************************************************************************************
 * Executes a version operation
 * Valid operations are:
 *  - 0: get current version
 *  - 1: get default version
 * @param[in]       op      operation code
 * @param[in,out]   version float version
 * @return          version or -1 on error
 *******************************************************************************************************/
        static float versionOps(const int op, float& version) {

            static const float _currentVersion = (float)PAX_VERSION;
            static const float _defaultVersion = 1.00f;

            switch (op) {
            case 0: ///< case0 get Current Version
                version = _currentVersion;
                break;
            case 1: ///< case1 get default version
                version = _defaultVersion;
                break;
            default:
                version = -1.0f;  ///< error
                break;
            }

            return version;
        }

    public:
/********************************************************************************************************
 * Gets the current version
 * @return The current active version
 *******************************************************************************************************/
        static float currentVersion() {
            float version;
            return versionOps(0, version);
        }

/********************************************************************************************************
 * Gets the default version
 * @return The default version
 *******************************************************************************************************/
        static float defaultVersion() {
            float version;
            return versionOps(1, version);
        }


/********************************************************************************************************
 * Returns the standard tag for the given metadata type
 * @param[in]       type Enumerated metadata type
 * @return          standard C-string metadata type data
 *******************************************************************************************************/
        static const char * getMetaTypeTag(const paxMetaDataTypes_e type) {

          // the order here must match paxMetaDataTypes_e!
            static const char* metaTypeTags[METATYPES] = {
              METATYPE_STRING_TAG,
              METATYPE_FLOAT_TAG,
              METATYPE_DOUBLE_TAG,
              METATYPE_INT64_TAG,
              METATYPE_UINT64_TAG,
              METATYPE_INT32_TAG,
              METATYPE_UINT32_TAG,
              METATYPE_INT16_TAG,
              METATYPE_UINT16_TAG,
              METATYPE_INT8_TAG,
              METATYPE_UINT8_TAG
            };

            if (type >= paxMetaDataTypes::paxMetaStart && type <= paxMetaDataTypes::paxMetaEnd) {
                return metaTypeTags[(int)type];
            }

            switch (type) {
            case paxMetaDataTypes::paxComment:
                return METATYPE_COMMENT_TAG;
            }

            // shouldn't normally get here
            PaxStatic::setStatus(PAX_FAIL);
            return METATYPE_INVALID_TAG;

        } // static const char * getMetaTypeTag (paxMetaDataTypes_e type) 


/********************************************************************************************************
 * Returns the standard tag for the given metadata array index
 * @param[in]       dimension index
 * @return          standard C-string metadata dimension index tag
 *******************************************************************************************************/
        static const char * getMetaArrayIndexTag(const uint32_t index) {

            static const char* metaArrayIndexTags[METAARRAYINDEXES] = {
              METAARRAYINDEX_FIRST_TAG,
              METAARRAYINDEX_SECOND_TAG,
              METAARRAYINDEX_THIRD_TAG,
              METAARRAYINDEX_FOURTH_TAG
            };

            if (index >= METAARRAYINDEXES) {
                PaxStatic::setStatus(PAX_FAIL);
                return "";
            }

            return metaArrayIndexTags[index];

        } // static const char * getMetaArrayIndexTag (int32_t index)

    }; // class PaxStatic 


/********************************************************************************************************
 * @class PaxArray
 * Wrapper for std::vector that can also accept a user buffer.
 * @tparam T type of the internal storage
 *******************************************************************************************************/
    template<typename T>
    class PaxArray {
    //private: // TODO: delete
    //    PaxArray() {}  // size is required

    public:
/********************************************************************************************************
 * Ctor using std::vector of given size.
 * @param len Desired buffer size.
 *******************************************************************************************************/
        PaxArray(const uint64_t len) : _buf(NULL), _len(len) { _vec.resize(len); }
/********************************************************************************************************
 * Ctor using user buffer of given (minimum) size.
 * @param len Specified buffer size.
 *******************************************************************************************************/
        PaxArray(T* buf, const uint64_t len) : _len(len), _buf(buf) { }
/********************************************************************************************************
 * Dtor
 *******************************************************************************************************/
        ~PaxArray() {}

    public:
/********************************************************************************************************
 * Get the buffer size.
 * @return Current buffer size.
 *******************************************************************************************************/
        uint64_t size() { return _len; }
/********************************************************************************************************
 * Direct buffer access.
 * @return Pointer to type of internal buffer.
 *******************************************************************************************************/
        T* data() { return useVec() ? _vec.data() : _buf; }

/********************************************************************************************************
 * Resizes the vector. Does not modify user buffer. Shrink-only if using user buffer.
 * @param newSize 
 *******************************************************************************************************/
        uint64_t resize(const uint64_t newSize) {
            if (useVec()) {
                _vec.resize(newSize); _len = newSize; return _len;
            } else {
                if (newSize < _len) { _len = newSize; } return _len;
            }
        }

/********************************************************************************************************
 * Appends a vector. Does not modify user buffer. Does nothing if using user buffer.
 *******************************************************************************************************/
        size_t appendVector(PaxArray<T> &vec) {
            if (useVec()) {
                _vec.resize(_len + vec._len);
                memcpy(_vec.data() + _len, vec.data(), vec._len);
                _len += vec._len;
                return _len;
            } else {
                return _len;
            }
        }

    private:
 /********************************************************************************************************
 * Is internal std::vector being used.
 * @return true if the std::vector is in use
 *******************************************************************************************************/
        bool useVec() { return _buf == NULL; }
        std::vector<T>  _vec;                   ///< The std::vector object
        T*              _buf;                   ///< The user buffer
        uint64_t        _len;                   ///< Current length of the buffer

    }; // class PaxArray 



/********************************************************************************************************
 * @enum metaLoc
 * standard metadata locations
 *******************************************************************************************************/
    typedef enum metaLoc {
        LOC_UNKNOWN = -1,   ///< Unknown or invalid location
        LOC_BEGIN = 0,      ///< Beginning of file
        LOC_AFTER_TAG = 0,  ///< immediately after PAX tag
        LOC_AFTER_BPV = 1,  ///< after bytes per value tag
        LOC_AFTER_VPE = 2,  ///< after values per element tag
        LOC_AFTER_SEQ = 3,  ///< after sequential dimension tag
        LOC_AFTER_STR1 = 4, ///< after strided dimension tag
        LOC_END = 4,        ///< at end of file (before DATA_LENGTH tag)
        LOC_DEFAULT = 4,    ///< default location
        LOC_COUNT = 5       ///< number of defined locations
    } metaLoc_e;

    //////////////////////////////////////////////////////////////////////////
    //
    // helper function to get size of meta type
    //
    static size_t getMetaDataTypeSize(paxMetaDataTypes_e type) {

        size_t size = 0;

        switch (type) {

        case paxMetaDataTypes::paxDouble:
        case paxMetaDataTypes::paxInt64:
        case paxMetaDataTypes::paxUint64:
            size = 8; break;

        case paxMetaDataTypes::paxFloat:
        case paxMetaDataTypes::paxInt32:
        case paxMetaDataTypes::paxUint32:
            size = 4; break;

        case paxMetaDataTypes::paxInt16:
        case paxMetaDataTypes::paxUint16:
            size = 2; break;

        case paxMetaDataTypes::paxInt8:
        case paxMetaDataTypes::paxUint8:
            size = 1; break;

        default:
            break;

        } // switch (type)

        // this shouldn't be called with non-numeric types
        if (0 == size) {
            PAX_LOG_ERROR(1, << "called getMetaDataTypeSize with invalid type = " << (int)type);
        }

        return size;

    } // size_t getMetaDataTypeSize (paxMetaDataTypes_e type) 


/********************************************************************************************************
 * @struct meta
 * structure for storing metadata
 * note: must be kept in sync with metaTypeTags in BufMan::getMeta() and the meta defines above
 *******************************************************************************************************/
/********************************************************************************************************
 * @typedef meta meta_t
 * Alias for meta structure
 *******************************************************************************************************/
    typedef struct meta {

        metaLoc_e      loc;        ///< index of location within file
        size_t              index;      ///< index within location
        paxMetaDataTypes_e  type;       ///< metadata type
        uint8_t             num_dims;   ///< number of dimensions. Use num_dims = 0 for scalar data.

/********************************************************************************************************
 * @union
 * for storing scalar/string/etc. data
 *******************************************************************************************************/
        union {
            float     f;
            double    d;
            uint64_t  u64;
            int64_t   n64;
            uint32_t  u32;
            int32_t   n32;
            uint16_t  u16;
            int16_t   n16;
            uint8_t   u8;
            int8_t    n8;
            char      s[PAX_MAX_METADATA_STRING_LENGTH];
        };

        bool                stripped;   ///< leading space was stripped off
        bool                ownBuf;     ///< do I own the buffer

        // non-POD types must go here at the end
        std::string         name;       ///< unique name of this meta
        uint32_t *          dims;       ///< dimensions array

/********************************************************************************************************
 * @union
 * for storing array data
 *******************************************************************************************************/
        union {
            char *            buf;
            float *           fb;
            double *          db;
            uint64_t *        u64b;
            int64_t *         n64b;
            uint32_t *        u32b;
            int32_t *         n32b;
            uint16_t *        u16b;
            int16_t *         n16b;
            uint8_t *         u8b;
            int8_t *          n8b;
        };


/********************************************************************************************************
 * @name Construction/destruction
 *******************************************************************************************************/
///@{
/********************************************************************************************************
 * Default Ctor
 *******************************************************************************************************/
        meta() :
            loc(LOC_UNKNOWN),
            type{ paxMetaDataTypes_e::paxInvalid },
            name(""),
            dims{ NULL },
            buf{ s }
        { }

/********************************************************************************************************
 * Ctor with type and initializers
 * @param[in]       _type type of the metadata
 * @param[in]       list initializer_list used to construct std::vector
 *******************************************************************************************************/
        meta(const paxMetaDataTypes_e _type, std::initializer_list<uint32_t> list) :
            loc{ LOC_UNKNOWN },
            type{ paxMetaDataTypes_e::paxInvalid },
            name{ "" }, 
            dims{ NULL },
            buf{ s }
        {

            initArray(_type, list);

        }

/********************************************************************************************************
 * Ctor with type and dimensions
 * @param[in]       _type type of the metadata
 * @param[in]       _dims dimensions of metadata array
 *******************************************************************************************************/
        meta(const paxMetaDataTypes_e _type, std::vector<uint32_t> &_dims) :
            loc{ LOC_UNKNOWN },
            type{ paxMetaDataTypes_e::paxInvalid },
            name{ "" },
            dims{ NULL },
            buf{ s }
        {

            initArray(_type, _dims);

        } //         meta(const paxMetaDataTypes_e _type, std::vector<uint32_t> &_dims) :


/********************************************************************************************************
 * Ctor with type, initializers, and data
 * @param[in]       _type type of the metadata
 * @param[in]       list initializer_list used to construct std::vector
 * @param[in]       data Buffer containing the initial data (must be correct size!)
 *******************************************************************************************************/
        meta(paxMetaDataTypes_e _type, std::initializer_list<uint32_t> list, const void* data) :
            loc(LOC_UNKNOWN),
            type(paxMetaDataTypes_e::paxInvalid),
            name(""),
            dims{ NULL },
            buf{ s }
        {

            initArray(_type, list, data);

        } //         meta(paxMetaDataTypes_e _type, std::initializer_list<uint32_t> list, const void* data) :


/********************************************************************************************************
 * Ctor with type, initializers, and data
 * @param[in]       _type type of the metadata
 * @param[in]       _dims dimensions of metadata array
 * @param[in]       data Buffer containing the initial data (must be correct size!)
 *******************************************************************************************************/
        meta(paxMetaDataTypes_e _type, std::vector<uint32_t> &_dims, const void* data) :
            loc(LOC_UNKNOWN),
            type(paxMetaDataTypes_e::paxInvalid),
            name{ "" },
            dims{ NULL },
            buf{ s }
        {

            initArray(_type, _dims, data);

        } //    meta(paxMetaDataTypes_e _type, std::vector<uint32_t> &_dims, const void* data)


/********************************************************************************************************
 * Deep-copy ctor
 * @param[in]       _meta The source meta
 *******************************************************************************************************/
        meta(const meta& _meta) {

            clone(_meta);

        } //         meta(const meta& _meta) {


/********************************************************************************************************
 * Dtor
 *******************************************************************************************************/
        ~meta() {
            cleanup();
        } // dtor
///@}

/********************************************************************************************************
 * Assignment operator
 * @param[in]       _meta The source meta
 * @return          Self-reference
 *******************************************************************************************************/
        meta& operator = (const meta& _meta) {

            if (&_meta == this) {
                return *this;
            }

            clone(_meta);

            return *this;

        } //         meta& operator = (const meta& _meta) {


/********************************************************************************************************
 * Performs a deep-copy of the given meta
 * @param[in]       _meta The source meta
 *******************************************************************************************************/
        void clone(const meta& _meta) {

            int status = PaxStatic::getStatus();
            if (!PaxStatic::paxNoError()) {
                PAX_LOG_ERROR(1, << "attemping to clone meta from meta '" << _meta.name << "' with existing error = " << status);
            }

            copyPod(_meta);
            name = _meta.name;

            // always set allocated members so cleanup() doesn't crash
            buf = s;
            dims = NULL;

            if (num_dims > 0) {
                std::vector<uint32_t> _dims(num_dims);
                memcpy(_dims.data(), _meta.dims, num_dims * sizeof(uint32_t));
                initArray(type, _dims);
                memcpy(buf, _meta.buf, bytes());
            }

            PAX_LOG(3, << "cloned meta '" << name << "', num_dims = " << num_dims << ", bytes = " << bytes() << ". Status = " << PaxStatic::getStatus());

        } // void clone (const meta& _meta) {


/********************************************************************************************************
 * Frees any allocated memory
 *******************************************************************************************************/
        void cleanup() {

            if (NULL != buf && s != buf) {
                free(buf);
                buf = NULL;
            }

            if (NULL != dims) {
                free(dims);
                dims = NULL;
            }
        } //         void cleanup() {


/********************************************************************************************************
 * Copies pod from the given meta
 * @param[in]       m       The source meta
 *******************************************************************************************************/
        void copyPod(const meta& m) {

            size_t len = sizeof(meta) - sizeof(name) - sizeof(buf) - sizeof(dims); // don't copy non-POD members!
            memcpy(this, &m, len);

        } //         void copyPod(const meta& m) {


/********************************************************************************************************
 * Initializes the object for array storage of the given dims and type.
 * @param[in]       _type   Type of the metadata
 * @param[in]       _dims   Dimensions of the metadata array
 * @param[in]       data    Input data
 * @return                  The total number of metadata elements
 *******************************************************************************************************/
        size_t initArray(paxMetaDataTypes_e _type, std::vector<uint32_t> &_dims, const void* data = NULL) {

            int status = PaxStatic::getStatus();
            PAX_LOG(3, << "allocating meta array, initial status = " << status);

            size_t _count = 1;
            for (auto dim : _dims) { _count *= dim; }

            // cases that are not an array
            if (_type < paxMetaDataTypes::paxNumericStart || _type > paxMetaDataTypes::paxNumericEnd || 1 >= _count) {

                num_dims = 0;
                buf = NULL;
                dims = NULL;
                ownBuf = false;

                PAX_LOG_WARN(3, << "Tried to initialize a meta array with invalid meta type = " << (int)_type << " and/or scalar data. count = " << _count);

                return 1;

            }

            status = PaxStatic::getStatus();
            PAX_LOG(4, << "allocating meta array, status after checking array = " << status);

            size_t _size = getMetaDataTypeSize(_type) * _count;
            if (0 == _size) {
                type = paxMetaDataTypes::paxInvalid;
                PAX_LOG_ERROR(3, << "Unknown size = 0 for meta type " << (int)_type);
                return PAX_FAIL;
            }

            status = PaxStatic::getStatus();
            PAX_LOG(4, << "allocating meta array, status after checking array = " << status);

            cleanup();

            status = PaxStatic::getStatus();
            PAX_LOG(4, << "after meta array cleanup, status = " << status);

            // store the dimensions (always allocated)
            num_dims = (uint8_t)_dims.size();  // TODO: justify dimension count
            dims = (uint32_t*)malloc(num_dims * sizeof(uint32_t));
            for (int i = 0; i < num_dims; ++i) { dims[i] = _dims[i]; }

            // initialize the data buffer
            ownBuf = _size > PAX_MAX_METADATA_STRING_LENGTH;
            if (ownBuf) {
                buf = (char*)malloc(_size);
            } else {
                buf = s;
            }

            type = _type;

            if (data != NULL) {
                memcpy(buf, data, _size);
            }

            return _count;

        } // size_t initArray(paxMetaDataTypes_e _type, ::std::vector<uint32_t> _dims) 

/********************************************************************************************************
 * Initializes the object for array storage of the given dims and type.
 * @param[in]       _type   Type of the metadata
 * @param[in]       list    initializer_list, gets casted to std::vector
 * @param[in]       data    Input data
 * @return                  The total number of metadata elements
 *******************************************************************************************************/
        size_t initArray(const paxMetaDataTypes_e _type, std::initializer_list<uint32_t> list,

            const void* data = NULL) {

            std::vector<uint32_t> _dims(list);
            size_t count = initArray(_type, _dims, data);

            return count;

        } // size_t initArray(paxMetaDataTypes_e _type, ::std::vector<uint32_t> _dims, void* data) 


/********************************************************************************************************
 * Sets all values to zero
 * @return                  number of allocated bytes
 *******************************************************************************************************/
        size_t zero() {

            size_t _bytes = bytes();

            memset(buf, 0, _bytes);

            return _bytes;

        } //         size_t zero() {


/********************************************************************************************************
 * Returns the number of elements in the specified dimension or all dimensions
 * @param[in]       dim     optional dimension index
 * @return                  number of elements
 *******************************************************************************************************/
        size_t count(const int8_t dim = -1) {

            size_t _count = 1;
            if (0 == num_dims) return 1;

            if (-1 == dim) {
                for (int i = 0; i < num_dims; ++i) {
                    _count *= dims[i];
                }
            } else if (dim < num_dims) {
                return dims[dim];
            } else {
                PaxStatic::setStatus(PAX_FAIL);
                return 0;
            }

            return _count;

        } // size_t count(int8_t dim = -1) 


/********************************************************************************************************
 * Returns the size of the data in the specified dimension or all dimensions in bytes
 * @param[in]       dim     optional dimension index
 * @return                  number of elements
 *******************************************************************************************************/
        size_t bytes(const int8_t dim = -1) {

            size_t _count = count(dim);
            if (0 == _count) {
                return 0;
            }

            if (paxMetaDataTypes::paxInvalid == type) {
                return 0;
            }

            return _count * getMetaDataTypeSize(type);

        } //         size_t bytes(const int8_t dim = -1) {


/********************************************************************************************************
 * Returns the number of defined dimensions, including trivial ones
 * @return                  number of dimensions
 *******************************************************************************************************/
        size_t dimCount() {

            return num_dims;

        } // size_t dimCount() 


/********************************************************************************************************
 * Calculates index into flattened array of the specified element 
 * @param[in]       indices Indices of desired element
 * @return                  flattened index
 *******************************************************************************************************/
        size_t getMetaArrayIndex(const std::vector<uint32_t> &indices) {

            if (indices.size() > num_dims) {
                PaxStatic::setStatus(PAX_FAIL);
                return 0; // std::numeric_limits<size_t>::max();
            }

            size_t index = 0;   ///< flattened index
            size_t mul = 1;     ///< multiplier for this dimension's data

            for (size_t i = 0; i < indices.size(); ++i) {

                if (indices[i] >= dims[i]) {
                    PaxStatic::setStatus(PAX_FAIL); // index out of range
                    return 0;
                }

                index += indices[i] * mul;
                mul *= dims[i];

            }

            return index;

        } //         size_t getMetaArrayIndex(const std::vector<uint32_t> &indices) {


/********************************************************************************************************
 * Calculates index into flattened array of the specified element (shorthand version)
 * @param[in]       indices Indices of desired element
 * @return                  flattened index
 *******************************************************************************************************/
        size_t I(std::vector<uint32_t> &indices) {

            return getMetaArrayIndex(indices);

        } //         size_t I(std::vector<uint32_t> &indices) {


/********************************************************************************************************
 * Returns the pointer to the internal array buffer
 * @return                  pointer to internal buffer
 *******************************************************************************************************/
        void * bufPtr() {

            if (!isArray()) {
                PaxStatic::setStatus(PAX_FAIL);
                return NULL;
            }
            // its less work to handle the non-numeric types since all numeric type buffers are in a union
            switch (type) {

            case paxMetaDataTypes::paxComment:
            case paxMetaDataTypes::paxInvalid:
            case paxMetaDataTypes::paxString:
                return NULL;

            default:
                return (void*)u64b;

            }

            return (void*)buf;

        } //         void * bufPtr() {


/********************************************************************************************************
 * Generate a standard name for a comment to be stored at the given location
 * @param[in]       loc     location index
 * @param[in]       index   index within location
 * @return                  string name
 *******************************************************************************************************/
        static std::string&& getCommentName(size_t loc, size_t index) {

            std::stringstream ss;

            // comments get a name starting with a character that is invalid at beginning of metadata name
            ss << COMMENT_NAME_DELIM << loc << COMMENT_NAME_DELIM << index;
            std::string name = ss.str();

            return std::move(name);

        } //         static std::string&& getCommentName(size_t loc, size_t index) {


/********************************************************************************************************
 * Generate and store the standard name for a comment to be stored at the current internal location.
 * @return                  string name
 *******************************************************************************************************/
        std::string commentName() {

            name = getCommentName(loc, index);

            return name;
        } //        std::string commentName() {


/********************************************************************************************************
 * Return a string representation of the metadata
 * @return                  string flattened string representation of the metadata
 *******************************************************************************************************/
        std::string value() {

            std::stringstream ss;

            switch (type)
            {
            case paxMetaDataTypes_e::paxDouble:
                ss << d;
                break;
            case paxMetaDataTypes_e::paxString:
            case paxMetaDataTypes_e::paxComment:
                ss << s;
                break;
            default:
                break;
            }

            std::string val = ss.str();

            return val;

        } //         std::string value() {


/********************************************************************************************************
 * less-than operator, needed for std::sort
 * @param[in]       rt  meta being compared to
 * @return              true if less than given meta, false otherwise
 *******************************************************************************************************/
        bool operator < (const meta &rt) const { return loc < rt.loc && index < rt.index; }


/********************************************************************************************************
 * is the meta an array?
 * @return              true if array, false otherwise
 *******************************************************************************************************/
        bool isArray() { return num_dims != 0; }

    } meta_t;   // typedef struct meta


/************************************************************************************************************
 * @class BufMan
 * PAX buffer manipulation object.
 ***********************************************************************************************************/
    class BufMan {
    public:
/************************************************************************************************************
 * Ctor for internal type.
 * @param[in]       buf     Shared pointer to the buffer
 * @param[in]       len     Buffer length
 ***********************************************************************************************************/
        BufMan(paxBufPtr buf, const size_t len) :
            _buf{ buf },
            _pos{ NULL },
            _len{ len },
            _metaLoc{ metaLoc::LOC_AFTER_TAG }
        {

            if (buf) _pos = buf->data();
            _start = _pos;
            PAX_LOG(2, << "created buf of length " << len);

        } // BufMan(paxBufPtr buf, const size_t len)


/************************************************************************************************************
 * Ctor for raw buffer
 * @param[in]       buf     Buffer address
 * @param[in]       len     Buffer length
 ***********************************************************************************************************/
        BufMan(paxBufData_t *buf, const size_t len) :
            _buf{ nullptr },
            _pos{ buf },
            _len{ len },
            _metaLoc{ metaLoc::LOC_AFTER_TAG }
        {

            _start = _pos;
            PAX_LOG(2, << "imported buf of length " << len);

        } // BufMan(paxBufData_t *buf, const size_t len)


/************************************************************************************************************
 * Tests the buffer for end-of-file
 * @param[in]       pos     Optional buffer address. If NULL, uses internal buffer
 * @return                  true if end-of-file found, false otherwise
 ***********************************************************************************************************/
        bool eof(char *pos = NULL) {

            if (!pos) pos = _pos;
            return (pos - _start) >= (int64_t)_len;

        } // bool eof(char *pos = NULL) {


/************************************************************************************************************
 * Sets the current location and index for storing metadata.
 * @param[in]       raster  Index of the raster for the next metadata.
 * @param[in]       index   Index within the raster.
 * @return                  status
 ***********************************************************************************************************/
        int setLoc(const metaLoc_e loc, const paxMetaLoc_t index) {

            _metaLoc = loc;
            _metaIdx = index;

            return PAX_OK;

        } // int setLoc(const paxMetaLoc_t loc, const paxMetaLoc_t index) {


/********************************************************************************************************
 * Advance the internal buffer past the next LF
 * @return                  true if end-of-file found, false otherwise
 *******************************************************************************************************/
        bool skipLine() {

            char * oldpos = _pos;
            _pos = strchr(_pos, '\n') + 1;
            PAX_LOG(3, << "skipLine advanced " << _pos - oldpos << " characters");

            return eof();

        } // bool skipLine() {


/********************************************************************************************************
 * Advance past the next LF
 * @param[in,out]   pos     Buffer to be advanced
 * @return                  true
 *******************************************************************************************************/
        static bool skipLine(char *&pos) {

            char * oldpos = pos;
            pos = strchr(pos, '\n') + 1;
            PAX_LOG(3, << "skipLine advanced " << pos - oldpos << " characters");

            return true;

        } // static bool skipLine(char *&pos) {


/********************************************************************************************************
 * Advance past the next chunk of whitespace. LF's are skipped if the 2nd argument is true
 * @param[in,out]   pos     Buffer to be advanced
 * @param[in]       skipLF  optional flag to skip linefeeds
  *******************************************************************************************************/
        static void skipWS(char *& pos, const bool skipLF = true) {

            char * oldpos = pos;

            while (*pos == ' ' || *pos == '\t' || *pos == '\r' || (skipLF && *pos == '\n')) ++pos;
            if (pos != oldpos) PAX_LOG(3, << "skipped " << (pos - oldpos) << " whitespace characters");

        } // static void skipWS(char *& pos, const bool skipLF = true) {


/********************************************************************************************************
 * Advance to the next chunk of whitespace, delimiter, or brace. LF's are skipped if the 2nd arg is true.
 * @param[in,out]   pos     Buffer to be advanced
 * @param[in]       skipLF  optional flag to skip linefeeds
 *******************************************************************************************************/
        static void skipJunk(char *& pos, bool skipLF = true) {

            char * oldpos = pos;

            // LF terminates the junk, so we check for it at the end
            while (*pos != '#' && *pos != ' ' && *pos != '\t' && *pos != '\r' && *pos != ':' && *pos != '=' && *pos != '[' && *pos != ']' && /*(skipLF ||*/ *pos != '\n'/*)*/) {
                ++pos;
            }

            if (skipLF && *pos == '\n') {
                ++pos;
            }

            if (pos != oldpos) PAX_LOG(3, << "skipped " << (pos - oldpos) << " junk characters");

        } // static void skipJunk(char *& pos, bool skipLF = true) {


/********************************************************************************************************
 * Advance past the next chunk of junk and whitespace. LF's are skipped if the 2nd arg is true.
 * @param[in,out]   pos     Buffer to be advanced
 * @param[in]       skipLF  optional flag to skip linefeeds
 *******************************************************************************************************/
        static void skipJunkAndWS(char *& pos, bool skipLF = true) {

            skipJunk(pos, skipLF);
            skipWS(pos, skipLF);

        } // static void skipJunkAndWS(char *& pos, bool skipLF = true) {


/********************************************************************************************************
 * Advance past whitespace, delimiter, whitespace. LF's are also skipped if the 2nd arg is true.
 * @param[in,out]   pos     Buffer to be advanced
 * @param[in]       skipLF  optional flag to skip linefeeds
 *******************************************************************************************************/
        static void skipDelimiter(char *& pos, bool skipLF = true) {

            skipWS(++pos, skipLF);
            while (*pos != ':' && *pos != '=') ++pos;
            skipWS(++pos, skipLF);

        } // static void skipDelimiter(char *& pos, bool skipLF = true) {


/********************************************************************************************************
 * Jump Advance junk, whitespace, specified char, whitespace. LF's are skipped if the 2nd arg is true.
 * @param[in]       skipme  character to be skipped
 * @param[in,out]   pos     Buffer to be advanced
 * @param[in]       skipLF  optional flag to skip linefeeds
 *******************************************************************************************************/
        static void skipChar(const char skipme, char *& pos, const bool skipLF = true) {

            skipJunkAndWS(pos, skipLF);
            // TODO: failure if next character is not closing brace?
            if (*pos != skipme) return;
            skipWS(++pos, skipLF);

        } // static void skipChar(const char skipme, char *& pos, const bool skipLF = true) {


/********************************************************************************************************
 * String-insensitive comparison of the internal buffer.
 * @param[in]       str     NULL-terminated string to be compared
 * @return                  true if strings match, false otherwise
 *******************************************************************************************************/
        bool compare(const char * str) {

            // Both strings must be null-terminated for stricmp.
            int len = (int)strlen(str);
            TempNull tn(&_pos[len]);

            int res = _stricmp(_pos, str);

            PAX_LOG(4, << "result of comparing '" << _pos << "' and '" << str << "': " << res);

            return res != 0;

        } // bool compare(const char * str) {


/***************************************************************************************************
 * Classify the line the internal buffer is currently pointing at.
 * @return                  PAX linetype
 **************************************************************************************************/
        headerLineType_t getHeaderLineType() {

            skipWS(_pos);
            if ('#' == _pos[0]) {
                PAX_LOG(3, << "found comment line");
                return headerLineType_t::COMMENT;
            }

            if ('@' == _pos[0]) {
                PAX_LOG(3, << "found metadata line");
                return headerLineType_t::METADATA;
            }

            if (!compare(PAX_TAG))      return hlType_t::PAX;
            if (!compare(BPV_TAG))      return hlType_t::BPV;
            if (!compare(VPE_TAG))      return hlType_t::VPE;
            if (!compare(DATALEN_TAG))  return hlType_t::DATALEN;

            // TODO: search for dim tags in generic fashion; for now revert to old 2-dimensional form
            if (!compare(DIM1_TAG)) {
                _dimTagIndex = 0;
                return hlType_t::DIM;
            }
            if (!compare(DIM2_TAG)) {
                _dimTagIndex = 1;
                return hlType_t::DIM;
            }

            // output a chunk of data upon failure
            if (PaxStatic::getVerbosity() >= 2) {
                TempNull tn(_pos + 32);
                PAX_LOG_ERROR(0, << "Unknown header line: " << _pos);
            }

            return hlType_t::UNKNOWN;

        } // headerLineType_t getHeaderLineType () 


/********************************************************************************************************
 * @def GETVAL_DEFAULTSKIP Default skip behavior
 *******************************************************************************************************/
#define GETVAL_DEFAULTSKIP skipFlags::SKIP_DELIMITER

/********************************************************************************************************
 * Extract a delimited float from the internal buffer.
 * @param[in]       skip    skip behavior
 * @return                  extracted value
 *******************************************************************************************************/
        float getFloat(const skipFlags_e skip = GETVAL_DEFAULTSKIP) {

            if (skip & skipFlags::SKIP_DELIMITER) {
                skipDelimiter(_pos);
            } // else: strto* extracts WS so we don't need to

            float val = strtof(_pos, &_pos);
            skipJunkAndWS(_pos, skipFlags::SKIP_NOTHING != (skip & skipFlags::SKIP_LINEFEED));  // whitespace or LF required after value.

            PAX_LOG(3, << "read a float from buffer: " << val);

            return val;

        } // float getFloat(const skipFlags_e skip = GETVAL_DEFAULTSKIP) {


/********************************************************************************************************
 * Extract a delimited double from the internal buffer.
 * @param[in]       skip    skip behavior
 * @return                  extracted value
 *******************************************************************************************************/
        double getDouble(const skipFlags_e skip = GETVAL_DEFAULTSKIP) {

            if (skip & skipFlags::SKIP_DELIMITER) {
                skipDelimiter(_pos);
            } // else: strto* extracts WS so we don't need to

            double val = strtod(_pos, &_pos);
            skipJunkAndWS(_pos, skipFlags::SKIP_NOTHING != (skip & skipFlags::SKIP_LINEFEED));  // whitespace or LF required after value.

            PAX_LOG(3, << "read a double from buffer: " << val);

            return val;

        } // double getDouble(const skipFlags_e skip = GETVAL_DEFAULTSKIP) {


/********************************************************************************************************
 * Extract a delimited int64 from the internal buffer.
 * @param[in]       skip    skip behavior
 * @return                  extracted value
 *******************************************************************************************************/
        int64_t getInt64(const skipFlags_e skip = GETVAL_DEFAULTSKIP) {

            if (skip & skipFlags::SKIP_DELIMITER) {
                skipDelimiter(_pos);
            } // else: strto* extracts WS so we don't need to

            int64_t val = strtoll(_pos, &_pos, 0);
            skipJunkAndWS(_pos, skipFlags::SKIP_NOTHING != (skip & skipFlags::SKIP_LINEFEED));  // whitespace or LF required after value.

            PAX_LOG(3, << "read an int64_t from buffer: " << val);

            return val;

        } // int64_t getInt64(const skipFlags_e skip = GETVAL_DEFAULTSKIP) {


/********************************************************************************************************
 * Extract a delimited uint64 from the internal buffer.
 * @param[in]       skip    skip behavior
 * @return                  extracted value
 *******************************************************************************************************/
        uint64_t getUint64(const skipFlags_e skip = GETVAL_DEFAULTSKIP) {

            if (skip & skipFlags::SKIP_DELIMITER) {
                skipDelimiter(_pos);
            } // else: strto* extracts WS so we don't need to

            uint64_t val = strtoull(_pos, &_pos, 0);
            skipJunkAndWS(_pos, skipFlags::SKIP_NOTHING != (skip & skipFlags::SKIP_LINEFEED));  // whitespace or LF required after value.

            PAX_LOG(3, << "read a uint64_t from buffer: " << val);

            return val;

        } // uint64_t getUint64(const skipFlags_e skip = GETVAL_DEFAULTSKIP) {


/********************************************************************************************************
 * Extract a delimited int32 from the internal buffer.
 * @param[in]       skip    skip behavior
 * @return                  extracted value
 *******************************************************************************************************/
        int32_t getInt32(const skipFlags_e skip = GETVAL_DEFAULTSKIP) {

            if (skip & skipFlags::SKIP_DELIMITER) {
                skipDelimiter(_pos);
            } // else: strto* extracts WS so we don't need to

            int32_t val = strtol(_pos, &_pos, 0);
            skipJunkAndWS(_pos, skipFlags::SKIP_NOTHING != (skip & skipFlags::SKIP_LINEFEED));  // whitespace or LF required after value.

            PAX_LOG(3, << "read an int32_t from buffer: " << val);

            return val;

        } // int32_t getInt32(const skipFlags_e skip = GETVAL_DEFAULTSKIP) {


/********************************************************************************************************
 * Extract a delimited uint32 from the internal buffer.
 * @param[in]       skip    skip behavior
 * @return                  extracted value
 *******************************************************************************************************/
        uint32_t getUint32(const skipFlags_e skip = GETVAL_DEFAULTSKIP) {

            if (skip & skipFlags::SKIP_DELIMITER) {
                skipDelimiter(_pos);
            } // else: strto* extracts WS so we don't need to

            uint32_t val = strtoul(_pos, &_pos, 0);
            skipJunkAndWS(_pos, skipFlags::SKIP_NOTHING != (skip & skipFlags::SKIP_LINEFEED));  // whitespace or LF required after value.

            PAX_LOG(3, << "read a uint32_t from buffer: " << val);

            return val;

        } // uint32_t getUint32(const skipFlags_e skip = GETVAL_DEFAULTSKIP) {


/********************************************************************************************************
 * Extract a delimited int16 from the internal buffer.
 * @param[in]       skip    skip behavior
 * @return                  extracted value
 *******************************************************************************************************/
        int16_t getInt16(const skipFlags_e skip = GETVAL_DEFAULTSKIP) {

            if (skip & skipFlags::SKIP_DELIMITER) {
                skipDelimiter(_pos);
            } // else: strto* extracts WS so we don't need to

            int16_t val = (int16_t)strtol(_pos, &_pos, 0);
            skipJunkAndWS(_pos, skipFlags::SKIP_NOTHING != (skip & skipFlags::SKIP_LINEFEED));  // whitespace or LF required after value.

            PAX_LOG(3, << "read an int16_t from buffer: " << val);

            return val;

        } // int16_t getInt16(const skipFlags_e skip = GETVAL_DEFAULTSKIP) {


/********************************************************************************************************
 * Extract a delimited uint16 from the internal buffer.
 * @param[in]       skip    skip behavior
 * @return                  extracted value
 *******************************************************************************************************/
        uint16_t getUint16(const skipFlags_e skip = GETVAL_DEFAULTSKIP) {

            if (skip & skipFlags::SKIP_DELIMITER) {
                skipDelimiter(_pos);
            } // else: strto* extracts WS so we don't need to

            uint16_t val = (uint16_t)strtoul(_pos, &_pos, 0);
            skipJunkAndWS(_pos, skipFlags::SKIP_NOTHING != (skip & skipFlags::SKIP_LINEFEED));  // whitespace or LF required after value.

            PAX_LOG(3, << "read a uint16_t from buffer: " << val);

            return val;

        } // uint16_t getUint16(const skipFlags_e skip = GETVAL_DEFAULTSKIP) {


/********************************************************************************************************
 * Extract a delimited int8 from the internal buffer.
 * @param[in]       skip    skip behavior
 * @return                  extracted value
 *******************************************************************************************************/
        int8_t getInt8(const skipFlags_e skip = GETVAL_DEFAULTSKIP) {

            if (skip & skipFlags::SKIP_DELIMITER) {
                skipDelimiter(_pos);
            } // else: strto* extracts WS so we don't need to

            int8_t val = (int8_t)strtol(_pos, &_pos, 0);
            skipJunkAndWS(_pos, skipFlags::SKIP_NOTHING != (skip & skipFlags::SKIP_LINEFEED));  // whitespace or LF required after value.

            PAX_LOG(3, << "read an int8_t from buffer: " << val);

            return val;

        } // int8_t getInt8(const skipFlags_e skip = GETVAL_DEFAULTSKIP) {


/********************************************************************************************************
 * Extract a delimited uint8 from the internal buffer.
 * @param[in]       skip    skip behavior
 * @return                  extracted value
 *******************************************************************************************************/
        uint8_t getUint8(const skipFlags_e skip = GETVAL_DEFAULTSKIP) {

            if (skip & skipFlags::SKIP_DELIMITER) {
                skipDelimiter(_pos);
            } // else: strto* extracts WS so we don't need to

            uint8_t val = (uint8_t)strtoul(_pos, &_pos, 0);
            skipJunkAndWS(_pos, skipFlags::SKIP_NOTHING != (skip & skipFlags::SKIP_LINEFEED));  // whitespace or LF required after value.

            PAX_LOG(3, << "read a uint8_t from buffer: " << val);

            return val;

        } // uint8_t getUint8(const skipFlags_e skip = GETVAL_DEFAULTSKIP) {


/********************************************************************************************************
 * Extract an name/metadata pair from the buffer.
 * @return                  pair containing name and meta
 *******************************************************************************************************/
        std::pair <std::string, meta_t>&& getMeta() {

            meta_t meta1;
            std::string name;
            std::pair <std::string, meta_t> badMeta("", meta_t{});

            // our current pos should be at the "#" or "##"
            if ('#' != *_pos) {
                PAX_LOG_ERROR(1, << "Attempted to extract metadata but no # found.");
                return std::move(badMeta);
            }

            char * pos = _pos + 1;

            if ('#' != *pos) {
              ///////////////////////////////////////////////////////////////////////////
              // read the comment
              //
                char * eol = strchr(pos, '\n');

                // check for buffer overrun
                if (eof(eol)) {
                    PAX_LOG_ERROR(1, << "Unexpected EOF reading PAX buffer. This may be expected if previewing a long header.");
                    return std::move(badMeta);
                }

                name = meta::getCommentName(_metaLoc, _metaIdx);

                int len = (int)(eol - pos);
                if (len >= PAX_MAX_METADATA_STRING_LENGTH) len = PAX_MAX_METADATA_STRING_LENGTH - 1;
                if (pos[len - 1] == '\r') --len;

                // trim the leading space if it is there
                if (' ' == pos[0]) {
                    ++pos;
                    --len;
                    meta1.stripped = true;
                }

                meta1.type = paxMetaDataTypes_e::paxComment;
                meta1.s[0] = '\0';
                pax_strncat(meta1.s, pos, len);

                // set buffer pointer past end of line
                _pos = eol + 1;

            } else {
              ///////////////////////////////////////////////////////////////////////////
              // metadata
              //
                ++pos; // skip the second '#'
                skipChar('[', pos);  // skip past the opening brace
                meta1.type = paxMetaDataTypes_e::paxInvalid;
                char typeTag[METATYPE_MAX_TAG_LEN + 1];

                // extract and identify the type
                for (int i = (int)paxMetaDataTypes::paxMetaStart; i <= (int)paxMetaDataTypes::paxMetaEnd; ++i) {

                    paxMetaDataTypes_e type = (paxMetaDataTypes_e)i;
                    const char * tag = PaxStatic::getMetaTypeTag(type);
                    TempNull tn(pos + strlen(tag));

                    if (0 == _stricmp(tag, pos)) {

                        meta1.type = type;
                        pax_strcpy(typeTag, pos);
                        PAX_LOG(4, << "Metadata type match! " << pos << " = type " << (int)meta1.type);
                        tn.restore();
                        skipChar(']', pos);

                        break;

                    }

                }

                // bad metadata type
                if (paxMetaDataTypes_e::paxInvalid == meta1.type) {

                    TempNull tn(pos + strlen(METATYPE_DOUBLE_TAG));
                    PAX_LOG_ERROR(0, << "Metadata type not found: " << pos);
                    skipLine(_pos);

                    return std::move(badMeta);

                }

                // Read the meta name. Whitespace, delimiter, or opening brace stops the search.
                int len = 0;
                while (pos[len] != ' ' && pos[len] != '\t' && pos[len] != ':' && pos[len] != '=' && pos[len] != '[') {
                    ++len;
                }

                name.append(pos, len);
                PAX_LOG(3, << "Metadata name is " << name.c_str() << ", type is " << (int)meta1.type);

                _pos = pos + len; // skip the name, swich back to internal buffer pointer
                skipWS(_pos);

                size_t values = 1;
                // Check for an array
                if ('[' == _pos[0]) {
                    skipWS(++_pos); // skip the opening brace and WS

                    // search for indexes in order
                    static const char * metaArrayIndexTags[METAARRAYINDEXES] = {
                      METAARRAYINDEX_FIRST_TAG,
                      METAARRAYINDEX_SECOND_TAG,
                      METAARRAYINDEX_THIRD_TAG,
                      METAARRAYINDEX_FOURTH_TAG
                    };

                    std::vector<uint32_t> dims;
                    int indexes = 0;

                    for (int i = 0; i < METAARRAYINDEXES; ++i) {

                        const char * tag = PaxStatic::getMetaArrayIndexTag(i);
                        TempNull tn(_pos + strlen(tag));

                        if (0 == pax_stricmp(tag, _pos)) {
                          // found the ith index; skip to value
                            tn.restore();
                            uint32_t dim = getUint32();
                            PAX_LOG(4, << "    (meta array) " << std::setw(6) << tag << " dim = " << dim);
                            dims.push_back(dim);
                        } else {
                            break;
                        }
                    }

                    skipChar(']', _pos); // skip any extra stuff in the index list

                    values = meta1.initArray(meta1.type, dims);

                } // if ('[' == pos[0]) (meta array input)

                while (*_pos != ':' && *_pos != '=') ++_pos;    // skip the delimiter ':' or '='
                ++_pos;
                char *eol;

                // Should be pointing at the data now (or perhaps whitespace)
                if (1 == values) {

                    switch (meta1.type) {

                    case paxMetaDataTypes_e::paxString:
                        eol = strchr(_pos, '\n');
                        len = (int)(eol - _pos);
                        if (len >= PAX_MAX_METADATA_STRING_LENGTH) len = PAX_MAX_METADATA_STRING_LENGTH - 1;
                        if (_pos[len - 1] == '\r') len--;

                        // trim the leading space if it is there
                        if (' ' == _pos[0]) {
                            ++_pos;
                            --len;
                            meta1.stripped = true;
                        }

                        meta1.s[0] = '\0';
                        pax_strncat(meta1.s, _pos, len);

                        // set buffer pointer at end of this line
                        _pos = eol;
                        break;

                    case paxMetaDataTypes_e::paxFloat:
                        meta1.f = getFloat(skipFlags::SKIP_NOTHING);
                        break;

                    case paxMetaDataTypes_e::paxDouble:
                        meta1.d = getDouble(skipFlags::SKIP_NOTHING);
                        break;

                    case paxMetaDataTypes_e::paxInt64:
                    case paxMetaDataTypes_e::paxInt32:
                    case paxMetaDataTypes_e::paxInt16:
                    case paxMetaDataTypes_e::paxInt8:
                        // read and store as 64-bit and trust the user to call the right accessor fn
                        meta1.n64 = getInt64(skipFlags::SKIP_NOTHING);
                        break;

                    case paxMetaDataTypes_e::paxUint64:
                    case paxMetaDataTypes_e::paxUint32:
                    case paxMetaDataTypes_e::paxUint16:
                    case paxMetaDataTypes_e::paxUint8:
                        // read and store as 64-bit and trust the user to call the right accessor fn
                        meta1.u64 = getUint64(skipFlags::SKIP_NOTHING);
                        break;

                    default:
                        PAX_LOG_ERROR(1, << "I don't know how to import metadata of type " << typeTag << "yet! skipping it...");
                        skipJunkAndWS(_pos, false);
                        break;
                    }

                } else {

                    // KLUDGE: skip LF's as data is read in, then roll back one char
                    for (size_t i = 0; i < values; ++i) {

                        // all types should leave the buffer pointer at the beginning of the next element or the end of the line
                        switch (meta1.type) {

                        case paxMetaDataTypes_e::paxFloat:
                            meta1.fb[i] = getFloat(skipFlags::SKIP_NOTHING);
                            break;

                        case paxMetaDataTypes_e::paxDouble:
                            meta1.db[i] = getDouble(skipFlags::SKIP_NOTHING);
                            break;

                        case paxMetaDataTypes_e::paxInt64:
                            meta1.n64b[i] = getInt64(skipFlags::SKIP_NOTHING);
                            break;

                        case paxMetaDataTypes_e::paxInt32:
                            meta1.n32b[i] = getInt32(skipFlags::SKIP_NOTHING);
                            break;

                        case paxMetaDataTypes_e::paxInt16:
                            meta1.n16b[i] = getInt16(skipFlags::SKIP_NOTHING);
                            break;

                        case paxMetaDataTypes_e::paxInt8:
                            meta1.n8b[i] = getInt8(skipFlags::SKIP_NOTHING);
                            break;

                        case paxMetaDataTypes_e::paxUint64:
                            meta1.u64b[i] = getUint64(skipFlags::SKIP_NOTHING);
                            break;

                        case paxMetaDataTypes_e::paxUint32:
                            meta1.u32b[i] = getUint32(skipFlags::SKIP_NOTHING);
                            break;

                        case paxMetaDataTypes_e::paxUint16:
                            meta1.u16b[i] = getUint16(skipFlags::SKIP_NOTHING);
                            break;

                        case paxMetaDataTypes_e::paxUint8:
                            meta1.u8b[i] = getUint8(skipFlags::SKIP_NOTHING);
                            break;

                        default:
                            PAX_LOG_ERROR(1, << "I don't know how to import array metadata of type " <<
                                typeTag << "yet! skipping it...");
                            skipJunkAndWS(_pos);
                            break;
                        }
                    } // for (size_t i = 0; i < values; ++i) {

                } // if (...) : reading array data

                skipLine(_pos);

            } // reading metadata

            meta1.loc   = _metaLoc;
            meta1.index = _metaIdx;
            meta1.name  = name;
            ++_metaIdx;

            return std::pair<std::string, meta_t>(name, meta1);

        }   // std::pair <std::string, meta_t>&& getMeta() {


/********************************************************************************************************
 * Copy binary raster data to the given buffer.
 * @param[out]      buf     User-supplied output buffer
 * @param[in]       len     bytes to be copied
 * @return                  number of bytes copied
 *******************************************************************************************************/
        size_t copyData(char * buf, const size_t len)   {

            ptrdiff_t remain = _len - (_pos - _start);
            if (len > static_cast<size_t>(remain)) {

                PAX_LOG_ERROR(1, << " insufficient buffer length. " << remain << " bytes remain but " <<
                    len << " requested");

                return 0;
            }

            memcpy(buf, _pos, len);
            _pos += len;
            PAX_LOG(2, << "copied " << len << " bytes of raster data from buffer. " << remain - len <<
                " bytes remaining.");

            return len;

        } // size_t copyData(char * buf, const size_t len)


/********************************************************************************************************
 * Direct access to the internal buffer.
 * @return                  reference to the internal buffer
 *******************************************************************************************************/
        char *& pos() { return _pos; }


/********************************************************************************************************
 * Get the current buffer offset
 * @return                  Internal buffer position in bytes
 *******************************************************************************************************/
        size_t offset() {
            if (_start > _pos) {
                PaxStatic::setStatus(PAX_FAIL);
                return 0;
            }

            return _pos - _start;

        } // size_t offset() {


/********************************************************************************************************
 * Temporary buffer access
 * @return                  Pointer to internal buffer
 *******************************************************************************************************/
        char * operator()() { return _pos; }


/********************************************************************************************************
 * Direct byte-by-byte data access.
 * @param[in]       pos     Desired offset
 * @return                  Reference to the byte at requested buffer offset
 *******************************************************************************************************/
        char &operator[](const size_t pos) { return _pos[pos]; }

/********************************************************************************************************
 * Artificially truncates the buffer if applicable. Throws warning if buffer is not long enough.
 * @param[in]       pos     Desired buffer length
 * @return                  Resulting buffer length
 *******************************************************************************************************/
        size_t truncate(const size_t pos) {

            if (_len >= pos) {
                _len = pos;
            } else {
                PAX_LOG_WARN(2, << "Tried to truncate a PAX buffer that is not long enough.");
            }

            return _len;

        } // size_t truncate(const size_t pos) {


/********************************************************************************************************
 * Returns the last detected dimension index
 * @param[in]       pos     Desired buffer length
 * @return                  Resulting buffer length
 *******************************************************************************************************/
        size_t getDimTagIndex() {

            return _dimTagIndex;

        } // size_t getDimTagIndex() {

    protected:

        paxBufPtr       _buf;           ///< The buffer
        paxBufData_t *  _pos;           ///< Current buffer offset
        paxBufData_t *  _start;         ///< Pointer to start of buffer
        size_t          _len;           ///< Current buffer length
        //PaxMetaLoc      _metaLoc;     // TODO: PaxMetaLoc
        metaLoc_e       _metaLoc;       ///< Current location for storing metadata
        size_t          _metaIdx;       ///< Current index for storing metadata within current location
        size_t          _dimTagIndex;   ///< TEMPCODE: index of last identified dimension tag

    };  // class BufMan

/************************************************************************************************************
 * @class PaxHeader
 * Defines and manipulates the PAX file header.
 ***********************************************************************************************************/
    class PaxHeader {

    public:
/************************************************************************************************************
 * Default Ctor
 ***********************************************************************************************************/
        PaxHeader() { }

    private:
        
        paxHeaderHashMap_t _hash;       ///< stores hashes for each raster, in the order they will be rendered
        paxHeaderMetaMap_t _meta;       ///< stores meta for each raster

    }; // class PaxHeader

/************************************************************************************************************
 * @class PaxMetaLoc
 * Location information for a PAX metadata object.
 ***********************************************************************************************************/
    class PaxMetaLoc {

    public:
/*******************************************************************************************************
 * Default Ctor.
 ******************************************************************************************************/
        PaxMetaLoc() : _raster(0), _index(0) { }
/*******************************************************************************************************
 * Ctor with raster and index.
 * @param[in]       raster  The raster index
 * @param[in]       index   Index within this raster
******************************************************************************************************/
        PaxMetaLoc(const paxMetaLoc_t raster, const paxMetaLoc_t index) : _raster(raster), _index(index) { }

    protected:
        paxMetaLoc_t _raster;       ///< Index of the raster this location refers to (TODO: always 0)
        paxMetaLoc_t _index;        ///< Region-specific index

    }; // class PaxMetaLoc


/************************************************************************************************************
 * @class PaxBuf
 * Defines and manipulates a buffer containing a PAX file. TODO: merge with BufMan.
 ***********************************************************************************************************/
    class PaxBuf {
    public:

/*******************************************************************************************************
 * Ctor accepting a PaxArray.
 * @param[in]       inBuf   xvalue identifying the buffer containing the file to be loaded.
 ******************************************************************************************************/
        PaxBuf(paxBuf_t &&inBuf) : _buf(inBuf) { }

    private:

    protected:
        paxBuf_t _buf;                      ///< The buffer
        char * _pos;                        ///< Iterator addressing the current buffer position
        size_t _headerLen;                  ///< The length of the header (0 if not yet known)

    };  // class PaxBuf

/************************************************************************************************************
 * @class PaxMeta
 * Defines and manipulates PAX metadata objects
 ***********************************************************************************************************/
    class PaxMeta {

    public:
        /************************************************************************************************************
        * Ctor accepting a paxMeta_t
        * @tparam _valueIn Generic-typed value
        ***********************************************************************************************************/
        PaxMeta(paxMeta_t _valueIn) {
            _data = std::move(_valueIn);
        }

    protected:
        PaxMetaLoc      _loc;       ///< Location of this meta object
        paxMeta_t       _data;      ///< The metadata
    }; // class PaxMeta


/************************************************************************************************************
 * @class paxMeta_td
 * Defines and manipulates strongly-typed PAX metadata objects.
 * @tparam T Internal type of the metadata
 ***********************************************************************************************************/
    template <class T>
    class paxMeta_td : public PaxMeta{

    public:

        /************************************************************************************************************
         * Ctor accepting a parameter of generic type
         * @tparam valueIn Generic-typed value
         ***********************************************************************************************************/
        paxMeta_td(T valueIn) : PaxMeta({ valueIn }) { }

        /************************************************************************************************************
         * Extract value
         * @return Generic-type value
         ***********************************************************************************************************/
        T get() { return std::get<T>(_data); }

    private:
    }; // class paxMeta_td


/************************************************************************************************************
 * @class Pax
 * The fundamental container class for raster data of arbitrary type.
 * @tparam _BPV Number of Bytes Per Value
 * @tparam _VPE Number of Values Per Element
 **********************************************************************************************************/
    template <paxBpv_t _BPV, paxVpe_t _VPE>
    class Pax {

    public:

/************************************************************************************************************
 * Ctor accepting dimensions and optional data buffer.
 * If the buffer is supplied, copies the data into internal storage.
 * @param[in]       dimsIn  The new raster dimensions.
 * @param[in]       dataIn  Optional input buffer. Must be at least as large as all of the data.
 ***********************************************************************************************************/
        Pax(const std::vector <paxDim_t>& dimsIn, const void * dataIn = NULL) {

            initHeader();
            size_t bytes = resize(dimsIn);

            if (NULL != dataIn) {
                memcpy(rawData.data(), dataIn, bytes);
            }

        } // Pax(...)

/************************************************************************************************************
 * Ctor accepting a file buffer.
 * @param[in]       inBuf   The buffer containing the file to be loaded.
 ***********************************************************************************************************/
        Pax(PaxBuf inBuf) {
            // TODO: implement
        } // Pax(string file)

/************************************************************************************************************
 * Ctor accepting a file name.
 * @param[in]       inFile  The file to be loaded.
 ***********************************************************************************************************/
        Pax(const std::string& inFile) {

        } // Pax(string file)

/************************************************************************************************************
 * Get the total size of the raster.
 * @return Total size of the raster in bytes
 ***********************************************************************************************************/
        size_t size() {

            size_t _elements = elements();
            size_t bytes = _elements * _BPV * _VPE;

            return bytes;

        } // size()

/************************************************************************************************************
 * Get the total number of elements in the raster.
 * @return Total number of elements in the raster
 ***********************************************************************************************************/
        size_t elements() {

            size_t _elements = std::reduce(dims.begin(), dims.end(), 1, std::multiplies<size_t>());

            return _elements;

        } // elements()

/************************************************************************************************************
 * Header access.
 * @return Reference to the header object
 ***********************************************************************************************************/
        auto header() -> PaxHeader & {

            return *hdr;

        } // header()

    protected:

/************************************************************************************************************
 * Header initialization.
 ***********************************************************************************************************/
        void initHeader() {

        }

        std::unique_ptr<PaxHeader>  hdr;        ///< The header for this PAX file
        std::vector <paxDim_t>      dims;       ///< The dimensions of the raster
        std::vector <uint8_t>       rawData;    ///< Single buffer for raster data

/********************************************************************************************************
 * Resizes the raster, preserving existing data.
 * Warning, does not preserve index. If interior dimensions change, the data will NOT aligned correctly.
 * @param[in]       dimsIn  The new raster dimensions.
 * @return                  The total number of elements in the raster.
 ********************************************************************************************************/
        size_t resize(const std::vector <paxDim_t>& dimsIn) {

            dims = dimsIn;
            size_t bytes = size();

            if (0 == bytes) {
                dims = { 0 };
                return 0;
            }

            rawData.resize(bytes);

            return bytes;

        } // resize

    }; // class Pax


/********************************************************************************************************
 * @class PaxScalar
 * The primary container class for rasters containing scalar data (single value per element).
 * @tparam _ET The type of each element
 *******************************************************************************************************/
    template <typename _ET>
    class PaxScalar : Pax <sizeof(_ET), 1> {

    public:
/*******************************************************************************************************
 * Ctor accepting dimensions and optional data buffer.
 * If the buffer is supplied, copies the data into internal storage.
 * @param[in]       dimsIn  The new raster dimensions.
 * @param[in]       dataIn  Optional input buffer. Must be at least as large as all of the data.
 ******************************************************************************************************/
        PaxScalar(const std::vector <paxDim_t>& dimsIn, void * dataIn = NULL) :
            Pax<sizeof(_ET), 1>(dimsIn, dataIn) { }

    protected:

    }; // class PaxScalar


/*******************************************************************************************************
 * @class PaxVector
 * The primary container class for rasters containing vector data (multiple values per element).
 * @tparam _ET The type of each element
 * @tparam _VPE Number of values per element
 ******************************************************************************************************/
    template <typename _ET, paxVpe_t _VPE>
    class PaxVector : public Pax<sizeof(_ET), _VPE> {

    public:
/*******************************************************************************************************
 * Ctor accepting dimensions and optional data buffer. If
 * the buffer is supplied, copies the data into internal storage.
 * @param[in]       dimsIn  The new raster dimensions.
 * @param[in]       dataIn  Optional input buffer. Must be at least as large as all of the data.
 ******************************************************************************************************/
        PaxVector(const std::vector<paxDim_t>& dimsIn, void * dataIn = NULL) :
            Pax<sizeof(_ET), _VPE>(dimsIn, dataIn) { }

    }; // class PaxVector


/********************************************************************************************************
 * @name PAX_LEGACY the following code is part of the original C++ library that will eventually be
 * replaced by the Pax, PaxScalar, PaxVector, etc. classes.
 *******************************************************************************************************/
///@{

  //////////////////////////////////////////////////////////////////////////
  //
  // PAX value spaces: defines values per element for various types of data.
  //
    typedef class ValueSpaceStatic
    {

    public:

      //////////////////////////////////////////////////////////////////////////
      //
      // REAL*, IMAGINARY*, COMPLEX* specify value-interleaved multidimensional data.
      //
      // An example of REAL3 data would be latitude, longitude, elevation for a grid of pixels.
      // An example of COMPLEX2 data would be signal for a two-channel system.
      // An example of POLAR4 data would be 8-bit mag/phase for a four-channel radar system.
      // Note that data such as GFF IQ2 requires separate buffers for REAL and IMAGINARY.
      // BITS is not directly supported, but can be used for packed binary data.
      //    Value space name,     enum,  vpe
#define PAX_VALUE_SPACE_DATA                                    \
      /* generic numbers */                                       \
      X(REAL,                   0,    1                         ) \
      X(IMAGINARY,              1,    1                         ) \
      X(COMPLEX,                2,    2                         ) \
      X(POLAR,                  3,    2                         ) \
      X(REAL2,                  4,    2                         ) \
      X(IMAGINARY2,             5,    2                         ) \
      X(COMPLEX2,               6,    4                         ) \
      X(POLAR2,                 7,    4                         ) \
      X(REAL3,                  8,    3                         ) \
      X(IMAGINARY3,             9,    3                         ) \
      X(COMPLEX3,              10,    6                         ) \
      X(POLAR3,                11,    6                         ) \
      X(REAL4,                 12,    4                         ) \
      X(IMAGINARY4,            13,    4                         ) \
      X(COMPLEX4,              14,    8                         ) \
      X(POLAR4,                15,    8                         ) \
      X(BITS,                  19,    1                         ) \
      /* SAR data */                                              \
      X(MAG,                   20,    1                         ) \
      X(PHASE,                 21,    1                         ) \
      X(MAGPHASE,              22,    2                         ) \
      X(IQ,                    23,    2                         ) \
      /* color spaces */                                          \
      X(RGB,                   31,    3                         ) \
      X(HSV,                   32,    3                         ) \
      /* misc */                                                  \
      X(ONE,                  901,    1                         ) \
      X(TWO,                  902,    2                         ) \
      X(THREE,                903,    3                         ) \
      X(FOUR,                 904,    4                         ) \
      X(FIVE,                 905,    5                         ) \
      X(SIX,                  906,    6                         ) \
      X(UNDEFINED,            999,    0                         )

#define X(name, val, vpe) eVS_ ## name = val,

        enum value_space {
            PAX_VALUE_SPACE_DATA
        };

#undef X

#define X(name, val, vpe) eVPE_ ## name = vpe,

        enum vpe {
            PAX_VALUE_SPACE_DATA
        };

#undef X

        static inline vpe lookupVpe(enum value_space vs) {

#define X(name, val, vpe) case eVS_ ## name : return eVPE_ ## name;

            switch (vs)
            {
                PAX_VALUE_SPACE_DATA
            default: return eVPE_UNDEFINED;
            }

#undef X
        }

        vpe getVpe() { return lookupVpe(_vs); }

    protected:
        value_space _vs;

    } ValueSpace;  // class ValueSpaceStatic

    typedef enum ValueSpace::value_space  value_space_e;
    typedef enum ValueSpace::vpe          vpe_e;

    typedef struct pax_float3
    {
        float x, y, z;
        bool operator==(const pax_float3 & rt) { return (x == rt.x && y == rt.y && z == rt.z); }
    } pax_float3_t;


#define PAX_TYPE_DATA                                                         \
/*    PAX type Name              enum  bpv    vpe                        */   \
    X(INVALID,                    -1,   0,    ValueSpace::eVPE_UNDEFINED    ) \
    X(SF_MAG_UCHAR,                0,   1,    ValueSpace::eVPE_MAG          ) \
    X(SF_MAG_PHASE_USHORT,         1,   2,    ValueSpace::eVPE_MAGPHASE     ) \
    X(SF_COMPLEX_USHORT,           2,   2,    ValueSpace::eVPE_COMPLEX      ) \
    X(SF_COMPLEX_UINT,             3,   4,    ValueSpace::eVPE_COMPLEX      ) \
    X(SF_COMPLEX_ULONG,            4,   8,    ValueSpace::eVPE_COMPLEX      ) \
    X(SF_MAG_CHAR,                 5,   1,    ValueSpace::eVPE_MAG          ) \
    X(SF_MAG_PHASE_SHORT,          6,   2,    ValueSpace::eVPE_MAGPHASE     ) \
    X(SF_COMPLEX_SHORT,            7,   2,    ValueSpace::eVPE_COMPLEX      ) \
    X(SF_COMPLEX_INT,              8,   4,    ValueSpace::eVPE_COMPLEX      ) \
    X(SF_COMPLEX_LONG,             9,   8,    ValueSpace::eVPE_COMPLEX      ) \
    X(SF_COMPLEX_SINGLE,          10,   4,    ValueSpace::eVPE_COMPLEX      ) \
    X(SF_COMPLEX_DOUBLE,          11,   8,    ValueSpace::eVPE_COMPLEX      ) \
    X(SF_MAG_PHASE_UCHAR,         12,   1,    ValueSpace::eVPE_MAGPHASE     ) \
    X(SF_MAG_PHASE_CHAR,          13,   1,    ValueSpace::eVPE_MAGPHASE     ) \
    X(SF_RGB_UCHAR,               14,   1,    ValueSpace::eVPE_RGB          ) \
    X(SF_HSV_UCHAR,               15,   1,    ValueSpace::eVPE_HSV          ) \
    X(SF_UNDEFINED_PIXEL_TYPE,    16,   0,    0                             ) \
                                                                              \
    X(CUSTOM,                     99,   0,    ValueSpace::eVPE_UNDEFINED    ) \
    X(CHAR,                      100,   1,    ValueSpace::eVPE_ONE          ) \
    X(UCHAR,                     101,   1,    ValueSpace::eVPE_ONE          ) \
    X(SHORT,                     102,   2,    ValueSpace::eVPE_ONE          ) \
    X(USHORT,                    103,   2,    ValueSpace::eVPE_ONE          ) \
    X(INT,                       104,   4,    ValueSpace::eVPE_ONE          ) \
    X(UINT,                      105,   4,    ValueSpace::eVPE_ONE          ) \
    X(LONG,                      106,   8,    ValueSpace::eVPE_ONE          ) \
    X(ULONG,                     107,   8,    ValueSpace::eVPE_ONE          ) \
    X(HALF,                      108,   2,    ValueSpace::eVPE_ONE          ) \
    X(FLOAT,                     109,   4,    ValueSpace::eVPE_ONE          ) \
    X(DOUBLE,                    110,   8,    ValueSpace::eVPE_ONE          ) \
    X(QUADRUPLE,                 111,  16,    ValueSpace::eVPE_ONE          ) \
                                                                              \
    X(META_ONLY,                 199,   0,    ValueSpace::eVPE_UNDEFINED    ) \
    X(FLOAT3,                    200,   4,    ValueSpace::eVPE_REAL3        ) \
                                                                              \
/* NOTE: using PBM_BINARY would require changing bpv to bits per value */     \
    X(PBM_ASCII,                1001,   1,    ValueSpace::eVPE_BITS         ) \
    X(PGM_ASCII,                1002,   1,    ValueSpace::eVPE_REAL         ) \
    X(PPM_ASCII,                1003,   1,    ValueSpace::eVPE_RGB          ) \
    X(PBM_BINARY,               1004,   1,    ValueSpace::eVPE_BITS         ) \
    X(PGM_BINARY,               1005,   1,    ValueSpace::eVPE_REAL         ) \
    X(PPM_BINARY,               1006,   1,    ValueSpace::eVPE_RGB          ) \

#define X(name,val,bpv,vpe) ePAX_ ## name = val,
    enum class paxTypes : std::int32_t {
        PAX_TYPE_DATA
    };
#undef X

    typedef std::complex<float>           csingle;
    typedef std::complex<double>          cdouble;

    using   floatRasterFile = rasterFile<paxTypes::ePAX_FLOAT>;
    using   floatRasterFilePtr = rasterFilePtr<paxTypes::ePAX_FLOAT>;
    using   charRasterFile = rasterFile<paxTypes::ePAX_CHAR>;
    using   charRasterFilePtr = rasterFilePtr<paxTypes::ePAX_CHAR>;
    using   ucharRasterFile = rasterFile<paxTypes::ePAX_UCHAR>;
    using   ucharRasterFilePtr = rasterFilePtr<paxTypes::ePAX_UCHAR>;
    using   float3RasterFile = rasterFile<paxTypes::ePAX_FLOAT3>;
    using   float3RasterFilePtr = rasterFilePtr<paxTypes::ePAX_FLOAT3>;

  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // 
  // rasterFile: The templated class associated with raster data files.
  //
  // Sample PAX file follows
  /*

PAX101 : PAX_UCHAR
#
# <pax root>/test/data/gold/pax_sample.pax
# This is a sample PAX file containing metadata. The metadata type is given in square brackets. There is no default type.
#
# As PAX is a self-documenting format, use of blank comments is strongly suggested to improve readability.
#
# Metadata names never begin with whitespace and are unique. Repeated names cause earlier data to be overwritten and lost.
# Comments are simply a special type of metadata not having a user-defined name and not requiring the leading description.
#
BYTES_PER_VALUE : 1
VALUES_PER_ELEMENT : 1
#
#  This file is a simple 2x2 byte array
## [double]   total_elements = 4
#
ELEMENTS_IN_SEQUENTIAL_DIMENSION : 2
ELEMENTS_IN_STRIDED_DIMENSION : 2
#
#  You can use scientific notation, hex, etc.
#  For floating-point data, it lets the C++ strstream choose how many significant figures, up to 15.
#  Additional digits are ignored.
#  Hex data can be read but not written. Caution, hex values can get weird though, since they are stored as double. Don't use more than ~52 bits!
#
## [double]   pi              = 3.1415926535897932384
## [double]   alpha_centauri  = 4.12E16
## [double]   crc32_checksum  = 0xFB29C8B3
#
DATA_LENGTH : 4
cHaR
  */
  //


  //////////////////////////////////////////////////////////////////////////
  // rasterFileBase: A simple parent class to support rasterFile portability
  //
    class rasterFileBase
    {
      // CXXTest Classes
        friend class TestRead;
        friend class TestWrite;
        friend class NewTestRead;
        friend class NewTestWrite;
        friend class TestUtility;

        typedef std::shared_ptr<std::unordered_map<std::string, meta_t>>    paxMetaDataPtr;

    public:

      //typedef std::shared_ptr<std::vector<char>>                        paxDataBufPtr;

        rasterFileBase() : _dataType(paxTypes::ePAX_INVALID), _version(PAX_VERSION), _metaLoc(LOC_END) { _importedLength = 0; }
        rasterFileBase(paxTypes_e dataType) : _version(PAX_VERSION), _metaLoc(LOC_END) { _dataType = dataType; _importedLength = 0; }

        //////////////////////////////////////////////////////////////////////////
        //
        // Return the PAX type
        //
        paxTypes_e getType() {
            return _dataType;
        }


        //////////////////////////////////////////////////////////////////////////
        //
        // Attempt to validate the given type
        //
        static bool isPaxType(int32_t t) {
            paxTypes_e pax = static_cast<paxTypes_e> (t);

            if (pax == paxTypes_e::ePAX_INVALID) {
                PAX_LOG(2, << "found PAX type PAX_INVALID");
                return false;
            }

#define X(name,val,bpv,vpe) || (t == val)
            bool ret = false
                PAX_TYPE_DATA
                ;
#undef X

            PAX_LOG(2, << (ret ? "found valid PAX type: " : "found invalid PAX type: ") << t);

            return ret;
        }

        //////////////////////////////////////////////////////////////////////////
        //
        // check that the given string is a valid PAX tag
        //
        static bool validatePaxTag(char *& pos, paxTypes_e &type, float &version) {

          // temporarily terminate at the end of line
            char * eol = strchr(pos, '\n');
            TempNull tn(eol);
            size_t tagLen = strlen(PAX_TAG);
            paxTypes_e paxType = paxTypes::ePAX_INVALID;

            // 'valid' is used for conditional debug output
            bool valid = (0 == strncmp(pos, PAX_TAG, tagLen));
            PAX_LOG(2, << (valid ? "found PAX tag" : "ERROR! invalid PAX tag!"));
            if (!valid) {
                return false;
            }

            // get type
            pos += tagLen;
            char *oldPos = pos;
            long ntype = strtol(pos, &pos, 10);
            valid = (0L != ntype && LONG_MAX != ntype && LONG_MIN != ntype) || (0L == ntype && '0' == *oldPos);

            if (valid) {
                paxType = getPaxType(ntype);
                valid = paxTypes_e::ePAX_INVALID != paxType;
            }

            { // local scope for TempNull to facilitate printing the converted character(s)
                TempNull tn2(pos);
                PAX_LOG(2, << (valid ? "found valid PAX type: " : "ERROR! invalid PAX type: ") << oldPos);
            }

            if (paxTypes_e::ePAX_INVALID == paxType) {
                PAX_LOG_ERROR(1, << "invalid PAX type discovered");
                return false;
            }

            BufMan::skipDelimiter(pos, false);

            // read version if it is given (did not exist prior to library version 1.0)
            if (*pos == 'v' || *pos == 'V') {
                BufMan::skipWS(++pos);
                version = strtof(pos, &pos);
                // TODO: verify version
                BufMan::skipDelimiter(pos, false);
            }

            // TODO: verify that PAX type text matches the tag
            //buf.skipWS (buf.pos ());
            //assert (':' == buf[0]);
            //buf.pos ()++;
            //buf.skipWS (buf.pos ());

            type = paxType;
            tn.restore();
            BufMan::skipLine(pos);

            return valid;
        } // bool validatePaxTag (char * buf)


        //////////////////////////////////////////////////////////////////////////
        //
        // convert an int to PAX type if possible
        //
        static paxTypes_e getPaxType(int32_t t) {
            if (!isPaxType(t))  return paxTypes_e::ePAX_INVALID;
            paxTypes_e pax = static_cast<paxTypes_e> (t);
            return pax;
        } // paxTypes_e getPaxType (int32_t t)


        //////////////////////////////////////////////////////////////////////////
        //
        // Read the PAX type from the given filename
        //
        static paxTypes_e getPaxFileType(const std::string& path) {
            paxBufPtr bufPtr = readFileChunk(path);

            PAX_LOG(2, << "Identifying PAX type in file " << path);
            paxTypes_e paxType = getPaxFileType(bufPtr);

            return paxType;
        } // paxTypes_e getPaxFileType (const std::string& path)


          //////////////////////////////////////////////////////////////////////////
          //
          // Read the PAX type from the given filename
          //
        static paxTypes_e getPaxFileType(paxBufPtr bufPtr, float *version = NULL) {
            BufMan buf(bufPtr->data(), bufPtr->size());

            paxTypes_e paxType = paxTypes::ePAX_INVALID;
            float      _version = PaxStatic::defaultVersion();
            bool valid = validatePaxTag(buf.pos(), paxType, _version);

            if (version != NULL) {
                *version = _version;
            }

            PAX_LOG(2, << (valid ? "Found PAX type " : "Invalid PAX type: ") << getTypeName(paxType));

            return paxType;
        } // paxTypes_e getPaxFileType (const std::string& path)


        //////////////////////////////////////////////////////////////////////////
        //
        // extract values per element for given type
        //
        static int32_t getVPE(paxTypes_e e) {
          // sanity check on enum
            int type = (int)e;
            if (!isPaxType(type))  return 0;

            int32_t _vpe = 0;

#define X(name,val,bpv,vpe) case (paxTypes_e::ePAX_ ## name): _vpe = vpe; break;

            switch (e) {
                PAX_TYPE_DATA
            }

#undef X

            return _vpe;
        }


        //////////////////////////////////////////////////////////////////////////
        //
        // extract bytes per value for given type
        //
        static int32_t getBPV(paxTypes_e e) {
          // sanity check on enum
            int type = (int)e;
            if (!isPaxType(type))  return 0;

            int32_t _bpv = 0;

#define X(name,val,bpv,VPE) if (val == type) { PAX_LOG (2, << "bpv for pax type " << val << " is " << bpv); _bpv = bpv; }

            PAX_TYPE_DATA

#undef X        

                return _bpv;
        }


        //////////////////////////////////////////////////////////////////////////
        //
        // extract PAX type name for given type
        //
        static std::string getTypeName(paxTypes_e e) {
          // sanity check on enum
            int type = (int)e;
            if (!isPaxType(type))  e = paxTypes_e::ePAX_INVALID;

            std::string name;

#define XX(name)            #name
#define X(name,val,bpv,vpe) case (paxTypes_e::ePAX_ ## name): return XX(PAX_ ## name); \


            switch (e) {
                PAX_TYPE_DATA
            }

#undef X
#undef XX

      // it's impossible to get here but some compilers doesn't know that and it removes the warnings
            return std::string();
        }


        //////////////////////////////////////////////////////////////////////////
        //
        // Query length of sequential dimension
        //
        uint32_t getNumSequential()
        {
            return _numSequential;
        }


        //////////////////////////////////////////////////////////////////////////
        //
        // Query length of strided dimension
        //
        uint32_t getNumStrided()
        {
            return _numStrided;
        }


        //////////////////////////////////////////////////////////////////////////
        //
        // Query number of elements
        //
        uint32_t getNumElements()
        {
            return _numStrided * _numSequential;
        }


        //////////////////////////////////////////////////////////////////////////
        //
        // Query total number of values in all elements
        //
        uint32_t getNumValues()
        {
            return _numStrided * _numSequential * getVPE(_dataType);
        }


        //////////////////////////////////////////////////////////////////////////
        //
        // Query length of file that was imported
        //
        size_t importedLength()
        {
            return _importedLength;
        }


        //////////////////////////////////////////////////////////////////////////
        //
        // Direct metadata access
        //
        std::shared_ptr<std::unordered_map<std::string, meta_t>> & meta()
        {
            return _meta;
        }


        //////////////////////////////////////////////////////////////////////////
        //
        // Check for metadata
        //
        paxMetaDataTypes_e getMetaType(std::string key)
        {
            auto iter = _meta->find(key);
            bool hasit = _meta->end() != iter;
            if (hasit) {
                PAX_LOG(2, << "found " << key.c_str() << " of type " << (int)iter->second.type << " in metadata");
            } else {
                PAX_LOG_ERROR(1, << "could not find " << key.c_str() << " in metadata");
            }

            if (!hasit) {
                return paxMetaDataTypes_e::paxInvalid;
            }

            return iter->second.type;
        } // getMetaType(std::string key)


        //////////////////////////////////////////////////////////////////////////
        //
        // Extract float from metadata
        //
        float getMetaFloat(std::string key)
        {
            auto iter = _meta->find(key);
            float val = std::numeric_limits<float>::quiet_NaN();
            bool hasit = (_meta->end() != iter);
            if (!hasit) {
                PAX_LOG_ERROR(1, << "getting float metadata, could not find '" << key.c_str() << "'");
                PaxStatic::setStatus(PAX_FAIL);
                return val;
            }

            bool validType = iter->second.type != paxMetaDataTypes::paxInvalid;
            if (!validType) {
                PAX_LOG_ERROR(1, << "getting float metadata, invalid type found for '" << key.c_str() << "'");
                PaxStatic::setStatus(PAX_FAIL);
                return val;
            }

            // If the meta is an array the value will be nonsensical. Just let this happen.
            if (iter->second.isArray()) {
                PAX_LOG_ERROR(1, << "getting float metadata, accessing array data as scalar for '" << key.c_str() << "'");
                val = iter->second.fb[0];
            } else {
                val = iter->second.f;
            }

            PAX_LOG(2, << "getting metadata: '" << key.c_str() << "' = " << val);

            return val;
        }


        //////////////////////////////////////////////////////////////////////////
        //
        // Extract float from metadata (array)
        //
        float getMetaFloat(std::string key, std::vector<uint32_t> &indices)
        {
            auto iter = _meta->find(key);
            float val = std::numeric_limits<float>::quiet_NaN();
            bool hasit = (_meta->end() != iter);
            if (!hasit) {
                PAX_LOG_ERROR(1, << "getting float metadata, could not find '" << key.c_str() << "'");
                PaxStatic::setStatus(PAX_FAIL);
                return val;
            }

            bool validType = iter->second.type != paxMetaDataTypes::paxInvalid;
            if (!validType) {
                PAX_LOG_ERROR(1, << "getting float metadata, invalid type found for '" << key.c_str() << "'");
                PaxStatic::setStatus(PAX_FAIL);
                return val;
            }

            // If the meta is not an array the value will probably be nonsensical. Just let this happen.
            if (!(iter->second.num_dims == indices.size())) {
                PAX_LOG_ERROR(1, << "getting float metadata, accessing scalar data with indexes for '" << key.c_str() << "'");
                return val;
            }

            size_t index = iter->second.I(indices);
            if (!PaxStatic::paxNoError()) {
                return val;
            }
            val = iter->second.fb[index];
            PAX_LOG(2, << "getting metadata: '" << key.c_str() << "' = " << val);

            return val;

        }


        //////////////////////////////////////////////////////////////////////////
        //
        // Extract float from metadata (array)
        //
        float getMetaFloat(std::string key, std::initializer_list<uint32_t> list)
        {
            std::vector<uint32_t> indices(list);
            return getMetaFloat(key, indices);
        }


        //////////////////////////////////////////////////////////////////////////
        //
        // Extract double from metadata
        //
        double getMetaDouble(std::string key)
        {
            auto iter = _meta->find(key);
            double val = std::numeric_limits<double>::quiet_NaN();
            bool hasit = (_meta->end() != iter);
            if (!hasit) {
                PAX_LOG_ERROR(1, << "getting double metadata, could not find '" << key.c_str() << "'");
                PaxStatic::setStatus(PAX_FAIL);
                return val;
            }

            bool validType = iter->second.type != paxMetaDataTypes::paxInvalid;
            if (!validType) {
                PAX_LOG_ERROR(1, << "getting double metadata, invalid type found for '" << key.c_str() << "'");
                PaxStatic::setStatus(PAX_FAIL);
                return val;
            }

            // If the meta is an array the value will be nonsensical. Just let this happen.
            if (iter->second.isArray()) {
                PAX_LOG_ERROR(1, << "getting double metadata, accessing array data as scalar for '" << key.c_str() << "'");
            }

            val = iter->second.d;
            PAX_LOG(2, << "getting metadata: '" << key.c_str() << "' = " << val);

            return val;
        }


        //////////////////////////////////////////////////////////////////////////
        //
        // Extract double from metadata (array)
        //
        double getMetaDouble(std::string key, std::vector<uint32_t> &indices)
        {
            auto iter = _meta->find(key);
            double val = std::numeric_limits<double>::quiet_NaN();
            bool hasit = (_meta->end() != iter);
            if (!hasit) {
                PAX_LOG_ERROR(1, << "getting double metadata, could not find '" << key.c_str() << "'");
                return val;
            }

            bool validType = iter->second.type != paxMetaDataTypes::paxInvalid;
            if (!validType) {
                PAX_LOG_ERROR(1, << "getting double metadata, invalid type found for '" << key.c_str() << "'");
                return val;
            }

            // If the meta is not an array the value will probably be nonsensical.
            if (!(iter->second.num_dims == indices.size())) {
                PAX_LOG_ERROR(1, << "getting double metadata, accessing scalar data with indexes for '" << key.c_str() << "'");
                return val;
            }

            size_t index = iter->second.I(indices);
            if (!PaxStatic::paxNoError()) {
                return val;
            }
            val = iter->second.db[index];
            PAX_LOG(2, << "getting metadata: '" << key.c_str() << "' = " << val);

            return val;
        }


        //////////////////////////////////////////////////////////////////////////
        //
        // Extract float from metadata (array)
        //
        double getMetaDouble(std::string key, std::initializer_list<uint32_t> list)
        {
            std::vector<uint32_t> indices(list);
            return getMetaDouble(key, indices);
        }


        //////////////////////////////////////////////////////////////////////////
        //
        // Extract integer types from metadata
        //
        template <typename T>
        T getMetaInteger(std::string key) {

            T errorcode = std::numeric_limits<T>::max();
            auto iter = _meta->find(key);
            bool hasit = (_meta->end() != iter);
            if (!hasit) {
                PAX_LOG_ERROR(1, << "getting integer metadata, could not find '" << key.c_str() << "'");
                return errorcode;
            }

            bool validType = iter->second.type != paxMetaDataTypes::paxInvalid;
            if (!validType) {
                PAX_LOG_ERROR(1, << "getting integer metadata, invalid type found for '" << key.c_str() << "'");
                return errorcode;
            }

            // If the meta is an array the value will probably be nonsensical.
            T val = errorcode;
            if (iter->second.isArray()) {
                PAX_LOG_ERROR(1, << "getting integer metadata, accessing array data as scalar for '" << key.c_str() << "'");
                val = static_cast<T>(iter->second.u64b[0]); // static_cast doesn't care about signed-ness. TODO: endian problems though.

            } else {

                val = static_cast<T>(iter->second.u64); // static_cast doesn't care about signed-ness. TODO: endian problems though.

            }

            PAX_LOG(2, << "getting metadata: '" << key.c_str() << "' = " << +val);

            return val;

        }


        //////////////////////////////////////////////////////////////////////////
        //
        // Extract integer types from metadata (array)
        //
        template <typename T>
        T getMetaInteger(std::string key, std::vector<uint32_t> &indices) {

            T errorcode = std::numeric_limits<T>::max();
            auto iter = _meta->find(key);
            bool hasit = (_meta->end() != iter);
            if (!hasit) {
                PAX_LOG_ERROR(1, << "getting integer metadata, could not find '" << key.c_str() << "'");
                PaxStatic::setStatus(PAX_FAIL);
                return errorcode;
            }

            bool validType = iter->second.type != paxMetaDataTypes::paxInvalid;
            if (!validType) {
                PAX_LOG_ERROR(1, << "getting integer metadata, invalid type found for '" << key.c_str() << "'");
                PaxStatic::setStatus(PAX_FAIL);
                return errorcode;
            }

            // If the meta is not an array the value will probably be nonsensical.
            if (!(iter->second.num_dims == indices.size())) {
                PAX_LOG_ERROR(1, << "getting integer metadata, accessing array data as scalar for '" << key.c_str() << "'");
                return errorcode;
            }

            T *bufPtr = static_cast<T*>(iter->second.bufPtr());
            size_t index = iter->second.I(indices);
            if (!PaxStatic::paxNoError()) {
                return errorcode;
            }
            T val = bufPtr[index];
            PAX_LOG(2, << "getting metadata: '" << key.c_str() << "' = " << +val);

            return val;

        }


        //////////////////////////////////////////////////////////////////////////
        //
        // Extract integer types from metadata (array)
        //
        template <typename T>
        T getMetaInteger(std::string key, std::initializer_list<uint32_t> list) {
            std::vector<uint32_t> indices(list);
            return getMetaInteger<T>(key, indices);
        }


        //////////////////////////////////////////////////////////////////////////
        //
        // Extract specific scalar types from metadata
        //
        int64_t  getMetaInt64(std::string key) { return getMetaInteger<int64_t>(key); }
        uint64_t getMetaUint64(std::string key) { return getMetaInteger<uint64_t>(key); }
        int32_t  getMetaInt32(std::string key) { return getMetaInteger<int32_t>(key); }
        uint32_t getMetaUint32(std::string key) { return getMetaInteger<uint32_t>(key); }
        int16_t  getMetaInt16(std::string key) { return getMetaInteger<int16_t>(key); }
        uint16_t getMetaUint16(std::string key) { return getMetaInteger<uint16_t>(key); }
        int8_t  getMetaInt8(std::string key) { return getMetaInteger<int8_t>(key); }
        uint8_t getMetaUint8(std::string key) { return getMetaInteger<uint8_t>(key); }

        // array index via vector
        int64_t  getMetaInt64(std::string key, std::vector<uint32_t> indices) { return getMetaInteger<int64_t>(key, indices); }
        uint64_t getMetaUint64(std::string key, std::vector<uint32_t> indices) { return getMetaInteger<uint64_t>(key, indices); }
        int32_t  getMetaInt32(std::string key, std::vector<uint32_t> indices) { return getMetaInteger<int32_t>(key, indices); }
        uint32_t getMetaUint32(std::string key, std::vector<uint32_t> indices) { return getMetaInteger<uint32_t>(key, indices); }
        int16_t  getMetaInt16(std::string key, std::vector<uint32_t> indices) { return getMetaInteger<int16_t>(key, indices); }
        uint16_t getMetaUint16(std::string key, std::vector<uint32_t> indices) { return getMetaInteger<uint16_t>(key, indices); }
        int8_t  getMetaInt8(std::string key, std::vector<uint32_t> indices) { return getMetaInteger<int8_t>(key, indices); }
        uint8_t getMetaUint8(std::string key, std::vector<uint32_t> indices) { return getMetaInteger<uint8_t>(key, indices); }

        //// array index via initializer list
        //int64_t  getMetaInt64 (std::string key, std::initializer_list<uint32_t> list)    { return getMetaInteger<int64_t> (key, list); }
        //uint64_t getMetaUint64 (std::string key, std::initializer_list<uint32_t> list)   { return getMetaInteger<uint64_t> (key, list); }
        //int64_t  getMetaInt32 (std::string key, std::initializer_list<uint32_t> list)    { return getMetaInteger<int32_t> (key, list); }
        //uint64_t getMetaUint32 (std::string key, std::initializer_list<uint32_t> list)   { return getMetaInteger<uint32_t> (key, list); }
        //int64_t  getMetaInt16 (std::string key, std::initializer_list<uint32_t> list)    { return getMetaInteger<int16_t> (key, list); }
        //uint64_t getMetaUint16 (std::string key, std::initializer_list<uint32_t> list)   { return getMetaInteger<uint16_t> (key, list); }
        //int64_t  getMetaInt8 (std::string key, std::initializer_list<uint32_t> list)     { return getMetaInteger<int8_t> (key, list); }
        //uint64_t getMetaUint8 (std::string key, std::initializer_list<uint32_t> list)    { return getMetaInteger<uint8_t> (key, list); }


        //////////////////////////////////////////////////////////////////////////
        //
        // Extract string from metadata
        //
        std::string getMetaString(std::string key)
        {
            auto iter = _meta->find(key);
            bool hasit = _meta->end() != iter;
            if (!hasit) {
                PAX_LOG_ERROR(1, << "getting metadata, could not find '" << key.c_str() << "'");
                return "";
            }

            std::string str = iter->second.s;
            PAX_LOG(2, << "getting metadata: '" << key << "' = " << str);

            return str;
        }


        //////////////////////////////////////////////////////////////////////////
        //
        // Convert the data to float
        //
        static std::shared_ptr<float> getFloatData(rasterFileBase *paxIn)
        {
          // not implements
            PAX_LOG_ERROR(1, << "getFloatData() has not been implemented yet");
            return nullptr;
      /*      std::shared_ptr<float> floatData(new float(paxIn->getNumElements()));
      //      float *data = floatData.get();
      //
      //      switch(paxIn->getType()) {
      //#define X(name, val, vpe)                                     \
      //        case ePAX_ ## name:                                   \
      //          { rasterFile<ePAX_ ## name> *pax = (rasterFile<ePAX_ ## name> *)                                                 \
      //        for (int i = 0; i < paxIn->getNumElements(); ++i) {
      //          data[i] = (float)
      //        }
      //      }
      //
      //      PAX_VALUE_SPACE_DATA
      //#undef X    */
        }


        //////////////////////////////////////////////////////////////////////////
        //
        // Checks for existence of the given directory
        //
        static bool isDirExist(const std::string& path)
        {
            struct pax_stat info;
            if (pax_stat(path.c_str(), &info) != 0)
            {
                return false;
            }
            return (info.st_mode & pax_ifdir) != 0;
        }


        //////////////////////////////////////////////////////////////////////////
        //
        // makes the full given path
        //
        static bool makePath(const std::string& path)
        {
#ifndef _WIN32
            mode_t mode = 0755;
#endif
            int ret = pax_mkdir(path.c_str(), mode);
            if (ret == 0)
                return true;

            switch (errno)
            {
            case ENOENT:
              // parent didn't exist, try to create it
            {
                size_t pos = path.find_last_of('/');
                if (pos == std::string::npos)
#if defined(_WIN32)
                    pos = path.find_last_of('\\');
                if (pos == std::string::npos)
#endif
                    return false;
                if (!makePath(path.substr(0, pos)))
                    return false;
            }
            // now, try to create again
            return 0 == pax_mkdir(path.c_str(), mode);

            case EEXIST:
              // done!
                return isDirExist(path);

            default:
                return false;
            }
        }


        //////////////////////////////////////////////////////////////////////////
        //
        // output given PAX buffer to file
        //
        static int writeToFile(std::ostringstream & oss, pax_filestring fileName) {
            // TODO: PAX improve writeToFile(ostringstream), needs 2 copies
            std::string str = oss.str();
            paxBufPtr buf = std::make_shared<paxBuf_t>(str.length());
            memcpy(buf->data(), str.c_str(), str.length());
            return writeToFile(buf, fileName);
        }


        //////////////////////////////////////////////////////////////////////////
        //
        // output given PAX buffer to file
        //
        static int writeToFile(paxBufPtr &buf, pax_filestring fileName) {
            PAX_LOG(1, << "Writing PAX buffer " << "of size " << buf->size() << " to " << fileName);

            pax_remove(fileName.c_str());
            int fd = pax_open(fileName.c_str(), O_BINARY | O_CREAT | O_WRONLY, 0660);
            if (-1 == fd) {
                PAX_LOG_ERRNO(1, << "Error " << errno << " opening output file.");
                return PAX_FAIL;
            }

            int ret = (int)pax_write(fd, buf->data(), (unsigned int)buf->size());
            pax_close(fd);

            if (ret == (int)buf->size()) {
                PAX_LOG(1, << "Successfully wrote " << buf->size() << " bytes.");
            } else {
                PAX_LOG(1, << "Failure. Write returned " << ret << " but expected " << buf->size() << ".");
            }

            return PAX_OK;
        }


        //////////////////////////////////////////////////////////////////////////
        //
        // open the given file and read its contents on a chunk-by-chunk basis
        //
        // note that CHUNK_LEN is tied to size of file read in test_previewmeta_only_multichunk
        //
#define CHUNK_LEN 16384
        static paxBufPtr readFileChunk(pax_filestring fileName, int nChunk = 0) {

            PAX_LOG(2, << "Reading chunk " << nChunk << " from " << fileName);

            // open the file
            int fd = pax_open(fileName.c_str(), O_BINARY | O_RDONLY, 0660);
            if (-1 == fd) {
                PAX_LOG_ERRNO(1, << " opening input file.");
                return nullptr;
            }

            PAX_LOG(2, << "successfully opened file");

            // get file size, create buffer
            long fileLength = pax_lseek(fd, 0, SEEK_END);
            if (-1L == fileLength) {
                PAX_LOG_ERRNO(1, << " getting size of input file.");
                return nullptr;
            }

            PAX_LOG(2, << "file length is " << fileLength);

            long start = nChunk * CHUNK_LEN;

            // trivial case: start is past EOF
            if (start > fileLength) {
                PAX_LOG_WARN(2, << "start of chunk " << nChunk << " is beyond length of file. Returning empty buffer.");
                return std::make_shared<paxBuf_t>(0);
            }

            long length = CHUNK_LEN;

            // check for partial chunk
            if (start + CHUNK_LEN > fileLength) {
                length = fileLength - start;
                PAX_LOG_WARN(2, << "End of chunk " << nChunk << " is beyond length of file. Returning partial chunk of length " << length);
            }

            // allocate buffer for input file
            paxBufPtr inBuf = std::make_shared<paxBuf_t>(length);
            if (!inBuf) {
                PAX_LOG_ERROR(1, " allocating input buffer. Returning null.");
                return nullptr;
            }

            // read the file into buffer
            pax_lseek(fd, nChunk * CHUNK_LEN, SEEK_SET);
            char *buf = inBuf->data();
            int readRet = pax_read(fd, buf, length);

            // close the file
            pax_close(fd);
            if (readRet != length) {
                PAX_LOG_ERROR(1, << " reading input file.");
                return nullptr;
            }

            PAX_LOG(2, << "readFile done");

            return inBuf;

        } // paxBufPtr readFile (pax_filestring fileName)


        //////////////////////////////////////////////////////////////////////////
        //
        // open the given file and read its contents
        //
        static paxBufPtr readFile(pax_filestring fileName) {

            PAX_LOG(1, << "Reading " << fileName);

            // open the file
            int fd = pax_open(fileName.c_str(), O_BINARY | O_RDONLY, 0660);
            if (-1 == fd) {
                PAX_LOG_ERRNO(1, << " opening input file.");
                return nullptr;
            }

            PAX_LOG(2, << "successfully opened file");

            // get file size, create buffer
            long length = pax_lseek(fd, 0, SEEK_END);
            if (-1L == length) {
                PAX_LOG_ERRNO(1, << " getting size of input file.");
                return nullptr;
            }

            PAX_LOG(2, << "file length is " << length);

            // allocate buffer for input file
            paxBufPtr inBuf = std::make_shared<paxBuf_t>(length);
            if (!inBuf) {
                PAX_LOG_ERROR(1, " allocating input buffer.");
                return nullptr;
            }

            // read the file into buffer
            pax_lseek(fd, 0, SEEK_SET);
            char *buf = inBuf->data();
            int readRet = pax_read(fd, buf, length);

            // close the file
            pax_close(fd);
            if (readRet != length) {
                PAX_LOG_ERROR(1, << " reading input file.");
                return nullptr;
            }

            PAX_LOG(2, << "readFile done");

            return inBuf;

        } // paxBufPtr readFile (pax_filestring fileName)


        //////////////////////////////////////////////////////////////////////////
        //
        // writeToBuffer: writes PAX to a buffer (base class implementation writes header only)
        // 
        virtual int writeToBuffer(paxBufPtr &outBuf) {
          // TODO: rasterFileBase::writeToBuffer
            return 0;
        }


        //////////////////////////////////////////////////////////////////////////
        //
        // importHeader: import an PAX header from a buffer
        // 
        int importHeader(BufMan & buf, int32_t &dataLen, bool fastImport = false) {
          // DEVCODE: verbosityLevel in importHeader
#define verbosityLevel 2

      // sanity check
            paxTypes_e  paxType = paxTypes_e::ePAX_INVALID;
            float       version = 0.0f;
            if (!validatePaxTag(buf.pos(), paxType, version)) {
                PAX_LOG_ERROR(1, << "not a valid PAX file");
                return PAX_FAIL;
            }

            _dataType = paxType;
            _version = version;

            PAX_LOG(2, << "validatePaxTag done");
            //if (buf.skipLine ()) { reportEOF (); return -1; }
            PAX_LOG(2, << "begin parsing header lines");

            int32_t bpv = 0, vpe = 0, dim1count = 0, dim2count = 0, datalencount = 0;

            getMetaRef();

            hlType_t type = HEADERLINETYPE::NOT_CHECKED;

            // Begin parsing header lines. Check for EOF each line.
            while (!buf.eof()) {

                bool headerDone = false;
                bool nextLine = false;
                std::pair <std::string, meta_t> meta1;

                if (HEADERLINETYPE::NOT_CHECKED == type) {
                    type = buf.getHeaderLineType(); // returns the line type but no further checking
                }

                switch (type) {
                case hlType_t::UNKNOWN:
                default:
                    PAX_LOG(verbosityLevel, << "Skipping line of type " << (int)type);
                    nextLine = true;
                    break;

                case hlType_t::PAX:
                    PAX_LOG(verbosityLevel, << "Skipping line of type " << (int)type);
                    buf.setLoc(metaLoc::LOC_AFTER_TAG, _metaLocCount[LOC_AFTER_TAG]);
                    nextLine = true;
                    break;

                case hlType_t::BPV:
                    bpv = buf.getUint32(skipFlags::SKIP_DELIMIER_AND_LINEFEED);
                    PAX_LOG(verbosityLevel, << "Read BPV = " << bpv);
                    buf.setLoc(metaLoc::LOC_AFTER_TAG, /*LOC_AFTER_BPV, */_metaLocCount[LOC_AFTER_TAG/*LOC_AFTER_BPV*/]);    // TEMPCODE: single meta location
                    break;

                case hlType_t::VPE:
                    vpe = buf.getUint32(skipFlags::SKIP_DELIMIER_AND_LINEFEED);
                    PAX_LOG(verbosityLevel, << "Read VPE = " << vpe);
                    buf.setLoc(metaLoc::LOC_AFTER_TAG, /*LOC_AFTER_VPE, */_metaLocCount[LOC_AFTER_TAG/*LOC_AFTER_VPE*/]);    // TEMPCODE: single meta location
                    break;

                case hlType_t::DIM: {
                    switch (buf.getDimTagIndex()) {
                    case 0:
                        _numSequential = buf.getUint32(skipFlags::SKIP_DELIMIER_AND_LINEFEED);
                        PAX_LOG(verbosityLevel, << "Read DIM1 = " << _numSequential);
                        buf.setLoc(metaLoc::LOC_AFTER_TAG, /*LOC_AFTER_SEQ, */_metaLocCount[LOC_AFTER_TAG/*LOC_AFTER_SEQ*/]);    // TEMPCODE: single meta location
                        ++dim1count;
                        break;
                    default:
                        _numStrided = buf.getUint32(skipFlags::SKIP_DELIMIER_AND_LINEFEED);
                        PAX_LOG(verbosityLevel, << "Read DIM2 = " << _numStrided);
                        buf.setLoc(metaLoc::LOC_AFTER_TAG, /*LOC_AFTER_STR1, */_metaLocCount[LOC_AFTER_TAG/*LOC_AFTER_STR1*/]);    // TEMPCODE: single meta location
                        ++dim2count;
                        break;
                    }
                    break;
                }

                case hlType_t::DATALEN:
                    dataLen = buf.getUint32(skipFlags::SKIP_DELIMIER_AND_LINEFEED);
                    PAX_LOG(verbosityLevel, << "Read DATALEN = " << dataLen);
                    ++datalencount;
                    headerDone = true;
                    break;

                case hlType_t::COMMENT:
                    if (fastImport) {
                        nextLine = true;
                    } else {
                        meta1 = buf.getMeta();
                        PAX_LOG(verbosityLevel, << "Read comment: " << meta1.second.s);
                        getMetaRef()[meta1.first] = meta1.second;
                    }
                    break;

                case hlType_t::METADATA:
                    if (fastImport) {
                        nextLine = true;
                    } else {
                        meta1 = buf.getMeta();
                        PAX_LOG(verbosityLevel, << "Read METADATA of type " << (int)meta1.second.type << " = " << meta1.first << " = " << meta1.second.value().c_str());
                        getMetaRef()[meta1.first] = meta1.second;
                    }
                    break;
                }

                // if finished, we may need to skip one last line
                if (headerDone) {

                    if (nextLine) {
                        buf.skipLine();
                    }

                    break;
                }

                if (nextLine) {

                    PAX_LOG(verbosityLevel, << "Skipping line in header loop");

                    buf.skipLine();
                    uint32_t skipped = 1;
                    // the "line" may be multiple lines, e.g. a meta array
                    while ((type = buf.getHeaderLineType()) == HEADERLINETYPE::UNKNOWN) {
                        if (buf.eof()) break;
                        buf.skipLine();
                        ++skipped;
                    }

                    PAX_LOG(verbosityLevel, << "Skipped " << skipped << " line(s) in header loop");

                } else {
                    type = type = HEADERLINETYPE::NOT_CHECKED;
                }

            } //         while (!buf.eof ()) {

            // validate required tags
            if (dim1count != 1 || dim2count != 1 || datalencount != 1) {
                PAX_LOG_ERROR(1, << "Incorrect PAX tags: dim1count=" << dim1count << ", dim2count=" << dim2count << ", datalencount=" << datalencount << ". This may expected if previewing a long header.");
                return PAX_INVALID;
            }

            // validate parameters
            int32_t _bpv = getBPV(this->_dataType);
            if (_bpv != bpv) {
                PAX_LOG_ERROR(1, << "Incorrect bpv. Read " << bpv << " but expected " << _bpv);
                return PAX_INVALID;
            }

            int32_t _vpe = getVPE(this->_dataType);
            if (_vpe != vpe) {
                PAX_LOG_ERROR(1, << "Incorrect vpe. Read " << vpe << " but expected " << _vpe);
                return PAX_INVALID;
            }

            int32_t elemLen = bpv * vpe;
            int32_t myDataLen = elemLen * _numSequential * _numStrided;
            if (dataLen != myDataLen) {
                PAX_LOG_ERROR(1, << "datalength in file incorrect! Calculated: " << myDataLen << ", read from file: " << dataLen);
                return PAX_INVALID;
            }

            _numValues = _numSequential * _numStrided;

            return PAX_OK;
        }

        //////////////////////////////////////////////////////////////////////////
        //
        // baseImport: import an PAX from a buffer and return the base class
        // 
        template <paxTypes_e E>
        static std::shared_ptr<rasterFileBase> baseImport(char *buf, size_t len) {
        // TODO: baseImport watchout, may cause slicing
            rasterFile<E> *paxFile = new rasterFile<E>();
            paxFile->import(buf, len);
            std::shared_ptr<rasterFileBase> baseFile(paxFile);

            return baseFile;
        }


        //////////////////////////////////////////////////////////////////////////
        //
        // read header to preview PAX file from file
        // TODO: preview, improve. For long headers, parsing multiple chunks may be slow. Should leave file open.
        //
        int preview(pax_filestring fileName) {

            PAX_LOG(1, << "Previewing PAX file " << fileName);

            int nChunk = 0;
            paxBufPtr fileBuf(new paxBuf_t(0));  // initial header buffer

            while (true) {
                int ret = PAX_OK;

                paxBufPtr chunk = readFileChunk(fileName, nChunk);
                if (!chunk) {
                  // error has already been reported
                    return PAX_FAIL;
                }

                // add chunk to buffer
                fileBuf->appendVector(*chunk);

                //// cache this chunk
                //memcpy (fileBuf->data () + (nChunk * CHUNK_LEN), chunk->data(), chunk->size());

                // if buffer is full, make sure buffer ends in \n
                if (chunk->size() == CHUNK_LEN) {
                    size_t eolpos = (nChunk + 1) * CHUNK_LEN - 1;
                    char * buf = fileBuf->data();
                    while (eolpos != 0 && buf[eolpos] != '\n') --eolpos;
                    //Swapper<'\n'> end(fileBuf->data () + eolpos);
                    ret = preview(fileBuf);
                } else {
                    ret = preview(fileBuf);
                }

                if (PAX_FAIL == ret) {
                    PaxStatic::setStatus(PAX_FAIL);
                    return PAX_FAIL;
                }

                if (PAX_OK == ret) break; // success!

                // try again with more data
                ++nChunk;
            }

            return PAX_OK;
        }


        //////////////////////////////////////////////////////////////////////////
        //
        // Read (partial) header to preview PAX file from buffer
        // Return PAX_OK upon success, PAX_FAIL upon failure, or the length that
        // was successfully imported if not all the header was imported.
        //
        // TODO: PAX fast preview (don't process metadata)
        //
        int preview(paxBufPtr inBuf) {

            PAX_LOG(1, << "Previewing PAX file in buffer of length " << inBuf->size());

            BufMan buf(inBuf->data(), inBuf->size());

            // make sure buf ends at the end of a line
            uint64_t eolpos = inBuf->size() - 1;
            while (eolpos != 0 && buf[eolpos] != '\n') --eolpos;
            buf.truncate(eolpos);

            int32_t datalen = 0;
            int ret = importHeader(buf, datalen, true);

            if (ret == PAX_INVALID) {
                return (int)buf.offset();
            }

            return ret;
        }


        //////////////////////////////////////////////////////////////////////////
        //
        // importMultiple: extract multiple PAX files from a single buffer.
        // 
        // This is to facilitate DDS and simplify the topics that contain a variable number of rasters.
        // Note that this is NOT a valid operation for files (ofc we may change that some day).
        // Note that all types must be known apriori!
        //
        static std::vector<std::shared_ptr<rasterFileBase>> importMultiple(size_t bufCount, paxBufPtr bufPtr)
        {
            std::vector<std::shared_ptr<rasterFileBase>> bufVec(bufCount);
            char *buf = bufPtr.get()->data();
            size_t bufLeft = bufPtr->size();

            for (int bufnum = 0; bufnum < bufCount; ++bufnum) {
              // TODO: import using inheritance
            }

            return bufVec;
        }

        static std::vector<std::shared_ptr<rasterFileBase>> importMultiple(std::vector<paxTypes_e> types, paxBufPtr bufPtr)
        {
            size_t bufCount = types.size();
            std::vector<std::shared_ptr<rasterFileBase>> bufVec(bufCount);
            char *buf = bufPtr.get()->data();
            size_t bufLeft = bufPtr->size();

            for (int bufnum = 0; bufnum < bufCount; ++bufnum) {
                std::shared_ptr<rasterFileBase> baseFile;

                switch (types[bufnum]) {

#define X(name,val,bpv,vpe) case paxTypes::ePAX_ ## name : baseFile = baseImport<paxTypes::ePAX_ ## name> (buf, bufLeft); break;
                    PAX_TYPE_DATA
#undef X

                } // switch (types[bufnum])

                bufVec[bufnum] = baseFile;
                buf += baseFile->importedLength();
                bufLeft -= baseFile->importedLength();
            }

            return bufVec;
        }


        //////////////////////////////////////////////////////////////////////////
        //
        // writeMultiple: writes multiple PAX files to a single buffer.
        // 
        // This is to facilitate DDS and simplify the topics that contain a variable number of rasters.
        // Note that this is NOT a valid operation for files (ofc we may change that some day).
        // Note that all types must be known apriori!
        //    
        static paxBufPtr writeMultiple(std::vector<rasterFileBase*> paxVec) {
            size_t bufCount = paxVec.size();
            std::vector<paxBufPtr> bufVec;
            size_t bufSize = 0;

            PAX_LOG(1, << "Writing " << bufCount << " pax files to a buffer.");

            for (auto paxBuf : paxVec) {
                paxBufPtr bufPtr = nullptr;
                paxBuf->writeToBuffer(bufPtr);
                bufVec.push_back(bufPtr);
                bufSize += bufPtr->size();
            }

            paxBufPtr bufPtr = std::make_shared<paxBuf_t>(bufSize);
            size_t bufPos = 0;
            for (auto buf : bufVec) {
                memcpy(bufPtr->data() + bufPos, buf->data(), buf->size());
                bufPos += buf->size();
            }

            PAX_LOG(1, << "Wrote a total of " << bufPos << " bytes to the buffer.");

            return bufPtr;
        }

        static paxBufPtr writeMultiple(std::vector<std::shared_ptr<rasterFileBase>> paxVec)
        {
            std::vector<rasterFileBase *> paxVecp;

            for (auto ppax : paxVec) {
                paxVecp.push_back(ppax.get());
            }

            paxBufPtr bufPtr = writeMultiple(paxVecp);

            return bufPtr;
        } // static std::vector<std::shared_ptr<rasterFileBase>> importMultiple


        //////////////////////////////////////////////////////////////////////////
        //
        // helper function to make sure meta exists
        //
        std::unordered_map<std::string, pax::meta_t> & getMetaRef() {
            if (nullptr == _meta) _meta = std::shared_ptr <std::unordered_map<std::string, pax::meta_t>>(new std::unordered_map<std::string, pax::meta_t>());
            return *_meta;
        }


        //////////////////////////////////////////////////////////////////////////
        //
        // helper function to organize and sort metadata
        //
        std::shared_ptr<std::vector<std::vector<std::pair<std::string, meta_t>>>>
            getMetaVecs() {

            std::shared_ptr<std::vector<std::vector<std::pair<std::string, meta_t>>>> metavecs(new std::vector<std::vector<std::pair<std::string, meta_t>>>(LOC_COUNT));
            if (nullptr == _meta) return metavecs;

            for (auto meta : getMetaRef()) {
                size_t loc = meta.second.loc;
                auto &vec = metavecs->at(loc);
                vec.push_back(meta);
            }

            for (int i = 0; i < LOC_COUNT; ++i) {
                std::vector<std::pair<std::string, meta_t>> &vec = metavecs->at(i);
                std::sort(vec.begin(), vec.end(), [](const std::pair<std::string, meta_t>& lhs, const std::pair<std::string, meta_t>& rhs) { return lhs.second.index < rhs.second.index; });
                PAX_LOG(3, << "done sorting metaLoc " << i);
            }

            return metavecs;
        }

        static void copyMeta(rasterFileBase & dest, rasterFileBase & src) {
          // allocate a new destination meta map to release the old one
            dest._meta = std::shared_ptr <std::unordered_map<std::string, pax::meta_t>>(new std::unordered_map<std::string, pax::meta_t>());
            PAX_LOG(2, << "copying " << src._meta->size() << " meta elements.");
            for (auto meta : *(src._meta)) {
                dest._meta->insert(meta);
            }
            PAX_LOG(2, << "Done copying meta. " << dest._meta->size() << " meta elements were copied.");
        }

    protected:
        paxTypes_e          _dataType;
        float               _version;
        size_t              _importedLength;
        uint32_t            _numValues;
        uint32_t            _numSequential;
        uint32_t            _numStrided;
        paxMetaDataPtr      _meta;
        metaLoc_e      _metaLoc;
        size_t              _metaLocCount[metaLoc_e::LOC_COUNT];
    };  //   class rasterFileBase


    template <paxTypes_e E>
    class rasterFile : public rasterFileBase
    {
      // CXXTest Classes
        friend class TestRead;
        friend class TestWrite;
        friend class NewTestRead;
        friend class NewTestWrite;

        //////////////////////////////////////////////////////////////////////////
        //
        // ctors/dtors
        //    
    public:

        rasterFile() : rasterFileBase(E) { reset(); }
        rasterFile(int32_t sequential, void *buf) : rasterFileBase(E) { init((uint32_t)sequential, 1, buf); }
        rasterFile(uint32_t sequential, void *buf) : rasterFileBase(E) { init((uint32_t)sequential, 1, buf); }
        rasterFile(uint64_t sequential, void *buf) : rasterFileBase(E) { init((uint32_t)sequential, 1, buf); }
        rasterFile(int32_t sequential, int32_t strided = 1) : rasterFileBase(E) { init((uint32_t)sequential, (uint32_t)strided); }
        rasterFile(uint32_t sequential, uint32_t strided = 1) : rasterFileBase(E) { init(sequential, strided); }
        rasterFile(uint64_t sequential, uint32_t strided = 1) : rasterFileBase(E) { init(sequential, strided); }
        rasterFile(int32_t sequential, int32_t strided, void *buf) : rasterFileBase(E) { init((uint32_t)sequential, (uint32_t)strided, buf); }
        rasterFile(uint32_t sequential, uint32_t strided, void *buf) : rasterFileBase(E) { init(sequential, strided, buf); }
        rasterFile(uint64_t sequential, uint32_t strided, void *buf) : rasterFileBase(E) { init(sequential, strided, buf); }

        //////////////////////////////////////////////////////////////////////////
        //
        // public functions
        //
    public:


        //////////////////////////////////////////////////////////////////////////
        //
        // Reset the PAX object
        //
        void reset() {
            _numValues = 0;
            _numSequential = 0;
            _numStrided = 0;
            _buf = nullptr;
            _meta = nullptr;
            _metaLoc = LOC_END;
            memset(_metaLocCount, 0, metaLoc_e::LOC_COUNT * sizeof(size_t));
        }


        //////////////////////////////////////////////////////////////////////////
        //
        // 2D/1D Initializer
        //
        int init(uint32_t sequential, uint32_t strided = 1, void * buf = NULL) {

            int32_t bpv = getBPV(E);
            int32_t vpe = getVPE(E);

            if (/*0 == sequential || 0 == strided ||*/ 0 == bpv || 0 == vpe) {

                std::runtime_error e("PAX init: invalid dimension or PAX type");
                throw (e);

            }

            _numValues = sequential * strided;
            _numSequential = sequential;
            _numStrided = strided;

            if (0 == _numSequential || 0 == _numStrided) {

                _numSequential = _numStrided = 0;
                _buf = nullptr;

            }

            if (_numValues > 0) {

                _buf = std::make_shared<paxBuf_t>(_numValues * bpv * vpe);

                if (buf != NULL) {
                    size_t bytes = getBPV(E) * getVPE(E) * _numValues;
                    memcpy(_buf->data(), buf, bytes);
                }

            }

            _meta = nullptr;
            _metaLoc = LOC_END;
            memset(_metaLocCount, 0, metaLoc_e::LOC_COUNT * sizeof(size_t));

            return PAX_OK;

        } // int init (uint32_t sequential, uint32_t strided = 1, void * buf = NULL) 


        //////////////////////////////////////////////////////////////////////////
        //
        // 1D initializer
        //
        int init(uint32_t sequential, void * buf) {
            return init(sequential, 1, buf);
        }


        //////////////////////////////////////////////////////////////////////////
        //
        // Adds/replaces the given meta object at the (optional) specified location.
        //
        // PAX keeps things simple by not allowing insertions. If you add a meta
        // having the same name as one in a different location, the old one gets
        // automatically whacked. The count for that location then becomes inaccurate,
        // but that's ok because it is only used for labeling comments and sorting.
        // It is not possible for the order to get screwed up. Neat, huh! It would be
        // a pain to do an insert now. You have to remove all meta from that location
        // after the insert point, add the new one, then re-add the others using
        // addComment and addMetaVal to keep the indexes straight. That's ok for now.
        //
        int addMeta(std::string name, meta_t meta, metaLoc_e loc = metaLoc_e::LOC_UNKNOWN) {

            if (metaLoc_e::LOC_UNKNOWN >= loc || metaLoc_e::LOC_COUNT <= loc) loc = _metaLoc;

            meta.loc = loc;
            meta.index = _metaLocCount[loc]++;
            if (paxMetaDataTypes_e::paxComment == meta.type) {
                meta.commentName();
            }

            getMetaRef()[name] = meta;

            _metaLoc = loc;

            return PaxStatic::getStatus();

        } // int addMeta (std::string name, meta_t meta, metaLoc_e loc = metaLoc_e::LOC_UNKNOWN)
        int addMeta(metaLoc_e loc, std::string name, meta_t meta) {
            return addMeta(name, meta, loc);
        }


        //////////////////////////////////////////////////////////////////////////
        //
        // Adds the given comment meta at the (optional) specified location
        //
        int addComment(meta_t meta, metaLoc_e loc = metaLoc_e::LOC_UNKNOWN) {
            return addMeta("", meta, loc);
        }
        int addComment(metaLoc_e loc, meta_t meta) {
            return addMeta("", meta, loc);
        }


        //////////////////////////////////////////////////////////////////////////
        //
        // Adds a comment at the (optional) specified location
        //
        int addComment(std::string comment, metaLoc_e loc = metaLoc_e::LOC_UNKNOWN) {

            if (metaLoc_e::LOC_UNKNOWN >= loc || metaLoc_e::LOC_COUNT <= loc) loc = _metaLoc;

            meta_t meta;
            meta.loc = loc;
            meta.index = _metaLocCount[loc]++;

            std::string name = meta.commentName();
            meta.s[0] = '\0';
            strncat(meta.s, comment.c_str(), PAX_MAX_METADATA_STRING_LENGTH - 1);
            meta.type = paxMetaDataTypes_e::paxComment;
            meta.stripped = strlen(meta.s) > 0;  // eliminates hanging space for null comments

            getMetaRef()[name] = meta;

            _metaLoc = loc;

            return PAX_OK;
        } // int addComment (std::string comment, metaLoc_e loc = metaLoc_e::LOC_UNKNOWN) 
        int addComment(metaLoc_e loc, std::string comment) {
            return addComment(comment, loc);
        }


        //////////////////////////////////////////////////////////////////////////
        //
        // Adds/replaces string metadata at the (optional) specified location
        //
        int addMetaVal(std::string name, std::string data, metaLoc_e loc = metaLoc_e::LOC_UNKNOWN) {

            if (metaLoc_e::LOC_UNKNOWN >= loc || metaLoc_e::LOC_COUNT <= loc) loc = _metaLoc;

            // TODO add meta ctor(s) to reduce line count
            meta_t meta;
            meta.loc = loc;
            meta.index = _metaLocCount[loc]++;
            meta.name = name;
            meta.s[0] = '\0';
            strncat(meta.s, data.c_str(), PAX_MAX_METADATA_STRING_LENGTH - 1);
            meta.type = paxMetaDataTypes_e::paxString;
            meta.stripped = true;

            getMetaRef()[name] = meta;

            _metaLoc = loc;

            return PAX_OK;

        } // int addMetaVal (std::string name, std::string data, metaLoc_e loc = metaLoc_e::LOC_UNKNOWN) 
        int addMetaVal(metaLoc_e loc, std::string name, std::string data) {
            return addMetaVal(name, data, loc);
        }


        //////////////////////////////////////////////////////////////////////////
        //
        // Adds/replaces float metadata at the (optional) specified location
        //
        int addMetaVal(std::string name, float data, metaLoc_e loc = metaLoc_e::LOC_UNKNOWN) {

            if (metaLoc_e::LOC_UNKNOWN >= loc || metaLoc_e::LOC_COUNT <= loc) loc = _metaLoc;

            meta_t meta;
            meta.loc = loc;
            meta.index = _metaLocCount[loc]++;
            meta.name = name;
            meta.f = data;
            meta.type = paxMetaDataTypes_e::paxFloat;

            getMetaRef()[name] = meta;

            _metaLoc = loc;

            return PAX_OK;

        } // int addMetaVal (std::string name, double data, metaLoc_e loc = metaLoc_e::LOC_UNKNOWN)
        int addMetaVal(metaLoc_e loc, std::string name, float data) {
            return addMetaVal(name, data, loc);
        }


        //////////////////////////////////////////////////////////////////////////
        //
        // Adds/replaces double metadata at the (optional) specified location
        //
        int addMetaVal(std::string name, double data, metaLoc_e loc = metaLoc_e::LOC_UNKNOWN) {

            if (metaLoc_e::LOC_UNKNOWN >= loc || metaLoc_e::LOC_COUNT <= loc) loc = _metaLoc;

            meta_t meta;
            meta.loc = loc;
            meta.index = _metaLocCount[loc]++;
            meta.name = name;
            meta.d = data;
            meta.type = paxMetaDataTypes_e::paxDouble;

            getMetaRef()[name] = meta;

            _metaLoc = loc;

            return PAX_OK;

        } // int addMetaVal (std::string name, double data, metaLoc_e loc = metaLoc_e::LOC_UNKNOWN)
        int addMetaVal(metaLoc_e loc, std::string name, double data) {
            return addMetaVal(name, data, loc);
        }


        //////////////////////////////////////////////////////////////////////////
        //
        // Adds/replaces integer metadata at the (optional) specified location
        //
        int addMetaVal(std::string name, uint64_t data, metaLoc_e loc = metaLoc_e::LOC_UNKNOWN, paxMetaDataTypes_e type = paxMetaDataTypes::paxUint64) {

            if (metaLoc_e::LOC_UNKNOWN >= loc || metaLoc_e::LOC_COUNT <= loc) loc = _metaLoc;

            meta_t meta;
            meta.loc = loc;
            meta.index = _metaLocCount[loc]++;
            meta.name = name;
            meta.u64 = data;
            meta.type = type;

            getMetaRef()[name] = meta;

            _metaLoc = loc;

            return PAX_OK;

        } // int addMetaVal (std::string name, double data, metaLoc_e loc = metaLoc_e::LOC_UNKNOWN)
        int addMetaVal(std::string name, uint32_t data, metaLoc_e loc = metaLoc_e::LOC_UNKNOWN) {
            int ret = addMetaVal(name, (uint64_t)data, loc, paxMetaDataTypes::paxUint32);
            return ret;
        }
        int addMetaVal(std::string name, uint16_t data, metaLoc_e loc = metaLoc_e::LOC_UNKNOWN) {
            int ret = addMetaVal(name, (uint64_t)data, loc, paxMetaDataTypes::paxUint16);
            return ret;
        }
        int addMetaVal(std::string name, uint8_t data, metaLoc_e loc = metaLoc_e::LOC_UNKNOWN) {
            int ret = addMetaVal(name, (uint64_t)data, loc, paxMetaDataTypes::paxUint8);
            return ret;
        }
        int addMetaVal(metaLoc_e loc, std::string name, uint64_t data) {
            return addMetaVal(name, (uint64_t)data, loc, paxMetaDataTypes::paxUint64);
        }
        int addMetaVal(metaLoc_e loc, std::string name, uint32_t data) {
            return addMetaVal(name, (uint64_t)data, loc, paxMetaDataTypes::paxUint32);
        }
        int addMetaVal(metaLoc_e loc, std::string name, uint16_t data) {
            return addMetaVal(name, (uint64_t)data, loc, paxMetaDataTypes::paxUint16);
        }
        int addMetaVal(metaLoc_e loc, std::string name, uint8_t data) {
            return addMetaVal(name, (uint64_t)data, loc, paxMetaDataTypes::paxUint8);
        }


        //////////////////////////////////////////////////////////////////////////
        //
        // Adds/replaces integer metadata at the (optional) specified location
        //
        int addMetaVal(std::string name, int64_t data, metaLoc_e loc = metaLoc_e::LOC_UNKNOWN, paxMetaDataTypes_e type = paxMetaDataTypes::paxInt64) {

            if (metaLoc_e::LOC_UNKNOWN >= loc || metaLoc_e::LOC_COUNT <= loc) loc = _metaLoc;

            meta_t meta;
            meta.loc = loc;
            meta.index = _metaLocCount[loc]++;
            meta.name = name;
            meta.n64 = data;
            meta.type = type;

            getMetaRef()[name] = meta;

            _metaLoc = loc;

            return PAX_OK;

        } // int addMetaVal (std::string name, double data, metaLoc_e loc = metaLoc_e::LOC_UNKNOWN)
        int addMetaVal(std::string name, int32_t data, metaLoc_e loc = metaLoc_e::LOC_UNKNOWN) {
            int ret = addMetaVal(name, (int64_t)data, loc, paxMetaDataTypes::paxInt32);
            return ret;
        }
        int addMetaVal(std::string name, int16_t data, metaLoc_e loc = metaLoc_e::LOC_UNKNOWN) {
            int ret = addMetaVal(name, (int64_t)data, loc, paxMetaDataTypes::paxInt16);
            return ret;
        }
        int addMetaVal(std::string name, int8_t data, metaLoc_e loc = metaLoc_e::LOC_UNKNOWN) {
            int ret = addMetaVal(name, (int64_t)data, loc, paxMetaDataTypes::paxInt8);
            return ret;
        }
        int addMetaIntVal(metaLoc_e loc, std::string name, int64_t data) {
            return addMetaVal(name, (int64_t)data, loc, paxMetaDataTypes::paxInt64);
        }
        int addMetaIntVal(metaLoc_e loc, std::string name, int32_t data) {
            return addMetaVal(name, (int64_t)data, loc, paxMetaDataTypes::paxInt32);
        }
        int addMetaIntVal(metaLoc_e loc, std::string name, int16_t data) {
            return addMetaVal(name, (int64_t)data, loc, paxMetaDataTypes::paxInt16);
        }
        int addMetaIntVal(metaLoc_e loc, std::string name, int8_t data) {
            return addMetaVal(name, (int64_t)data, loc, paxMetaDataTypes::paxInt8);
        }


        //////////////////////////////////////////////////////////////////////////
        //
        // get values per element for internal type
        //
        int32_t vpe() {
            return getVPE(E);
        }


        //////////////////////////////////////////////////////////////////////////
        //
        // get bytes per value for internal type
        //
        int32_t bpv() {
            return getBPV(E);
        }


        //////////////////////////////////////////////////////////////////////////
        //
        // get data length
        //
        int32_t datalen() {
            return getBPV(E) * getVPE(E) * _numValues;
        }


        //////////////////////////////////////////////////////////////////////////
        //
        // extract PAX type name for given type
        //
        std::string getTypeName() {
            return rasterFileBase::getTypeName(E);
        }


        //////////////////////////////////////////////////////////////////////////
        //
        // Convert byte data to float
        //
        std::shared_ptr<float> byteToFloatData() {
            std::shared_ptr<float> floatData(new float[getNumElements()]);
            float *data = floatData.get();

            for (uint32_t y = 0; y < _numStrided; ++y) {
                for (uint32_t x = 0; x < _numSequential; ++x) {
                    data[y * _numSequential + x] = (float)ucharValXY(x, y);
                }
            }

            return floatData;

        } // std::shared_ptr<float> byteToFloatData() 


        //////////////////////////////////////////////////////////////////////////
        //
        // Convert float data to uint8
        //
        std::shared_ptr<uint8_t> floatToByteData() {

            std::shared_ptr<uint8_t> floatData(new uint8_t[getNumElements()]);
            uint8_t *data = floatData.get();

            float val;
            uint8_t byte;
            for (uint32_t y = 0; y < _numStrided; ++y) {
                for (uint32_t x = 0; x < _numSequential; ++x) {
                    val = floatValXY(x, y);
                    byte = (val <= 0) ? 0 : ((val >= 255.0f) ? 255 : (uint8_t)val);
                    data[y * _numSequential + x] = byte;
                }
            }

            return floatData;

        } // std::shared_ptr<uint8_t> floatToByteData() 


        //////////////////////////////////////////////////////////////////////////
        //
        // Convert the data to a PGM file (currently only valid for UCHAR, CHAR, FLOAT)
        // Valid values for pgmType are 2 and 5 for P2 (8-bit ascii) and P5 (8-bit binary), respectively.
        //
        paxBufPtr toPGM(int pgmType = 5) {

            ucharRasterFile *paxPtr;  // use an abstract pointer to facilitate conversion
            ucharRasterFilePtr bytePtr;
            std::shared_ptr<uint8_t> byteData;

            if (pgmType != 2 && pgmType != 5) return nullptr;

            switch (_dataType) {
                //paxPtr = this;
                //break;

            case paxTypes::ePAX_UCHAR:
            case paxTypes::ePAX_CHAR:
                paxPtr = reinterpret_cast<ucharRasterFile *>(this);
                break;

            case paxTypes::ePAX_FLOAT:
                byteData = floatToByteData();
                bytePtr = std::make_shared<ucharRasterFile>(_numSequential, _numStrided, byteData.get());
                paxPtr = bytePtr.get();
                break;

            default:
                return nullptr;
            }

            // write output file to a pgm file
            std::ostringstream hdrs;
            const char *pgmTag = (2 == pgmType) ? "P2\n" : "P5\n";
            hdrs << pgmTag << _numSequential << " " << _numStrided << "\n255\n";

            std::string hdr = hdrs.str();
            size_t hdrLen = hdr.length();
            size_t dataLen = getNumValues() * sizeof(uint8_t);
            if (2 == pgmType) {
                dataLen = 4 * dataLen + 1; // length of ascii buffer is exactly this (all values are 3 digits, plus spaces and LF's, plus NULL)
            }

            size_t imgLen = hdrLen + dataLen;

            paxBufPtr pgmBuf = std::make_shared<paxBuf_t>(imgLen);
            char *pgmbuf = pgmBuf.get()->data();
            uint8_t *paxBuf = reinterpret_cast<uint8_t*> (paxPtr->buf());

            // TODO: PAX insertion operator for paxBufData_t
            memcpy(pgmbuf, hdr.c_str(), hdrLen);
            pgmbuf += hdrLen;

            if (2 == pgmType) {
              // write ascii file
                size_t len = 0;

                for (uint32_t j = 0; j < _numStrided; ++j) {
                    for (uint32_t i = 0; i < _numSequential; ++i) {
                        len += pax_sprintf(pgmbuf + len, "%3d ", *(paxBuf++));
                    }
                    pgmbuf[len - 1] = '\n';
                }
                pgmbuf[len - 1] = '\n';
                pgmbuf[len] = ' ';

            } else {
                memcpy(pgmbuf, paxBuf, dataLen);
            }

            return pgmBuf;

        } // paxBufPtr toPGM(int pgmType = 5)  


        //////////////////////////////////////////////////////////////////////////
        //
        // Write the data to a PGM file (currently only valid for ePAX_UCHAR, ePAX_CHAR, ePAX_FLOAT)
        // Valid values for pgmType are 2 and 5 for P2 (8-bit ascii) and P5 (8-bit binary), respectively.
        //
        int writeToPGMFile(std::string fileName, int pgmType = 5) {

            paxBufPtr pgmBuf = toPGM(pgmType);

            if (nullptr == pgmBuf) {
                PAX_LOG_ERROR(1, << "Error writing PAX to PGM!! Filename was going to be '" << fileName << "'");
                return PAX_FAIL;
            }

            int ret = rasterFileBase::writeToFile(pgmBuf, fileName);

            if (PAX_OK != ret) {
                PAX_LOG_ERROR(1, << "Error writing PGM file '" << fileName << "'");
            }

            return ret;

        } // int writeToPGMFile(std::string fileName, int pgmType = 5) 


        //////////////////////////////////////////////////////////////////////////
        //
        // import PAX file from file
        //
        int import(pax_filestring fileName) {

            PAX_LOG(1, << "Importing PAX file " << fileName);

            paxBufPtr fileBuf = readFile(fileName);
            if (!fileBuf) {
              // error has already been reported
                return PAX_FAIL;
            }

            if (fileBuf.get()->size() < MIN_PAX_LENGTH) {
                PAX_LOG_ERROR(1, << ("PAX file too short"));
                return PAX_FAIL;
            }

            return import(fileBuf);

        } // int import (pax_filestring fileName)


        //////////////////////////////////////////////////////////////////////////
        //
        // import PAX file from paxBufPtr
        //
        int import(paxBufPtr inBuf) {
            return import(inBuf.get()->data(), inBuf.get()->size());
        }


        //////////////////////////////////////////////////////////////////////////
        //
        // import PAX file from constant buffer
        //
        int import_copy(const unsigned char* inBuf, size_t length) {
            std::vector<unsigned char> buf(length);
            memcpy(buf.data(), inBuf, length);
            PAX_LOG(2, << "PAX import_copy copied an unsigned buffer of length " << length);
            return import(buf.data(), length);
        }


        //////////////////////////////////////////////////////////////////////////
        //
        // import PAX file from buffer
        //
        int import(unsigned char* inBuf, size_t length) {
            return import((char*)inBuf, length);
        }


        //////////////////////////////////////////////////////////////////////////
        //
        // import PAX file from const buffer
        //
        int import_copy(const char* inBuf, size_t length) {
            std::vector<char> buf(length);
            memcpy(buf.data(), inBuf, length);
            PAX_LOG(2, << "PAX import_copy copied a signed buffer of length " << length);
            return import(buf.data(), length);
        }


        //////////////////////////////////////////////////////////////////////////
        //
        // import PAX file from buffer
        //
        int import(char* inBuf, size_t length) {

            if (_numValues != 0 || _numSequential != 0 || _numStrided != 0 || _buf != nullptr || _meta != nullptr) {
                reset();
            }

            PAX_LOG(1, << "Importing PAX buffer of length " << length);

            // parse the header
            BufMan buf(inBuf, length);

            int32_t dataLen = 0;
            int headerRet = importHeader(buf, dataLen);

            if (PAX_OK != headerRet) {
                return PAX_FAIL;
            }

            // copy that data
            paxBufPtr dataBuf = std::make_shared <paxBuf_t>(dataLen);
            buf.copyData(dataBuf.get()->data(), dataLen);

            // store those metadata counts
            auto metavecs = getMetaVecs();
            int i = 0;
            for (auto vec : (*metavecs)) {
                _metaLocCount[i] = vec.size();
                ++i;
            }

            if (PaxStatic::getVerbosity() >= 3) {
                PAX_LOG(3, << "Some data for ya:");
                float * buf = (float*)dataBuf.get()->data();
                for (uint32_t i = 0; i < PAX_MIN(8, _numSequential); ++i) {
                    std::stringstream ss;
                    for (uint32_t j = 0; j < PAX_MIN(8, _numStrided); ++j) {
                        ss << std::setw(12) << buf[i + j * _numSequential];
                    }
                    PAX_LOG(3, << ss.str());
                }
            }

            _buf = dataBuf;
            _importedLength = buf.offset();

            if (PaxStatic::getVerbosity() >= 3) {
                std::stringstream ss;
                ss << "Total imported length = " << _importedLength << " bytes.";
                PAX_LOG(3, << ss.str());
            }

            return PAX_OK;
#undef verbosityLevel

        } // int import (char* inBuf, size_t length)


        //////////////////////////////////////////////////////////////////////////
        //
        // TODO: import PAX file from char string
        //
        //int import (char * inBuf) {
        //  return PAX_FAIL;
        //}


        //////////////////////////////////////////////////////////////////////////
        //
        // Helper function to write metadata. Assumes the given data have been sorted.
        //
        int writeMeta(pax_stringstream &ss, std::vector<std::pair<std::string, meta_t>> &metavec) {

            for (auto meta : metavec) {

                pax_stringstream ssmeta;
                std::streamsize prec = ssmeta.precision(15);
                paxMetaDataTypes_e type = meta.second.type;

                // handle trivial and nonnumeric types first
                switch (type) {

                case paxMetaDataTypes_e::paxComment:
                    ss << (meta.second.stripped ? "# " : "#") << meta.second.s /*+ skip*/ << '\n';
                    continue;

                case paxMetaDataTypes_e::paxString:
                    ss << "## [" << METATYPE_STRING_TAG << "]   ";
                    ss << meta.first << (meta.second.stripped ? " = " : " =") << meta.second.s /*+ skip */ << '\n';
                    continue;

                case paxMetaDataTypes_e::paxInvalid:
                    continue;

                default: break;

                } // switch (type), trivial types

                size_t count = meta.second.count();
                size_t subrowlength = 1;
                size_t rowlength = 1;

                pax_stringstream sstypetag;
                sstypetag << "[" << PaxStatic::getMetaTypeTag(type) << "]      ";
                ssmeta << "## " << sstypetag.str().substr(0, 11) << meta.first;  // write the type tag and name

                if (meta.second.dimCount() >= 1) {

                  // choose a reasonable length for separating data into rows
                    for (int i = 0; i < meta.second.num_dims && rowlength < 16; ++i) {
                        rowlength *= meta.second.dims[i];
                        if (subrowlength * meta.second.dims[i] < 8) subrowlength *= meta.second.dims[i];
                    }

                    // write the array index tags and values
                    ssmeta << " [";
                    for (size_t i = 0; i < meta.second.dimCount(); ++i) {
                        ssmeta << " " << PaxStatic::getMetaArrayIndexTag((uint32_t)i) << " = " << meta.second.dims[i];
                    }
                    ssmeta << " ]";

                }

                ssmeta << " ="; // write delimiter

                // multidimensional arrays begin on a new line and indented
                for (uint32_t i = 0; i < count; ++i) {

                    if (0 == (i % rowlength) && meta.second.dimCount() > 1) {
                        ssmeta << "\n ";
                    }

                    switch (type) {

                    case paxMetaDataTypes_e::paxFloat:      ssmeta << " " << meta.second.fb[i];       break;

                    case paxMetaDataTypes_e::paxDouble:     ssmeta << " " << meta.second.db[i];       break;

                    case paxMetaDataTypes_e::paxInt64:      ssmeta << " " << meta.second.n64b[i];     break;

                    case paxMetaDataTypes_e::paxUint64:     ssmeta << " " << meta.second.u64b[i];     break;

                    case paxMetaDataTypes_e::paxInt32:      ssmeta << " " << meta.second.n32b[i];     break;

                    case paxMetaDataTypes_e::paxUint32:     ssmeta << " " << meta.second.u32b[i];     break;

                    case paxMetaDataTypes_e::paxInt16:      ssmeta << " " << meta.second.n16b[i];     break;

                    case paxMetaDataTypes_e::paxUint16:     ssmeta << " " << meta.second.u16b[i];     break;

                    case paxMetaDataTypes_e::paxInt8:       ssmeta << " " << +meta.second.n8b[i];     break;

                    case paxMetaDataTypes_e::paxUint8:      ssmeta << " " << +meta.second.u8b[i];     break;

                    default: continue;

                    } // switch (type)

                } // for (uin32_t i = 0; i < count; ++i)

                ss << ssmeta.str() << "\n";

            } // for (auto meta : metavec) 

            return PAX_OK;
        }


        //////////////////////////////////////////////////////////////////////////
        //
        // output PAX file to buffer
        //
        int writeToBuffer(paxBufPtr &outBuf) {

            pax_stringstream ss;
            size_t _bpv = bpv();
            size_t _vpe = vpe();
            size_t dataLen = _bpv * _vpe * _numSequential * _numStrided;
            int32_t metaLoc = LOC_UNKNOWN;

            auto meta = getMetaVecs();

            // write file ID line
            ss << PAX_TAG << (int)_dataType << " : v" << std::fixed << std::setprecision(2) << _version << " : " << getTypeName() << '\n';
            PAX_LOG(3, << "typeName = " << getTypeName().c_str());
            PAX_LOG(3, << "version = " << _version);

            ss.unsetf(std::ios::floatfield);

            metaLoc = LOC_AFTER_TAG;
            std::vector<std::pair<std::string, meta_t>> &metavec = (*meta)[metaLoc];
            PAX_LOG(3, << "writing " << metavec.size() << " metadata lines at location " << metaLoc);
            writeMeta(ss, metavec);

            ss << BPV_TAG << " : " << _bpv << '\n';

            metaLoc = LOC_AFTER_BPV;
            metavec = (*meta)[metaLoc];
            PAX_LOG(3, << "writing " << metavec.size() << " metadata lines at location " << metaLoc);
            writeMeta(ss, metavec);

            ss << VPE_TAG << " : " << _vpe << '\n';

            metaLoc = LOC_AFTER_VPE;
            metavec = (*meta)[metaLoc];
            PAX_LOG(3, << "writing " << metavec.size() << " metadata lines at location " << metaLoc);
            writeMeta(ss, metavec);
            ss << DIM1_TAG << " : " << _numSequential << '\n';

            metaLoc = LOC_AFTER_SEQ;
            metavec = (*meta)[metaLoc];
            PAX_LOG(3, << "writing " << metavec.size() << " metadata lines at location " << metaLoc);
            writeMeta(ss, metavec);
            ss << DIM2_TAG << " : " << _numStrided << '\n';

            metaLoc = LOC_AFTER_STR1;
            metavec = (*meta)[metaLoc];
            PAX_LOG(3, << "writing " << metavec.size() << " metadata lines at location " << metaLoc);
            writeMeta(ss, metavec);

            ss << DATALEN_TAG << " : " << dataLen << '\n';

            std::string header = ss.str();
            size_t headerLen = pax_strlen(header.c_str());
            size_t bufLen = dataLen + headerLen;

            PAX_LOG(1, << "Wrote to buffer: " << headerLen << " header bytes and " << dataLen << " data bytes for a total of " << bufLen << " bytes");

            paxBufPtr buf = std::make_shared<paxBuf_t>(bufLen);
            memcpy(buf->data(), header.c_str(), headerLen);
            if (dataLen > 0) {
                memcpy(buf->data() + headerLen, _buf->data(), dataLen);
            }

            outBuf = buf;

            return PAX_OK;
        }


        //////////////////////////////////////////////////////////////////////////
        //
        // output to file
        //
        int writeToFile(pax_filestring fileName) {
            PAX_LOG(1, << "Writing PAX data " << " to " << fileName);

            paxBufPtr buf;
            writeToBuffer(buf);

            return rasterFileBase::writeToFile(buf, fileName);
        }


        //////////////////////////////////////////////////////////////////////////
        //
        // Basic buffer access
        //
        char * buf() {
            if (!_buf) return NULL;
            return _buf.get()->data();
        }


        //////////////////////////////////////////////////////////////////////////
        //
        // Read elements from the array. Accessors are defined for both X,Y and R,C
        // indexing styles. The data is always stored column-major (i.e. X/C is the fast dim).
        //
        template <typename T>
        T & value(uint64_t x, uint64_t y = 0) {
            T * tbuf = (T*)buf();
            if (NULL == tbuf || x >= _numSequential || y >= _numStrided) return *((T*)_junk);
            uint64_t index = x + y * _numSequential;
            return tbuf[index];
        }

        float    & floatValXY(uint64_t x, uint64_t y = 0) { return value<float>(x, y); };
        double   & doubleValXY(uint64_t x, uint64_t y = 0) { return value<double>(x, y); };
        int8_t   & charValXY(uint64_t x, uint64_t y = 0) { return value<int8_t>(x, y); };
        int16_t  & shortValXY(uint64_t x, uint64_t y = 0) { return value<int16_t>(x, y); };
        int32_t  & intValXY(uint64_t x, uint64_t y = 0) { return value<int32_t>(x, y); };
        int64_t  & longValXY(uint64_t x, uint64_t y = 0) { return value<int64_t>(x, y); };
        uint8_t  & ucharValXY(uint64_t x, uint64_t y = 0) { return value<uint8_t>(x, y); };
        uint16_t & ushortValXY(uint64_t x, uint64_t y = 0) { return value<uint16_t>(x, y); };
        uint32_t & uintValXY(uint64_t x, uint64_t y = 0) { return value<uint32_t>(x, y); };
        uint64_t & ulongValXY(uint64_t x, uint64_t y = 0) { return value<uint64_t>(x, y); };
        csingle  & csingleValXY(uint64_t x, uint64_t y = 0) { return value<csingle>(x, y); };
        cdouble  & cdoubleValXY(uint64_t x, uint64_t y = 0) { return value<cdouble>(x, y); };
        pax_float3_t & cfloat3ValXY(uint64_t x, uint64_t y = 0) { return value<pax_float3_t>(x, y); };

        float    & floatValRC(uint64_t r, uint64_t c = 0) { return value<float>(c, r); };
        double   & doubleValRC(uint64_t r, uint64_t c = 0) { return value<double>(c, r); };
        int8_t   & charValRC(uint64_t r, uint64_t c = 0) { return value<int8_t>(c, r); };
        int16_t  & shortValRC(uint64_t r, uint64_t c = 0) { return value<int16_t>(c, r); };
        int32_t  & intValRC(uint64_t r, uint64_t c = 0) { return value<int32_t>(c, r); };
        int64_t  & longValRC(uint64_t r, uint64_t c = 0) { return value<int64_t>(c, r); };
        uint8_t  & ucharValRC(uint64_t r, uint64_t c = 0) { return value<uint8_t>(c, r); };
        uint16_t & ushortValRC(uint64_t r, uint64_t c = 0) { return value<uint16_t>(c, r); };
        uint32_t & uintValRC(uint64_t r, uint64_t c = 0) { return value<uint32_t>(c, r); };
        uint64_t & ulongValRC(uint64_t r, uint64_t c = 0) { return value<uint64_t>(c, r); };
        csingle  & csingleValRC(uint64_t r, uint64_t c = 0) { return value<csingle>(c, r); };
        cdouble  & cdoubleValRC(uint64_t r, uint64_t c = 0) { return value<cdouble>(c, r); };
        pax_float3_t & cfloat3ValRC(uint64_t r, uint64_t c = 0) { return value<pax_float3_t>(c, r); };


        //int import (
        //  uint32_t      numSequential,
        //  uint32_t      numStrided,
        //  paxBufPtr     buf,
        //  paxMetaDataPtr meta
        //) {
        //  _numSequential = numSequential;
        //  _numStrided = numStrided;
        //  return 0;
        //};

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //
        // private functions
        //

        //////////////////////////////////////////////////////////////////////////
        //
        // premature end of file error
        //
        void reportEOF() {
            PAX_LOG_ERROR(1, << "premature end to PAX file found");
        }


        //////////////////////////////////////////////////////////////////////////
        //
        // data operator access does not work because we can't infer the type at runtime
        //
        //E operator[](size_t idx) {
        //  size_t x = idx % _numSequential;
        //  size_t y = idx / _numSequential;
        //  E val = value<E> (x, y);
        //  return E;
        //}
        //E operator[](size_t x, size_t y) {
        //  E val = value<E> (x, y);
        //  return E;
        //}
        //E operator()(size_t r, size_t c) {
        //  E val = value<E> (c, r);
        //  return E;
        //}


        //////////////////////////////////////////////////////////////////////////
        //
        // metadata operator access
        //
        double operator[](std::string key) {
            return getMetaDouble(key);
        }
        std::string operator()(std::string key) {
            return getMetaString(key);
        }


      //////////////////////////////////////////////////////////////////////////
      // internal data
    protected:
        paxBufPtr           _buf;
        uint8_t             _junk[32];  // garbage data for accessors that have to return references
    };  // template <paxTypes_e E> class rasterFile

///@}

} // namespace pax

} // namespace sss
