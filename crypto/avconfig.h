#ifndef AVUTIL_AVCONFIG_H
#define AVUTIL_AVCONFIG_H

#define AV_HAVE_BIGENDIAN 0
#define AV_HAVE_FAST_UNALIGNED 1
#define AV_HAVE_INCOMPATIBLE_FORK_ABI 0

#define CONFIG_HARDCODED_TABLES      1
#define CONFIG_FFRTMPCRYPT_PROTOCOL  0
#define CONFIG_SMALL                 0

#if defined(__GNUC__)
#    define av_unused __attribute__((unused))
#else
#    define av_unused
#define snprintf      _snprintf
#pragma warning(disable: 4018) //“>”: 有符号/无符号不匹配
#pragma warning(disable: 4057) //“函数”: “const uint8_t *”与“const char *”在稍微不同的基类型间接寻址上不同
#pragma warning(disable: 4127) //条件表达式是常量
#pragma warning(disable: 4244) //从“const unsigned int”转换到“uint8_t”，可能丢失数据
#pragma warning(disable: 4389) //“!=”: 有符号/无符号不匹配
#endif

#define av_alias
#define av_const
#define av_unused
#define av_cold
#define av_log(...)
#define av_pure

#ifndef av_always_inline
#if defined(_MSC_VER)
#    define av_always_inline __forceinline
#    define inline __inline
#elif AV_GCC_VERSION_AT_LEAST(3,1)
#    define av_always_inline __attribute__((always_inline)) inline
#else
#    define av_always_inline inline
#endif
#endif

/* error handling */
#if EDOM > 0
#define AVERROR(e) (-(e))   ///< Returns a negative error code from a POSIX error code, to return from library functions.
#define AVUNERROR(e) (-(e)) ///< Returns a POSIX error code from a library function error return value.
#else
/* Some platforms have E* and errno already negated. */
#define AVERROR(e) (e)
#define AVUNERROR(e) (e)
#endif

#define AVERROR_INVALIDDATA  AVERROR(EINVAL)

#define FFMAX(a,b) ((a) > (b) ? (a) : (b))
#define FFMAX3(a,b,c) FFMAX(FFMAX(a,b),c)
#define FFMIN(a,b) ((a) > (b) ? (b) : (a))
#define FFMIN3(a,b,c) FFMIN(FFMIN(a,b),c)

#ifndef av_clip
static av_always_inline av_const int av_clip(int a, int amin, int amax)
{
#if defined(HAVE_AV_CONFIG_H) && defined(ASSERT_LEVEL) && ASSERT_LEVEL >= 2
	if (amin > amax) abort();
#endif
	if      (a < amin) return amin;
	else if (a > amax) return amax;
	else               return a;
}
#endif


#endif
